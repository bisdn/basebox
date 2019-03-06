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

#include "netlink-utils.h"

extern "C" {
struct rtnl_link_bridge_vlan;
struct rtnl_link;
struct rtnl_neigh;
struct nl_addr;
}

namespace rofl {
class caddress_ll;
}

namespace basebox {

class cnetlink;
class nl_vxlan;
class switch_interface;
class tap_manager;
struct packet;

class nl_bridge {
public:
  nl_bridge(switch_interface *sw, std::shared_ptr<tap_manager> tap_man,
            cnetlink *nl, std::shared_ptr<nl_vxlan> vxlan);

  virtual ~nl_bridge();

  void set_bridge_interface(rtnl_link *);
  bool is_bridge_interface(rtnl_link *);

  void add_interface(rtnl_link *);
  void update_interface(rtnl_link *old_link, rtnl_link *new_link);
  void delete_interface(rtnl_link *);

  void add_neigh_to_fdb(rtnl_neigh *);
  void remove_neigh_from_fdb(rtnl_neigh *);

  bool is_mac_in_l2_cache(rtnl_neigh *n);
  int learn_source_mac(rtnl_link *br_link, packet *p);
  int fdb_timeout(rtnl_link *br_link, uint16_t vid,
                  const rofl::caddress_ll &mac);
  int get_ifindex() { return rtnl_link_get_ifindex(bridge); }

  void get_bridge_ports(
      std::tuple<std::shared_ptr<tap_manager>, std::deque<rtnl_link *> *>
          &params, // XXX TODO make a struct here
      rtnl_link *br_port) noexcept;

  bool is_port_flooding(rtnl_link *br_link) const;

  int update_access_ports(rtnl_link *vxlan_link, rtnl_link *br_link,
                          const uint32_t tunnel_id, bool add);

private:
  void update_vlans(rtnl_link *, rtnl_link *);

  std::deque<rtnl_neigh *> get_fdb_entries_of_port(rtnl_link *br_port,
                                                   uint16_t vid);

  void update_access_ports(rtnl_link *vxlan_link, rtnl_link *br_link,
                           const uint16_t vid, const uint32_t tunnel_id,
                           const std::deque<rtnl_link *> &bridge_ports,
                           bool add);
  rtnl_link *bridge;
  switch_interface *sw;
  std::shared_ptr<tap_manager> tap_man;
  cnetlink *nl;
  std::shared_ptr<nl_vxlan> vxlan;
  std::unique_ptr<nl_cache, decltype(&nl_cache_free)> l2_cache;

  rtnl_link_bridge_vlan empty_br_vlan;
  uint32_t vxlan_dom_bitmap[RTNL_LINK_BRIDGE_VLAN_BITMAP_LEN];
  std::map<uint16_t, uint32_t> vxlan_domain;
};

} /* namespace basebox */
