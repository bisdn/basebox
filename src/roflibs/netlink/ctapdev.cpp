/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>

#include <cerrno>

#include <glog/logging.h>

#include "ctapdev.hpp"
#include "roflibs/netlink/tap_manager.hpp"
#include "roflibs/netlink/cpacketpool.hpp"

namespace rofcore {

static inline void release_packets(std::deque<rofl::cpacket *> &q) {
  for (auto i : q) {
    cpacketpool::get_instance().release_pkt(i);
  }
}

ctapdev::ctapdev(tap_callback &cb, std::string const &devname, uint32_t port_id,
                 pthread_t tid)
    : fd(-1), devname(devname), thread(this), cb(cb), port_id(port_id) {
  if (devname.size() > IFNAMSIZ) {
    throw std::length_error("devname.size() > IFNAMSIZ");
  }
}

ctapdev::~ctapdev() {
  tap_close();
  thread.stop();

  release_packets(pout_queue);
}

void ctapdev::tap_open() {
  struct ifreq ifr;
  int rc;

  if (fd != -1) {
    VLOG(1) << __FUNCTION__ << ": tapdev " << devname
            << " is alredy open using fd=" << fd;
    return;
  }

  if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
    LOG(FATAL) << __FUNCTION__
               << ": could not open /dev/net/tun (module loaded?)";
  }

  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
  strncpy(ifr.ifr_name, devname.c_str(), IFNAMSIZ);

  if ((rc = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0) {
    close(fd);
    fd = -1;
    LOG(FATAL) << __FUNCTION__ << ": ioctl TUNSETIFF failed on fd=" << fd;
  }

  thread.add_read_fd(fd, true, false);
  thread.add_write_fd(fd, true, false);
  thread.start();

  LOG(INFO) << __FUNCTION__ << ": created tapdev " << devname
            << " port_id=" << port_id << " fd=" << fd
            << " tid=" << thread.get_thread_id();
}

void ctapdev::tap_close() {
  if (fd == -1) {
    return;
  }

  try {
    thread.drop_read_fd(fd);
  } catch (std::exception &e) {
    LOG(ERROR) << __FUNCTION__ << ": failed to drop read fd=" << fd;
  }

  try {
    thread.drop_write_fd(fd);
  } catch (std::exception &e) {
    LOG(ERROR) << __FUNCTION__ << ": failed to drop write fd=" << fd;
  }

  int rv = close(fd);
  if (rv < 0)
    LOG(ERROR) << __FUNCTION__ << ": failed to close fd=" << fd;

  fd = -1;

  LOG(INFO) << __FUNCTION__ << ": closed tapdev " << devname
            << " port_id=" << port_id << " tid=" << thread.get_thread_id();
}

void ctapdev::enqueue(rofl::cpacket *pkt) {
  if (fd == -1) {
    cpacketpool::get_instance().release_pkt(pkt);
    return;
  }

  // store pkt in outgoing queue
  {
    rofl::AcquireReadWriteLock rwlock(pout_queue_rwlock);
    pout_queue.push_back(pkt);
  }

  thread.wakeup();
}

void ctapdev::handle_read_event(rofl::cthread &thread, int fd) {
  rofl::cpacket *pkt = nullptr;
  try {
    pkt = cpacketpool::get_instance().acquire_pkt();

    ssize_t n_bytes = read(fd, pkt->soframe(), pkt->length());

    // error occured (or non-blocking)
    if (n_bytes < 0) {
      switch (errno) {
      case EAGAIN:
        LOG(ERROR) << __FUNCTION__
                   << ": EAGAIN XXX not implemented packet is dropped";
        cpacketpool::get_instance().release_pkt(pkt);
      default:
        LOG(ERROR) << __FUNCTION__ << ": unknown error occured";
        cpacketpool::get_instance().release_pkt(pkt);
      }
    } else {
      VLOG(2) << __FUNCTION__ << ": read " << n_bytes << " bytes from fd=" << fd
              << " (" << devname << ") into pkt=" << pkt
              << " enqueuing as port_id=" << port_id
              << " tid=" << pthread_self();
      cb.enqueue_to_switch(port_id, pkt);
    }

  } catch (ePacketPoolExhausted &e) {
    LOG(ERROR) << __FUNCTION__
               << ": packet pool exhausted, no idle slots available";
  }
}

void ctapdev::handle_write_event(rofl::cthread &thread, int fd) { tx(); }

void ctapdev::tx() {
  rofl::cpacket *pkt = NULL;
  std::deque<rofl::cpacket *> out_queue;

  {
    rofl::AcquireReadWriteLock rwlock(pout_queue_rwlock);
    std::swap(out_queue, pout_queue);
  }

  while (not out_queue.empty()) {

    pkt = out_queue.front();
    int rc = 0;
    if ((rc = write(fd, pkt->soframe(), pkt->length())) < 0) {
      switch (errno) {
      case EAGAIN:
        VLOG(1) << __FUNCTION__ << ": EAGAIN";
        {
          rofl::AcquireReadWriteLock rwlock(pout_queue_rwlock);
          std::move(out_queue.rbegin(), out_queue.rend(),
                    std::front_inserter(pout_queue));
        }
        return;
      case EIO:
        // tap not enabled drop packet
        VLOG(1) << __FUNCTION__ << ": EIO";
        release_packets(out_queue);
        return;
      default:
        // will drop packets
        release_packets(out_queue);
        LOG(ERROR) << __FUNCTION__ << ": unknown error occured rc=" << rc
                   << " errno=" << errno << " '" << strerror(errno);
        return;
      }
    }
    cpacketpool::get_instance().release_pkt(pkt);
    out_queue.pop_front();
  }
}

} // namespace rofcore
