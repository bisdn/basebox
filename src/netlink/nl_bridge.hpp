/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <cstdint>
#include <list>
#include <string>
#include <memory>

#include <netlink/route/link/bridge.h>

#include "netlink-utils.hpp"

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
class switch_interface;
class tap_manager;
struct packet;

class nl_bridge {
public:
  nl_bridge(switch_interface *sw, std::shared_ptr<tap_manager> tap_man,
            cnetlink *nl);
  virtual ~nl_bridge();

  void set_bridge_interface(rtnl_link *);
  bool is_bridge_interface(rtnl_link *);

  void add_interface(rtnl_link *);
  void update_interface(rtnl_link *old_link, rtnl_link *new_link);
  void delete_interface(rtnl_link *);

  void add_neigh_to_fdb(rtnl_neigh *);
  void remove_neigh_from_fdb(rtnl_neigh *);

  int learn_source_mac(rtnl_link *br_link, packet *p);
  int fdb_timeout(rtnl_link *br_link, uint16_t vid,
                  const rofl::caddress_ll &mac);
  int get_ifindex() { return rtnl_link_get_ifindex(bridge); }

private:
  void update_vlans(rtnl_link *, rtnl_link *);

  rtnl_link *bridge;
  switch_interface *sw;
  std::shared_ptr<tap_manager> tap_man;
  cnetlink *nl;
  rtnl_link_bridge_vlan empty_br_vlan;
  std::unique_ptr<nl_cache, decltype(&nl_cache_free)> l2_cache;
};

} /* namespace basebox */
