/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <glog/logging.h>
#include <linux/if_tun.h>
#include <net/if.h>
#include <netlink/route/link.h>
#include <sys/ioctl.h>
#include <utility>
#include <linux/ethtool.h>
#include <linux/sockios.h>

#include "cnetlink.h"
#include "ctapdev.h"
#include "tap_io.h"
#include "tap_manager.h"

#define ETHTOOL_LINK_MODE_MASK_MAX_KERNEL_NU32 (SCHAR_MAX)
#define ETHTOOL_SPEED(speed) speed / 1000 // conversion to Mbit

namespace basebox {

tap_manager::tap_manager() : io(new tap_io()) {}

tap_manager::~tap_manager() {
  std::map<uint32_t, ctapdev *> ddevs;
  ddevs.swap(tap_devs);
  for (auto &dev : ddevs) {
    delete dev.second;
  }
}

int tap_manager::create_portdev(uint32_t port_id, const std::string &port_name,
                                const rofl::caddress_ll &hwaddr,
                                switch_callback &cb) {
  int r = 0;
  bool dev_exists = false;
  bool dev_name_exists = false;
  auto dev_it = tap_devs.find(port_id);

  if (dev_it != tap_devs.end())
    dev_exists = true;

  {
    std::lock_guard<std::mutex> lock{tn_mutex};
    auto dev_name_it = port_names2id.find(port_name);

    if (dev_name_it != port_names2id.end())
      dev_name_exists = true;
  }

  if (!dev_exists && !dev_name_exists) {
    // create a new tap device
    ctapdev *dev;

    try {
      int fd = -1;

      dev = new ctapdev(port_name, hwaddr);
      tap_devs.insert(std::make_pair(port_id, dev));
      {
        std::lock_guard<std::mutex> lock{tn_mutex};
        auto rv = port_names2id.emplace(std::make_pair(port_name, port_id));

        if (!rv.second) {
          LOG(FATAL) << __FUNCTION__ << ": failed to insert";
        }

        dev->tap_open();
        fd = dev->get_fd();
        tap_names2fds.emplace(std::make_pair(port_name, fd));
      }

      LOG(INFO) << __FUNCTION__
                << ": created device having the following details: port_id="
                << port_id << " portname=" << port_name << " fd=" << fd
                << " ptr=" << dev;

      // start reading from port
      tap_io::tap_io_details td(fd, port_id, &cb, 1500);
      io->register_tap(td);
    } catch (std::exception &e) {
      LOG(ERROR) << __FUNCTION__ << ": failed to create tapdev " << port_name;
      r = -EINVAL;
    }
  } else {
    VLOG(1) << __FUNCTION__ << ": " << port_name << " with port_id=" << port_id
            << " already existing";
  }

  return r;
}

int tap_manager::destroy_portdev(uint32_t port_id,
                                 const std::string &port_name) {
  auto it = tap_devs.find(port_id);
  if (it == tap_devs.end()) {
    LOG(WARNING) << __FUNCTION__ << ": called for invalid port_id=" << port_id
                 << " port_name=" << port_name;
    return 0;
  }

  // drop port from name mapping
  std::lock_guard<std::mutex> lock{tn_mutex};
  port_deleted.push_back(port_id);
  auto tap_names_it = port_names2id.find(port_name);

  if (tap_names_it != port_names2id.end()) {
    port_names2id.erase(tap_names_it);
  }

  auto tap_names2fd_it = tap_names2fds.find(port_name);

  if (tap_names2fd_it != tap_names2fds.end()) {
    tap_names2fds.erase(tap_names2fd_it);
  }

  // drop port from port mapping
  auto dev = it->second;
  int fd = dev->get_fd();
  tap_devs.erase(it);
  delete dev;

  io->unregister_tap(fd);

  return 0;
}

int tap_manager::enqueue(uint32_t port_id, basebox::packet *pkt) {
  int fd = get_fd(port_id);
  if (fd < 0) {
    std::free(pkt);
    return 0;
  }

  VLOG(3) << __FUNCTION__ << ": send pkt " << pkt << " to tap on fd=" << fd;

  try {
    io->enqueue(fd, pkt);
  } catch (std::exception &e) {
    LOG(ERROR) << __FUNCTION__ << ": failed to enqueue packet " << pkt
               << " to fd=" << fd;
    std::free(pkt);
  }
  return 0;
}

int tap_manager::get_fd(uint32_t port_id) const noexcept {
  // XXX TODO add assert wrt threading
  auto it = tap_devs.find(port_id);

  if (it == tap_devs.end()) {
    return -EINVAL;
  }

  return it->second->get_fd();
}

bool tap_manager::portdev_ready(rtnl_link *link) {
  assert(link);

  enum link_type lt = get_link_type(link);
  if (lt != LT_TUN) {
    VLOG(2) << __FUNCTION__ << ": ignore creation of device of type lt=" << lt;
    return false;
  }

  // already registered?
  int ifindex = rtnl_link_get_ifindex(link);
  auto it = ifindex_to_id.find(ifindex);
  if (it != ifindex_to_id.end()) {
    LOG(ERROR) << __FUNCTION__ << ": already registered port "
               << rtnl_link_get_name(link);
    return false;
  }

  {
    std::string name(rtnl_link_get_name(link));
    std::lock_guard<std::mutex> lock{tn_mutex};
    auto tn_it = port_names2id.find(name);
    if (tn_it == port_names2id.end()) {
      LOG(WARNING) << __FUNCTION__ << "invalid port name " << name;
      return false;
    }

    // update maps
    auto id2ifi_it =
        id_to_ifindex.insert(std::make_pair(tn_it->second, ifindex));
    if (!id2ifi_it.second && id2ifi_it.first->second != ifindex) {
      // update only if the ifindex has changed
      LOG(WARNING) << __FUNCTION__
                   << ": enforced update of id:ifindex mapping id="
                   << id2ifi_it.first->first
                   << " ifindex(old)=" << id2ifi_it.first->second
                   << " ifindex(new)=" << ifindex;

      // remove overwritten index in ifindex_to_id map
      auto it = ifindex_to_id.find(id2ifi_it.first->second);
      if (it != ifindex_to_id.end()) {
        ifindex_to_id.erase(it);
      }

      // update the old one
      id2ifi_it.first->second = ifindex;
    }

    auto rv1 = ifindex_to_id.insert(std::make_pair(ifindex, tn_it->second));
    if (!rv1.second && rv1.first->second != tn_it->second) {
      // update only if the id has changed
      LOG(WARNING) << __FUNCTION__
                   << ": enforced update of ifindex:id mapping ifindex="
                   << ifindex << " id(old)=" << rv1.first->second
                   << " id(new)=" << tn_it->second;
      rv1.first->second = tn_it->second;
    }
  }

  update_mtu(link);

  return true;
}

int tap_manager::update_mtu(rtnl_link *link) {
  assert(link);

  std::lock_guard<std::mutex> lock{tn_mutex};
  auto fd_it = tap_names2fds.find(std::string(rtnl_link_get_name(link)));
  if (fd_it == tap_names2fds.end()) {
    LOG(ERROR) << __FUNCTION__ << ": tap_dev not found";
    return -EINVAL;
  }

  if (fd_it->second == -1) {
    LOG(FATAL) << __FUNCTION__ << ": need to update fd";
  }

  io->update_mtu(fd_it->second, rtnl_link_get_mtu(link));
  return 0;
}

bool tap_manager::portdev_removed(rtnl_link *link) {
  assert(link);

  int ifindex(rtnl_link_get_ifindex(link));
  std::string portname(rtnl_link_get_name(link));
  enum link_type lt = get_link_type(link);
  bool port_removed(false);

  if (lt != LT_TUN) {
    VLOG(2) << __FUNCTION__ << ": ignore removal of device of type lt=" << lt;
    return false;
  }

  std::lock_guard<std::mutex> lock{tn_mutex};

  auto ifi2id_it = ifindex_to_id.find(ifindex);
  if (ifi2id_it == ifindex_to_id.end()) {
    VLOG(2) << __FUNCTION__
            << ": ignore removal of tap device with ifindex=" << ifindex;
    return false;
  }

  // check if this port was scheduled for deletion
  auto pd_it =
      std::find(port_deleted.begin(), port_deleted.end(), ifi2id_it->second);
  if (pd_it == port_deleted.end()) {
    auto pn_it = tap_devs.find(ifi2id_it->second);
    LOG(ERROR) << __FUNCTION__ << ": illegal removal of port "
               << pn_it->second->get_devname() << " with ifindex=" << ifindex;

    port_removed = true;
  }

  auto id2ifi_it = id_to_ifindex.find(ifi2id_it->second);
  if (id2ifi_it == id_to_ifindex.end()) {
    auto pn_it = tap_devs.find(ifi2id_it->second);
    LOG(ERROR) << __FUNCTION__ << ": illegal removal of port "
               << pn_it->second->get_devname() << " with ifindex=" << ifindex;
    port_removed = true;
  }

  if (port_removed) {
    auto pn_it = tap_devs.find(ifi2id_it->second);
    recreate_tapdev(ifindex, portname, pn_it->second->get_hwaddr());
  }

  ifindex_to_id.erase(ifi2id_it);
  if (id2ifi_it != id_to_ifindex.end())
    id_to_ifindex.erase(id2ifi_it);
  if (pd_it != port_deleted.end())
    port_deleted.erase(pd_it);

  return true;
}

int tap_manager::recreate_tapdev(int ifindex, const std::string &portname,
                                 const rofl::caddress_ll &hwaddr) {
  int rv = 0;

  VLOG(1) << __FUNCTION__ << ": recreating port " << portname
          << " with ifindex " << ifindex;
  ctapdev *dev;

  try {
    dev = new ctapdev(portname, hwaddr);
    auto id = ifindex_to_id.find(ifindex);

    tap_devs.erase(id->second);

    // create the port
    dev->tap_open();
    tap_devs.emplace(std::make_pair(id->second, dev));

    int fd = dev->get_fd();

    LOG(INFO) << __FUNCTION__ << ": port_id=" << ifindex
              << " portname=" << portname << " fd=" << fd << " ptr=" << dev;

    tap_names2fds.emplace(std::make_pair(portname, fd));
    port_names2id.emplace(std::make_pair(portname, id->second));

  } catch (std::exception &e) {
    LOG(ERROR) << __FUNCTION__ << ": failed to create tapdev " << portname;
    rv = -EINVAL;
  }

  return rv;
}

/*
 * Adds a function to set port state according to link state
 * Status is determined via the portstatus/port_desc_reply message
 * This function sets the state using ioctl since netlink apparently cannot
 * handle setting these parameters
 *
 * @param name port name to be changed
 * @param status interface status: 1=up / 0=down
 * @return ioctl value from setting the flags
 */
int tap_manager::change_port_status(const std::string name, bool status) {
  std::lock_guard<std::mutex> lock{tn_mutex};
  auto fd_it = tap_names2fds.find(name);
  int error;
  int carrier = status;

  if (fd_it == tap_names2fds.end()) {
    LOG(ERROR) << __FUNCTION__ << ": tap_dev not found";
    return -EINVAL;
  }

  if (fd_it->second == -1) {
    LOG(FATAL) << __FUNCTION__ << ": need to update fd";
    return -EINVAL;
  }

  // Set flags
  error = ioctl(fd_it->second, TUNSETCARRIER, &carrier);
  if (error) {
    LOG(ERROR) << __FUNCTION__ << ": ioctl failed with error code " << error;
  }

  return error;
}

int tap_manager::set_port_speed(const std::string name, uint32_t speed,
                                uint8_t duplex) {
  LOG(INFO) << __FUNCTION__ << ": changing port= " << name
            << " speed=" << ETHTOOL_SPEED(speed);
  struct ifreq ifr = {};

  auto sockFd = socket(AF_INET, SOCK_DGRAM, 0);
  strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ);

