/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <cstdint>
#include <memory>
#include <deque>

#include "nl_l3_interfaces.h"

extern "C" {
struct nl_addr;
struct rtnl_addr;
struct rtnl_neigh;
struct rtnl_route;
struct rtnl_nexthop;
}

namespace rofl {
class caddress_ll;
class caddress_in4;
} // namespace rofl

namespace basebox {

class cnetlink;
class nl_vlan;
class switch_interface;

class nl_l3 {
public:
  nl_l3(std::shared_ptr<nl_vlan> vlan, cnetlink *nl);
  ~nl_l3() {}

  int init() noexcept;

  int add_l3_addr(struct rtnl_addr *a);
  int add_l3_addr_v6(struct rtnl_addr *a);
  int del_l3_addr(struct rtnl_addr *a);

  int add_lo_addr_v6(struct rtnl_addr *a);

  int add_l3_neigh(struct rtnl_neigh *n);
  int update_l3_neigh(struct rtnl_neigh *n_old, struct rtnl_neigh *n_new);
  int del_l3_neigh(struct rtnl_neigh *n);

  int add_l3_route(struct rtnl_route *r);
  int del_l3_route(struct rtnl_route *r);

  void get_nexthops_of_route(rtnl_route *route,
                             std::deque<struct rtnl_nexthop *> *nhs) noexcept;

  int get_neighbours_of_route(rtnl_route *r,
                              std::deque<struct rtnl_neigh *> *neighs,
                              std::deque<nh_stub> *unresolved_nh) noexcept;

  void register_switch_interface(switch_interface *sw);

  void notify_on_net_reachable(net_reachable *f, struct net_params p) noexcept;
  void notify_on_nh_reachable(nh_reachable *f, struct nh_params p) noexcept;

private:
  int add_l3_unicast_route(rtnl_route *r);
  int del_l3_unicast_route(rtnl_route *r);

  int add_l3_unicast_route_v6(rtnl_route *r);
  int del_l3_unicast_route_v6(rtnl_route *r);

  int add_l3_neigh_egress(struct rtnl_neigh *n, uint32_t *l3_interface_id);
  int del_l3_neigh_egress(struct rtnl_neigh *n);

  int add_l3_egress(int ifindex, const uint16_t vid,
                    const struct nl_addr *s_mac, const struct nl_addr *d_mac,
                    uint32_t *l3_interface_id);
  int del_l3_egress(int ifindex, uint16_t vid, const struct nl_addr *s_mac,
                    const struct nl_addr *d_mac);

  int add_l3_unicast_host(const rofl::caddress_in4 &ipv4_dst,
                          uint32_t l3_interface_id) const;

  bool is_link_local_address(const struct nl_addr *addr);

  switch_interface *sw;
  std::shared_ptr<nl_vlan> vlan;
  cnetlink *nl;
  std::deque<std::pair<net_reachable *, net_params>> net_callbacks;
  std::deque<std::pair<nh_reachable *, nh_params>> nh_callbacks;
};

} // namespace basebox
