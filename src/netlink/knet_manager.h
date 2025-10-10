// SPDX-FileCopyrightText: © 2022 BISDN GmbH
//
// SPDX-License-Identifier: MPL-2.0-no-copyleft-exception

#pragma once

#include <deque>
#include <string>
#include <map>
#include <memory>
#include <mutex>
#include <bitset>

#include "port_manager.h"
#include "sai.h"
#include <linux/ethtool.h>

extern "C" {
struct rtnl_link;
}

namespace basebox {

class knet_manager final : public port_manager {

public:
  knet_manager();
  ~knet_manager();

  int create_portdev(uint32_t port_id, const std::string &port_name,
                     const rofl::caddress_ll &hwaddr,
                     switch_callback &callback);

  int destroy_portdev(uint32_t port_id, const std::string &port_name);

  int enqueue(uint32_t port_id, basebox::packet *pkt);

  int change_port_status(const std::string name, bool status);
  int set_port_speed(const std::string name, uint32_t speed, uint8_t duplex);
  int set_offloaded(rtnl_link *link, bool offloaded);

  // access from northbound (cnetlink)
  bool portdev_removed(rtnl_link *link);
  bool portdev_ready(rtnl_link *link);
  int update_mtu(rtnl_link *link);

private:
  knet_manager(const knet_manager &other) = delete; // non construction-copyable
  knet_manager &operator=(const knet_manager &) = delete; // non copyable

  std::bitset<128> netif_ids_in_use;
  int get_next_netif_id();

  // only accessible from southbound
  std::map<uint32_t, uint32_t> knet_devs; // port id:netif_id
  std::deque<uint32_t> port_deleted;
};

} // namespace basebox
