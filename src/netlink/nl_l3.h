// SPDX-FileCopyrightText: © 2016 BISDN GmbH
// SPDX-License-Identifier: MPL-2.0-no-copyleft-exception

#pragma once

#include <cstdint>
#include <deque>
#include <list>
#include <memory>
#include <set>

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
class nl_bridge;
class switch_interface;

class l3_prefix_comp final {
public:
  bool operator()(const nh_stub &nh, const struct nl_addr *addr) const {
    return nl_addr_cmp_prefix(nh.nh, addr) < 0;
  }

  bool operator()(const struct nl_addr *addr, const nh_stub &nh) const {
    return nl_addr_cmp_prefix(addr, nh.nh) < 0;
  }
};

class nl_l3 : public nh_reachable, public nh_unreachable {
public:
  nl_l3(std::shared_ptr<nl_vlan> vlan, cnetlink *nl);
  ~nl_l3();

  int init() noexcept;

  int add_l3_addr(struct rtnl_addr *a);
  int add_l3_addr_v6(struct rtnl_addr *a);
  int del_l3_addr(struct rtnl_addr *a);

  int add_lo_addr_v6(struct rtnl_addr *a);

  int add_l3_neigh(struct rtnl_neigh *n);
  int update_l3_neigh(struct rtnl_neigh *n_old, struct rtnl_neigh *n_new);
  int del_l3_neigh(struct rtnl_neigh *n);

  int get_l3_addrs(struct rtnl_link *link, std::deque<rtnl_addr *> *addresses,
                   int af = AF_UNSPEC);

  int add_l3_route(struct rtnl_route *r);
  int update_l3_route(struct rtnl_route *r_old, struct rtnl_route *r_new);
  int del_l3_route(struct rtnl_route *r);

  int get_l3_routes(struct rtnl_link *link, std::deque<rtnl_route *> *routes);

  int update_l3_termination(int port_id, uint16_t vid, struct nl_addr *old_mac,
                            struct nl_addr *new_mac) noexcept;
  int update_l3_egress(int port_id, uint16_t vid, struct nl_addr *old_mac,
                       struct nl_addr *new_mac) noexcept;

  void get_nexthops_of_route(rtnl_route *route,
                             std::deque<struct rtnl_nexthop *> *nhs) noexcept;

  int get_neighbours_of_route(rtnl_route *r, std::set<nh_stub> *nhs) noexcept;

  void register_switch_interface(switch_interface *sw);

  void notify_on_net_reachable(net_reachable *f, struct net_params p) noexcept;
  void notify_on_nh_reachable(nh_reachable *f, struct nh_params p) noexcept;
  void notify_on_nh_unreachable(nh_unreachable *f, struct nh_params p) noexcept;

  void nh_reachable_notification(struct rtnl_neigh *,
                                 struct nh_params) noexcept override;
  void nh_unreachable_notification(struct rtnl_neigh *,
                                   struct nh_params) noexcept override;

private:
  int get_l3_interface_id(int ifindex, const struct nl_addr *s_mac,
                          const struct nl_addr *d_mac,
                          uint32_t *l3_interface_id, uint16_t vid = 0);
  int get_l3_interface_id(const nh_stub &nh, uint32_t *l3_interface_id);

  int add_l3_termination(uint32_t port_id, uint16_t vid,
                         const rofl::caddress_ll &mac, int af) noexcept;
  int del_l3_termination(uint32_t port_id, uint16_t vid,
                         const rofl::caddress_ll &mac, int af) noexcept;

  int add_l3_unicast_route(rtnl_route *r, bool update_route);
  int update_l3_unicast_route(rtnl_route *r_old, rtnl_route *r_new);
  int del_l3_unicast_route(rtnl_route *r, bool keep_route);

  int add_l3_unicast_route(nl_addr *rt_dst, uint32_t l3_interface_id,
                           bool is_ecmp, bool update_route,
                           uint16_t vrf_id = 0);
  int del_l3_unicast_route(nl_addr *rt_dst, uint16_t vrf_id = 0);

  int add_l3_neigh_egress(struct rtnl_neigh *n, uint32_t *l3_interface_id,
                          uint16_t *vrf_id = nullptr);
  int del_l3_neigh_egress(struct rtnl_neigh *n);

  int add_l3_egress(int ifindex, const uint16_t vid,
                    const struct nl_addr *s_mac, const struct nl_addr *d_mac,
                    uint32_t *l3_interface_id);
  int del_l3_egress(int ifindex, uint16_t vid, const struct nl_addr *s_mac,
                    const struct nl_addr *d_mac);
  int search_neigh_cache(int ifindex, struct nl_addr *addr, int family,
                         std::list<struct rtnl_neigh *> *neigh);

  int add_l3_ecmp_group(const std::set<nh_stub> &nhs, uint32_t *l3_ecmp_id);
  int get_l3_ecmp_group(const std::set<nh_stub> &nhs, uint32_t *l3_ecmp_id);
  int del_l3_ecmp_group(const std::set<nh_stub> &nhs);

  bool is_ipv6_link_local_address(const struct nl_addr *addr) {
    return !nl_addr_cmp_prefix(ipv6_ll, addr);
  }

  bool is_ipv6_multicast_address(const struct nl_addr *addr) {
    return !nl_addr_cmp_prefix(ipv6_mc, addr);
  }

  bool is_l3_neigh_routable(struct rtnl_neigh *n);

  std::deque<nh_stub> get_l3_neighs_of_prefix(std::set<nh_stub> &list,
                                              nl_addr *prefix);

  switch_interface *sw;
  std::shared_ptr<nl_vlan> vlan;
  cnetlink *nl;
  std::list<std::pair<net_reachable *, net_params>> net_callbacks;
  std::list<std::pair<nh_reachable *, nh_params>> nh_callbacks;
  std::list<std::pair<nh_unreachable *, nh_params>> nh_unreach_callbacks;

  std::set<nh_stub> routable_l3_neighs;
  std::set<nh_stub> unroutable_l3_neighs;
  struct l3_prefix_comp l3_prefix_comp;

  const uint8_t MAIN_ROUTING_TABLE = 254;

  nl_addr *ipv4_lo;
  nl_addr *ipv6_lo;
  nl_addr *ipv6_ll;
  nl_addr *ipv6_mc;
};

} // namespace basebox
