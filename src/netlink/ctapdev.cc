/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <fcntl.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <cerrno>

#include <glog/logging.h>

#include "ctapdev.h"
#include "tap_manager.h"

namespace basebox {

ctapdev::ctapdev(std::string const &devname, const rofl::caddress_ll &hwaddr)
    : fd(-1), devname(devname), hwaddr(hwaddr) {
  if (devname.size() >= IFNAMSIZ || devname.size() == 0) {
    throw std::length_error("invalid devname size");
  }
}

ctapdev::~ctapdev() { tap_close(); }

void ctapdev::tap_open() {
  struct ifreq ifr;
  int rc, carrier = 0;

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
  strncpy(ifr.ifr_name, devname.c_str(), IFNAMSIZ - 1);
  hwaddr.pack(reinterpret_cast<uint8_t *>(ifr.ifr_hwaddr.sa_data), ETH_ALEN);

  if ((rc = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0) {
    LOG(FATAL) << __FUNCTION__ << ": ioctl TUNSETIFF failed on fd=" << fd
               << " errno=" << errno << " reason: " << strerror(errno);
    close(fd);
    fd = -1;
  }

  if ((rc = ioctl(fd, TUNSETCARRIER, &carrier)) < 0) {
    LOG(ERROR) << __FUNCTION__ << ": ioctl TUNSETCARRIER failed on fd=" << fd
               << " errno=" << errno << " reason: " << strerror(errno);
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