  // The ethtool API requires an handshake via ETHTOOL_GLINKSETTINGS
  // before getting the "real" value to agree on the length of link
  // mode bitmaps.

  struct {
    struct ethtool_link_settings req;
    __u32 link_mode_data[3 * ETHTOOL_LINK_MODE_MASK_MAX_KERNEL_NU32];
  } link_settings;

  memset(&link_settings, 0, sizeof(link_settings));
  link_settings.req.cmd = ETHTOOL_GLINKSETTINGS;

  ifr.ifr_data = reinterpret_cast<char *>(&link_settings);
  int error = ioctl(sockFd, SIOCETHTOOL, static_cast<void *>(&ifr));
  if (error < 0) {
    LOG(ERROR) << __FUNCTION__ << " handshake failed error=" << error;
    close(sockFd);
    return error;
  }

  if (link_settings.req.link_mode_masks_nwords >= 0 ||
      link_settings.req.cmd != ETHTOOL_GLINKSETTINGS) {
    close(sockFd);
    return -EOPNOTSUPP;
  }

  link_settings.req.link_mode_masks_nwords =
      -link_settings.req.link_mode_masks_nwords;

  ifr.ifr_data = reinterpret_cast<char *>(&link_settings);
  error = ioctl(sockFd, SIOCETHTOOL, static_cast<void *>(&ifr));
  if (error < 0) {
    LOG(ERROR) << __FUNCTION__ << " failed to get port= " << name
               << " error=" << error;
    close(sockFd);
    return error;
  }

  link_settings.req.duplex = duplex;
  link_settings.req.speed = ETHTOOL_SPEED(speed);
  link_settings.req.cmd = ETHTOOL_SLINKSETTINGS;

  ifr.ifr_data = reinterpret_cast<char *>(&link_settings);
  error = ioctl(sockFd, SIOCETHTOOL, static_cast<void *>(&ifr));
  if (error < 0) {
    LOG(ERROR) << __FUNCTION__ << " failed to set port= " << name
               << " speed, error=" << error;
  }

  close(sockFd);
  return error;
}

} // namespace basebox
