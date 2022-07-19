/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <glog/logging.h>
#include <linux/sockios.h>
#include <netlink/route/link.h>

#include <cassert>
#include <fstream>

#include "cnetlink.h"
#include "knet_manager.h"

#define ETHTOOL_SPEED(speed) speed / 1000 // conversion to Mbit

namespace basebox {

knet_manager::knet_manager() {}

knet_manager::~knet_manager() {
  std::map<uint32_t, uint32_t> ddevs;
  ddevs.swap(knet_devs);
  for (auto dev : ddevs) {
    uint32_t netif_id = dev.second;

    int ret = system(("/usr/sbin/client_drivshell knet filter destroy " +
                      std::to_string(netif_id + 1))
                         .c_str());
    if (!WIFEXITED(ret) || WEXITSTATUS(ret) != 0)
      LOG(WARNING) << __FUNCTION__ << ": failed to remove filter with id "
                   << netif_id + 1;
    ret = system(("/usr/sbin/client_drivshell knet netif destroy " +
                  std::to_string(netif_id))
                     .c_str());
    if (!WIFEXITED(ret) || WEXITSTATUS(ret) != 0)
      LOG(WARNING) << __FUNCTION__ << ": failed to remove knet netif with id "
                   << netif_id;
  }
}

int knet_manager::get_next_netif_id(void) {
  for (int i = 1; i < 128; i++)
    if (!netif_ids_in_use[i])
      return i;

  return -ENOSPC;
}

int knet_manager::create_portdev(uint32_t port_id, const std::string &port_name,
                                 const rofl::caddress_ll &hwaddr,
                                 switch_callback &cb) {
  int r = 0;
  bool dev_exists = false;
  bool dev_name_exists = false;
  auto dev_it = knet_devs.find(port_id);

  if (dev_it != knet_devs.end())
    dev_exists = true;

  {
    std::lock_guard<std::mutex> lock{tn_mutex};
    auto dev_name_it = port_names2id.find(port_name);

    if (dev_name_it != port_names2id.end())
      dev_name_exists = true;
  }

  if (!dev_exists && !dev_name_exists) {
    // create a new tap device
    int netif_id;
    char mac_string[18];
    sprintf(mac_string, "%02x:%02x:%02x:%02x:%02x:%02x", hwaddr[0], hwaddr[1],
            hwaddr[2], hwaddr[3], hwaddr[4], hwaddr[5]);
    try {
      {
        std::lock_guard<std::mutex> lock{tn_mutex};

        netif_id = get_next_netif_id();
        if (netif_id < 0)
          return netif_id;

        knet_devs.insert(std::make_pair(port_id, netif_id));
        auto rv = port_names2id.emplace(std::make_pair(port_name, port_id));

        if (!rv.second) {
          LOG(FATAL) << __FUNCTION__ << ": failed to insert";
        }
        r = system(("/usr/sbin/client_drivshell knet netif create port=" +
                    std::to_string(port_id) + " ifname=" + port_name +
                    " mac=" + mac_string + " keeprxtag=yes")
                       .c_str());
        if (!WIFEXITED(r) || WEXITSTATUS(r) != 0)
          LOG(FATAL) << __FUNCTION__
                     << ": failed to create knet interface for port_id "
                     << port_id;
        r = system(
            ("/usr/sbin/client_drivshell knet filter create dt=netif did=" +
             std::to_string(netif_id) + " ip=" + std::to_string(port_id))
                .c_str());
        if (!WIFEXITED(r) || WEXITSTATUS(r) != 0)
          LOG(FATAL) << __FUNCTION__
                     << ": failed to create knet filter for port_id "
                     << port_id;
        netif_ids_in_use[netif_id] = true;
        change_port_status(port_name, false);
      }

      LOG(INFO) << __FUNCTION__
                << ": created device having the following details: port_id="
                << port_id << " portname=" << port_name
                << " netif_id=" << netif_id;

    } catch (std::exception &e) {
      LOG(ERROR) << __FUNCTION__ << ": failed to create portdev " << port_name;
      r = -EINVAL;
    }
  } else {
    VLOG(1) << __FUNCTION__ << ": " << port_name << " with port_id=" << port_id
            << " already existing";
  }

  return r;
}

int knet_manager::destroy_portdev(uint32_t port_id,
                                  const std::string &port_name) {
  int ret = 0;
  auto it = knet_devs.find(port_id);
  if (it == knet_devs.end()) {
    LOG(WARNING) << __FUNCTION__ << ": called for invalid port_id=" << port_id
                 << " port_name=" << port_name;
    return 0;
  }

  uint32_t netif_id = it->second;

  // drop port from name mapping
  std::lock_guard<std::mutex> lock{tn_mutex};
  port_deleted.push_back(port_id);
  auto port_names_it = port_names2id.find(port_name);

  if (port_names_it != port_names2id.end()) {
    port_names2id.erase(port_names_it);
  }

  // filter id 1 is the RxAPI, so filter API ids are always one off
  ret = system(("/usr/sbin/client_drivshell knet filter destroy " +
                std::to_string(netif_id + 1))
                   .c_str());
  if (!WIFEXITED(ret) || WEXITSTATUS(ret) != 0)
    LOG(WARNING) << __FUNCTION__ << ": failed to remove filter with id "
                 << netif_id + 1;
  ret = system(("/usr/sbin/client_drivshell knet netif destroy " +
                std::to_string(netif_id))
                   .c_str());
  if (!WIFEXITED(ret) || WEXITSTATUS(ret) != 0)
    LOG(WARNING) << __FUNCTION__ << ": failed to remove knet netif with id "
                 << netif_id;

  netif_ids_in_use[netif_id] = false;
  knet_devs.erase(it);

  return 0;
}

int knet_manager::enqueue(uint32_t port_id, basebox::packet *pkt) {
  std::free(pkt);

  return 0;
}

bool knet_manager::portdev_ready(rtnl_link *link) {
  assert(link);

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
      VLOG(2) << __FUNCTION__ << "ignoring unxpected device " << name;
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

int knet_manager::update_mtu(rtnl_link *link) {
  assert(link);

  return 0;
}

bool knet_manager::portdev_removed(rtnl_link *link) {
  assert(link);

  int ifindex(rtnl_link_get_ifindex(link));
  std::string portname(rtnl_link_get_name(link));
  int rv = 0;
  bool port_removed(false);
  std::lock_guard<std::mutex> lock{tn_mutex};

  auto ifi2id_it = ifindex_to_id.find(ifindex);
  if (ifi2id_it == ifindex_to_id.end()) {
    VLOG(2) << __FUNCTION__
            << ": ignore removal of device with ifindex=" << ifindex;
    return false;
  }

  // check if this port was scheduled for deletion
  auto pd_it =
      std::find(port_deleted.begin(), port_deleted.end(), ifi2id_it->second);
  if (pd_it == port_deleted.end()) {
    LOG(FATAL) << __FUNCTION__ << ": unexpected port removal of "
               << OBJ_CAST(link);
  }

  auto id2ifi_it = id_to_ifindex.find(ifi2id_it->second);
  if (id2ifi_it == id_to_ifindex.end()) {
    LOG(FATAL) << __FUNCTION__ << ": unexpected port removal of "
               << OBJ_CAST(link);
  }

  ifindex_to_id.erase(ifi2id_it);
  id_to_ifindex.erase(id2ifi_it);
  port_deleted.erase(pd_it);

  return true;
}

/*
 * set netif link state according to open flow link state
 * Status is determined via the portstatus/port_desc_reply message
 *
 * @param name port name to be changed
 * @param status interface status: true=up / false=down
 * @return 0 on success
 */
int knet_manager::change_port_status(const std::string name, bool status) {
  std::ofstream file("/proc/bcm/knet/link");

  if (file.is_open()) {
    file << (name + "=" + (status ? "up" : "down"));
    file.close();
    return 0;
  }

  return 1;
}

/*
 * set netlif link speed according to open flow link speed
 * Status is determined via the portstatus/port_desc_reply message
 *
 * @param name port name to be changed
 * @param speed speed in kbit/s
 * @param duplex true=full duplex / false=half duplex
 * @return 0 on success
 */
int knet_manager::set_port_speed(const std::string name, uint32_t speed,
                                 uint8_t duplex) {
  std::ofstream file("/proc/bcm/knet/link");

  if (file.is_open()) {
    file << (name + "=" + std::to_string(ETHTOOL_SPEED(speed)) + "," +
             (duplex ? "fd" : "hd"));
    file.close();
    return 0;
  }
  return 1;
}

} // namespace basebox
