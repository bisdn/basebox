/*
 * ctapdev.cc
 *
 *  Created on: 24.06.2013
 *      Author: andreas
 */

#include <sys/socket.h>
#include <linux/if.h>

#include "ctapdev.hpp"
#include "roflibs/netlink/tap_manager.hpp"
#include "roflibs/netlink/cpacketpool.hpp"

extern int errno;

namespace rofcore {

ctapdev::ctapdev(tap_callback &cb, std::string const &devname, pthread_t tid)
    : fd(-1), devname(devname), thread(this), cb(cb) {
  try {
    tap_open(); /// XXX FIXME exceptions are wrong handled here
  } catch (std::exception &e) {
    thread.add_timer(CTAPDEV_TIMER_OPEN_PORT, rofl::ctimespec().expire_in(1));
  }
}

ctapdev::~ctapdev() {
  tap_close();
  thread.stop();
}

void ctapdev::tap_open() {
  try {
    struct ifreq ifr;
    int rc;

    if (fd > -1) {
      tap_close();
    }

    if ((fd = open("/dev/net/tun", O_RDWR | O_NONBLOCK)) < 0) {
      throw eTapDevOpenFailed(
          "ctapdev::tap_open() opening /dev/net/tun failed");
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
      throw eTapDevIoctlFailed("ctapdev::tap_open() setting flags IFF_TAP | "
                               "IFF_NO_PI on /dev/net/tun failed");
    }

    thread.start();
    thread.add_read_fd(fd);
    thread.add_write_fd(fd);

  } catch (eTapDevOpenFailed &e) {
    rofcore::logging::error
        << "ctapdev::tap_open() open() failed, dev:" << devname << std::endl
        << rofl::eSysCall("open");
  } catch (eTapDevIoctlFailed &e) {
    rofcore::logging::error
        << "ctapdev::tap_open() open() failed, dev:" << devname << std::endl
        << rofl::eSysCall("ctapdev ioctl");
  }
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
        rofcore::logging::error
            << "ctapdev::handle_revent() EAGAIN, retrying later" << std::endl;
      default:
        rofcore::logging::error << "ctapdev::handle_revent() error occured"
                                << std::endl;
      }
    } else {
      pkt = cpacketpool::get_instance().acquire_pkt();

      pkt->unpack(mem.somem(), rc);

      cb.enqueue(this, pkt);
    }

  } catch (ePacketPoolExhausted &e) {
    rofcore::logging::error << "ctapdev::handle_revent() packet pool "
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
        rofcore::logging::debug << "ctapdev::tx() EAGAIN" << std::endl;
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
        rofcore::logging::error
            << "ctapdev::tx() unknown error occured rc=" << rc
            << " errno=" << errno << " '" << strerror(errno) << std::endl;
        return;
      }
    }
    cpacketpool::get_instance().release_pkt(pkt);
    out_queue.pop_front();
  }
}

void ctapdev::handle_timeout(rofl::cthread &thread, uint32_t timer_id,
                             const std::list<unsigned int> &ttypes) {
  switch (timer_id) {
  case CTAPDEV_TIMER_OPEN_PORT: {
    try {
      tap_open();
    } catch (...) {
      thread.add_timer(CTAPDEV_TIMER_OPEN_PORT, rofl::ctimespec().expire_in(1));
    }
  } break;
  }
}

} // namespace rofcore
