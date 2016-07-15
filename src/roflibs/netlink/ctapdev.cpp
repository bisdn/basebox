/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>

#include <cerrno>

#include "ctapdev.hpp"
#include "roflibs/netlink/tap_manager.hpp"
#include "roflibs/netlink/cpacketpool.hpp"
#include <glog/logging.h>

namespace rofcore {

ctapdev::ctapdev(tap_callback &cb, std::string const &devname, pthread_t tid)
    : fd(-1), devname(devname), thread(this), cb(cb) {
  if (devname.size() > IFNAMSIZ) {
    throw std::length_error("devname.size() > IFNAMSIZ");
  }
}

ctapdev::~ctapdev() {
  tap_close();
  thread.stop();
}

void ctapdev::tap_open() {
  struct ifreq ifr;
  int rc;

  if (fd != -1) {
    return;
  }

  if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
    assert(0 && "CRITICAL: could not open /dev/net/tun");
  }

  memset(&ifr, 0, sizeof(ifr));

  /* Flags: IFF_TUN   - TUN device (no Ethernet headers)
   *        IFF_TAP   - TAP device
   *
   *        IFF_NO_PI - Do not provide packet information
   */
  ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
  strncpy(ifr.ifr_name, devname.c_str(), IFNAMSIZ);

  if ((rc = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0) {
    close(fd);
    fd = -1;
    assert(0 && "CRITICAL: ioctl TUNSETIFF failed");
  }

  thread.add_read_fd(fd);
  thread.add_write_fd(fd);
  thread.start();
}

void ctapdev::tap_close() {
  if (fd == -1) {
    return;
  }

  thread.drop_read_fd(fd, false);
  thread.drop_write_fd(fd, false);

  close(fd);

  fd = -1;
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

void ctapdev::enqueue(std::vector<rofl::cpacket *> pkts) {
  if (fd == -1) {
    for (std::vector<rofl::cpacket *>::iterator it = pkts.begin();
         it != pkts.end(); ++it) {
      cpacketpool::get_instance().release_pkt((*it));
    }
    return;
  }

  // store pkts in outgoing queue
  {
    rofl::AcquireReadWriteLock rwlock(pout_queue_rwlock);
    for (std::vector<rofl::cpacket *>::iterator it = pkts.begin();
         it != pkts.end(); ++it) {
      pout_queue.push_back(*it);
    }
  }

  thread.wakeup();
}

void ctapdev::handle_read_event(rofl::cthread &thread, int fd) {
  rofl::cpacket *pkt = NULL;
  try {

    rofl::cmemory mem(1518);

    int rc = read(fd, mem.somem(), mem.memlen());

    // error occured (or non-blocking)
    if (rc < 0) {
      switch (errno) {
      case EAGAIN:
        LOG(ERROR) << "ctapdev::handle_revent() EAGAIN, retrying later"
                   << std::endl;
      default:
        LOG(ERROR) << "ctapdev::handle_revent() error occured" << std::endl;
      }
    } else {
      pkt = cpacketpool::get_instance().acquire_pkt();

      pkt->unpack(mem.somem(), rc);

      cb.enqueue(this, pkt);
    }

  } catch (ePacketPoolExhausted &e) {
    LOG(ERROR) << "ctapdev::handle_revent() packet pool "
                  "exhausted, no idle slots available"
               << std::endl;
  }
}

void ctapdev::handle_write_event(rofl::cthread &thread, int fd) { tx(); }

static inline void release_packets(std::deque<rofl::cpacket *> &q) {
  for (auto i : q) {
    cpacketpool::get_instance().release_pkt(i);
  }
}

void ctapdev::tx() {
  rofl::cpacket *pkt = NULL;
  std::deque<rofl::cpacket *> out_queue;

  {
    rofl::AcquireReadWriteLock rwlock(pout_queue_rwlock);
    out_queue = std::move(pout_queue);
  }

  while (not out_queue.empty()) {

    pkt = out_queue.front();
    int rc = 0;
    if ((rc = write(fd, pkt->soframe(), pkt->length())) < 0) {
      switch (errno) {
      case EAGAIN:
        VLOG(1) << "ctapdev::tx() EAGAIN" << std::endl;
        {
          rofl::AcquireReadWriteLock rwlock(pout_queue_rwlock);
          std::move(out_queue.rbegin(), out_queue.rend(),
                    std::front_inserter(pout_queue));
        }
        return;
      case EIO:
        // tap not enabled drop packet
        release_packets(out_queue);
        return;
      default:
        // will drop packets
        release_packets(out_queue);
        LOG(ERROR) << "ctapdev::tx() unknown error occured rc=" << rc
                   << " errno=" << errno << " '" << strerror(errno)
                   << std::endl;
        return;
      }
    }
    cpacketpool::get_instance().release_pkt(pkt);
    out_queue.pop_front();
  }
}

} // namespace rofcore
