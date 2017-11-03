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
#include "netlink/tap_manager.hpp"
#include "of-dpa/packetpool.hpp"

namespace basebox {

ctapdev::ctapdev(std::string const &devname) : fd(-1), devname(devname) {
  if (devname.size() > IFNAMSIZ || devname.size() == 0) {
    throw std::length_error("invalid devname size");
  }
}

ctapdev::~ctapdev() { tap_close(); }

void ctapdev::tap_open() {
  struct ifreq ifr;
  int rc;

  if (fd != -1) {
    VLOG(1) << __FUNCTION__ << ": tapdev is already open using fd=" << fd;
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
    LOG(FATAL) << __FUNCTION__ << ": ioctl TUNSETIFF failed on fd=" << fd
               << " errno=" << errno << " reason: " << strerror(errno);
    close(fd);
    fd = -1;
  }

  LOG(INFO) << __FUNCTION__ << ": created tapdev " << devname << " fd=" << fd
            << " tid=" << pthread_self();
}

void ctapdev::tap_close() {
  if (fd == -1) {
    return;
  }

  int rv = close(fd);
  if (rv < 0)
    LOG(ERROR) << __FUNCTION__ << ": failed to close fd=" << fd;

  fd = -1;

  LOG(INFO) << __FUNCTION__ << ": closed tapdev " << devname
            << " tid=" << pthread_self();
}

} // namespace basebox
