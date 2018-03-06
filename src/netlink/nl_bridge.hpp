/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <cstdint>
#include <deque>
#include <list>
#include <map>
#include <memory>
#include <string>

#include <netlink/route/link/bridge.h>

#include "netlink-utils.hpp"

extern "C" {
struct rtnl_link_bridge_vlan;
struct rtnl_link;
struct rtnl_neigh;
struct nl_addr;
}

namespace basebox {

class cnetlink;
class switch_interface;
class tap_manager;
class nl_vxlan;

class nl_bridge {
public:
  nl_bridge(switch_interface *sw, std::shared_ptr<tap_manager> tap_man,
            cnetlink *nl, std::shared_ptr<nl_vxlan> vxlan);

  virtual ~nl_bridge();

  void set_bridge_interface(rtnl_link *);

  bool has_bridge_interface() const { return bridge != nullptr; }

  void add_interface(rtnl_link *);

  void update_interface(rtnl_link *old_link, rtnl_link *new_link);

  void delete_interface(rtnl_link *);

  void add_neigh_to_fdb(rtnl_neigh *);
  void add_mac_to_fdb(rtnl_neigh *, rtnl_link *);

  void remove_mac_from_fdb(rtnl_neigh *);

  void get_bridge_ports(
      std::tuple<std::shared_ptr<tap_manager>, std::deque<rtnl_link *> *>
          &params, // XXX TODO make a struct here
      rtnl_link *br_port) noexcept;

private:
  void update_vlans(rtnl_link *, rtnl_link *);

  void update_access_ports(const int ifindex_vxlan, const uint16_t vid,
                           const uint32_t tunnel_id,
                           const std::deque<rtnl_link *> &bridge_ports,
                           bool add);
  rtnl_link *bridge;
  switch_interface *sw;

  std::shared_ptr<tap_manager> tap_man;
  cnetlink *nl;
  std::shared_ptr<nl_vxlan> vxlan;

  rtnl_link_bridge_vlan empty_br_vlan;
  uint32_t vxlan_dom_bitmap[RTNL_LINK_BRIDGE_VLAN_BITMAP_LEN];
  std::map<uint16_t, uint32_t> vxlan_domain;
};

} /* namespace basebox */
