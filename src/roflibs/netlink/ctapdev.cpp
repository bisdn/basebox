/*
 * ctapdev.cc
 *
 *  Created on: 24.06.2013
 *      Author: andreas
 */

#include "ctapdev.hpp"

using namespace rofcore;

extern int errno;

ctapdev::ctapdev(cnetdev_owner *netdev_owner, const rofl::cdptid &dptid,
                 std::string const &devname, uint16_t pvid,
                 rofl::cmacaddr const &hwaddr, pthread_t tid)
    : fd(-1), dptid(dptid), devname(devname), pvid(pvid), hwaddr(hwaddr),
      thread(this), netdev_owner(netdev_owner) {
  try {
    tap_open(hwaddr);
  } catch (...) {
    thread.add_timer(CTAPDEV_TIMER_OPEN_PORT, rofl::ctimespec().expire_in(1));
  }
}

ctapdev::~ctapdev() {
  tap_close();
  thread.stop();
}

void ctapdev::tap_open(rofl::cmacaddr const &hwaddr) {
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

    // set_hwaddr(hwaddr);

    // netdev_owner->netdev_open(this);

    thread.start();
    thread.add_read_fd(fd);
    thread.add_write_fd(fd);

  } catch (eTapDevOpenFailed &e) {

    rofcore::logging::error
        << "ctapdev::tap_open() open() failed, dev:" << devname << std::endl
        << rofl::eSysCall("open");

    throw eNetDevCritical(e.what());

  } catch (eTapDevIoctlFailed &e) {

    rofcore::logging::error
        << "ctapdev::tap_open() open() failed, dev:" << devname << std::endl
        << rofl::eSysCall("ctapdev ioctl");

    throw eNetDevCritical(e.what());

  } catch (eNetDevIoctl &e) {

    rofcore::logging::error
        << "ctapdev::tap_open() open() failed, dev:" << devname << std::endl
        << rofl::eSysCall("cnetdev ioctl");

    throw eNetDevCritical(e.what());
  }
}

void ctapdev::tap_close() {
  try {
    if (fd == -1) {
      return;
    }

    thread.drop_read_fd(fd);
    thread.drop_write_fd(fd);

  } catch (eNetDevIoctl &e) {
    rofcore::logging::error << "ctapdev::tap_close() failed: dev:" << devname
                            << std::endl;
  }

  close(fd);

  fd = -1;
}

void ctapdev::enqueue(rofl::cpacket *pkt) {
  if (fd == -1) {
    cpacketpool::get_instance().release_pkt(pkt);
    return;
  }

  // store pkt in outgoing queue
  rofl::AcquireReadWriteLock rwlock(pout_queue_rwlock);
  pout_queue.push_back(pkt);

  // todo log enqueue

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
  rofl::AcquireReadWriteLock rwlock(pout_queue_rwlock);
  for (std::vector<rofl::cpacket *>::iterator it = pkts.begin();
       it != pkts.end(); ++it) {
    pout_queue.push_back(*it);
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
        throw eNetDevAgain(
            "ctapdev::handle_revent() EAGAIN when reading from /dev/net/tun");
      default:
        throw eNetDevCritical(
            "ctapdev::handle_revent() error when reading from /dev/net/tun");
      }
    } else {
      pkt = cpacketpool::get_instance().acquire_pkt();

      pkt->unpack(mem.somem(), rc);

      netdev_owner->enqueue(this, pkt);
    }

  } catch (ePacketPoolExhausted &e) {
    rofcore::logging::error << "ctapdev::handle_revent() packet pool "
                               "exhausted, no idle slots available"
                            << std::endl;

  } catch (eNetDevAgain &e) {

    rofcore::logging::error << "ctapdev::handle_revent() EAGAIN, retrying later"
                            << std::endl;

    if (pkt)
      cpacketpool::get_instance().release_pkt(pkt);

  } catch (eNetDevCritical &e) {
    rofcore::logging::error << "ctapdev::handle_revent() error occured"
                            << std::endl;

    if (pkt)
      cpacketpool::get_instance().release_pkt(pkt);

    delete this;
    return;
  }
}

void ctapdev::handle_write_event(rofl::cthread &thread, int fd) { tx(); }

void ctapdev::tx() {
  rofl::cpacket *pkt = NULL;
  try {
    rofl::AcquireReadWriteLock rwlock(pout_queue_rwlock);
    while (not pout_queue.empty()) {

      pkt = pout_queue.front();
      int rc = 0;
      if ((rc = write(fd, pkt->soframe(), pkt->length())) < 0) {
        switch (errno) {
        case EAGAIN:
          rofcore::logging::debug << "ctapdev::tx() EAGAIN" << std::endl;
          return;
        default:
          // throw eNetDevCritical("ctapdev::handle_wevent() error occured");
          rofcore::logging::error << "ctapdev::tx() unknown error occured"
                                  << std::endl;
          return;
        }
      }

      cpacketpool::get_instance().release_pkt(pkt);

      pout_queue.pop_front();
    }

  } catch (eNetDevAgain &e) {

    // keep fd in wfds
    rofcore::logging::error << "ctapdev::handle_wevent() EAGAIN, retrying later"
                            << std::endl;

  } catch (eNetDevCritical &e) {

    rofcore::logging::error << "ctapdev::handle_wevent() critical error occured"
                            << std::endl;
    cpacketpool::get_instance().release_pkt(pkt);

    throw;
    // delete this; return;
  }
}

void ctapdev::handle_timeout(rofl::cthread &thread, uint32_t timer_id,
                             const std::list<unsigned int> &ttypes) {
  switch (timer_id) {
  case CTAPDEV_TIMER_OPEN_PORT: {
    try {
      tap_open(hwaddr);
    } catch (...) {
      thread.add_timer(CTAPDEV_TIMER_OPEN_PORT, rofl::ctimespec().expire_in(1));
    }
  } break;
  }
}
