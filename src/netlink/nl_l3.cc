/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <net/if.h>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <utility>

#include <glog/logging.h>
#include <netlink/route/addr.h>
#include <netlink/route/link.h>
#include <netlink/route/link/vlan.h>
#include <netlink/route/link/vrf.h>
#include <netlink/route/neighbour.h>
#include <netlink/route/route.h>
#include <netlink/route/nexthop.h>
#include <netlink/cache.h>

#include "cnetlink.h"
#include "nl_hashing.h"
#include "nl_l3.h"
#include "nl_output.h"
#include "nl_vlan.h"
#include "nl_route_query.h"
#include "sai.h"
#include "utils/rofl-utils.h"

namespace std {

template <> struct hash<rofl::caddress_ll> {
  using argument_type = rofl::caddress_ll;
  using result_type = std::size_t;
  result_type operator()(argument_type const &lla) const noexcept {
    return std::hash<uint64_t>{}(lla.get_mac());
  }
};

template <class T, class A> struct hash<std::set<T, A>> {
  using argument_type = std::set<T, A>;
  using result_type = std::size_t;
  result_type operator()(argument_type const &arg) const noexcept {
    result_type seed = 0;
    for (const auto v : arg) {
      hash_combine(seed, v);
    }
    return seed;
  }
};

} // namespace std

namespace basebox {

class l3_interface final {
public:
  l3_interface(uint32_t l3_interface_id)
      : l3_interface_id(l3_interface_id), refcnt(1) {}

  uint32_t l3_interface_id;
  int refcnt;
};

// next hop mapping key: <port_id, vid, src_mac, dst_mac>
std::unordered_map<
    std::tuple<int, uint16_t, rofl::caddress_ll, rofl::caddress_ll>,
    l3_interface>
    l3_interface_mapping;

// key: source port_id, vid, src_mac, ethertype ; value: refcount
std::unordered_multimap<std::tuple<int, uint16_t, rofl::caddress_ll, uint16_t>,
                        int>
    termination_mac_mapping;

// ECMP mapping
std::unordered_multimap<std::set<uint32_t>, l3_interface> l3_ecmp_mapping;

nl_l3::nl_l3(std::shared_ptr<nl_vlan> vlan, cnetlink *nl)
    : sw(nullptr), vlan(std::move(vlan)), nl(nl) {}

rofl::caddress_ll libnl_lladdr_2_rofl(const struct nl_addr *lladdr) {
  // XXX check for family
  return rofl::caddress_ll((uint8_t *)nl_addr_get_binary_addr(lladdr),
                           nl_addr_get_len(lladdr));
}

rofl::caddress_in4 libnl_in4addr_2_rofl(struct nl_addr *addr, int *rv) {
  struct sockaddr_in sin;
  socklen_t salen = sizeof(sin);

  assert(rv);
  *rv = nl_addr_fill_sockaddr(addr, (struct sockaddr *)&sin, &salen);
  return rofl::caddress_in4(&sin, salen);
}

rofl::caddress_in6 libnl_in6addr_2_rofl(struct nl_addr *addr, int *rv) {
  struct sockaddr_in6 sin;
  socklen_t salen = sizeof(sin);

  assert(rv);
  *rv = nl_addr_fill_sockaddr(addr, (struct sockaddr *)&sin, &salen);
  return rofl::caddress_in6(&sin, salen);
}

int nl_l3::init() noexcept {
  std::list<struct rtnl_addr *> lo_addr;
  std::unique_ptr<struct rtnl_addr, void (*)(rtnl_addr *)> addr_filter(
      rtnl_addr_alloc(), &rtnl_addr_put);

  rtnl_addr_set_ifindex(addr_filter.get(), 1);
  rtnl_addr_set_family(addr_filter.get(), AF_INET);

  nl_cache_foreach_filter(
      nl->get_cache(cnetlink::NL_ADDR_CACHE), OBJ_CAST(addr_filter.get()),
      [](struct nl_object *obj, void *arg) {
        VLOG(3) << __FUNCTION__ << " : found configured loopback " << obj;

        auto *add_list = static_cast<std::list<struct rtnl_addr *> *>(arg);

        add_list->emplace_back(ADDR_CAST(obj));
      },
      &lo_addr);

  for (auto addr : lo_addr) {
    if (add_l3_addr(addr) < 0)
      return -EINVAL;
  }

  return 0;
}

// searches the neigh cache for an address on one interface
int nl_l3::search_neigh_cache(int ifindex, struct nl_addr *addr, int family,
                              std::list<struct rtnl_neigh *> *neigh) {
  std::unique_ptr<struct rtnl_neigh, void (*)(rtnl_neigh *)> neigh_filter(
      rtnl_neigh_alloc(), &rtnl_neigh_put);

  rtnl_neigh_set_ifindex(neigh_filter.get(), ifindex);
  rtnl_neigh_set_dst(neigh_filter.get(), addr);
  rtnl_neigh_set_family(neigh_filter.get(), family);

  nl_cache_foreach_filter(
      nl->get_cache(cnetlink::NL_NEIGH_CACHE), OBJ_CAST(neigh_filter.get()),
      [](struct nl_object *obj, void *arg) {
        auto *add_list = static_cast<std::list<struct rtnl_neigh *> *>(arg);

        add_list->emplace_back(NEIGH_CAST(obj));
      },
      neigh);

  return 0;
}

// XXX separate function to make it possible to add lo addresses more directly
int nl_l3::add_l3_addr(struct rtnl_addr *a) {
  assert(sw);
  assert(a);

  int rv;

  if (a == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": addr can't be null";
    return -EINVAL;
  }

  struct rtnl_link *link = rtnl_addr_get_link(a);
  if (link == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": no link for addr a=" << OBJ_CAST(a);
    return -EINVAL;
  }

  bool is_loopback = (rtnl_link_get_flags(link) & IFF_LOOPBACK);
  bool is_bridge = rtnl_link_is_bridge(link); // XXX TODO svi as well?
  int ifindex = 0;
  uint16_t vid = vlan->get_vid(link);
  uint32_t vrf_id = vlan->get_vrf_id(vid, link);

  // checks if the bridge is the configured one
  if (is_bridge && !nl->is_bridge_configured(link)) {
    VLOG(1) << __FUNCTION__ << ": ignoring " << OBJ_CAST(link);
    return -EINVAL;
  }

  // checks if the bridge is already configured with an address
  int master_id = rtnl_link_get_master(link);
  if (master_id && is_bridge &&
      rtnl_link_get_addr(nl->get_link(master_id, AF_BRIDGE))) {
    VLOG(1) << __FUNCTION__ << ": ignoring address on " << OBJ_CAST(link);
    return -EINVAL;
  }

  // XXX TODO split this into several functions
  if (!is_loopback) {
    ifindex = rtnl_addr_get_ifindex(a);
    int port_id = nl->get_port_id(link);

    if (port_id == 0) {
      if (is_bridge or nl->is_bridge_interface(link)) {
        LOG(INFO) << __FUNCTION__ << ": address on top of bridge";
        port_id = 0;
      } else {
        LOG(ERROR) << __FUNCTION__ << ": invalid port_id 0 for link "
                   << OBJ_CAST(link);
        return -EINVAL;
      }
    }

    auto addr = rtnl_link_get_addr(link);
    rofl::caddress_ll mac = libnl_lladdr_2_rofl(addr);

    rv = add_l3_termination(port_id, vid, mac, AF_INET);
    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__
                 << ": failed to setup termination mac port_id=" << port_id
                 << ", vid=" << vid << " mac=" << mac << "; rv=" << rv;
      return rv;
    }
  }

  // get v4 dst (local v4 addr)
  auto prefixlen = rtnl_addr_get_prefixlen(a);
  auto addr = rtnl_addr_get_local(a);
  rofl::caddress_in4 ipv4_dst = libnl_in4addr_2_rofl(addr, &rv);
  rofl::caddress_in4 mask = rofl::build_mask_in4(prefixlen);

  if (rv < 0) {
    // TODO shall we remove the l3_termination mac?
    LOG(ERROR) << __FUNCTION__ << ": could not parse addr " << addr;
    return rv;
  }

  if (is_loopback) {
    auto p = nl_addr_alloc(255);
    nl_addr_parse("127.0.0.0/8", AF_INET, &p);
    std::unique_ptr<nl_addr, decltype(&nl_addr_put)> lo_addr(p, nl_addr_put);

    if (!nl_addr_cmp_prefix(addr, lo_addr.get())) {
      VLOG(3) << __FUNCTION__ << ": skipping 127.0.0.0/8";
      return 0;
    }

    if (prefixlen == 32)
      rv = sw->l3_unicast_host_add(ipv4_dst, 0, false, false);
    else
      rv = sw->l3_unicast_route_add(ipv4_dst, mask, 0, false, false);

    return rv;
  }

  // check if neighbor is configured on this interface
  // if neighbor exists, list size > 0, then force the address entry on
  // the ASIC
  bool update = false;
  if (std::list<struct rtnl_neigh *> neigh;
      search_neigh_cache(ifindex, addr, AF_INET, &neigh) == 0) {
    VLOG(2) << __FUNCTION__ << ": list neigh size " << neigh.size();
    if (neigh.size() > 0)
      update = true;
  }

  if (prefixlen == 32)
    rv = sw->l3_unicast_host_add(ipv4_dst, 0, false, update, vrf_id);
  else
    rv = sw->l3_unicast_route_add(ipv4_dst, mask, 0, false, update, vrf_id);
  if (rv < 0) {
    // TODO shall we remove the l3_termination mac?
    LOG(ERROR) << __FUNCTION__ << ": failed to setup l3 addr " << addr;
  }

  // Avoid adding table VLAN entry for the following two cases
  // Loopback: does not require entry on the Ingress table
  // Bridges and Bridge Interfaces: Entry already set
  if (!is_loopback && !is_bridge && !nl->is_bridge_interface(link)) {
    assert(ifindex);
    // add vlan
    bool tagged = !!rtnl_link_is_vlan(link);
    rv = vlan->add_vlan(link, vid, tagged);
    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__ << ": failed to add vlan id " << vid
                 << " (tagged=" << tagged << " to link " << OBJ_CAST(link);
    }
  }

  return rv;
}

int nl_l3::add_l3_addr_v6(struct rtnl_addr *a) {
  assert(sw);
  assert(a);

  int rv = 0;

  if (a == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": addr can't be null";
    return -EINVAL;
  }

  struct rtnl_link *link = rtnl_addr_get_link(a);
  if (link == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": no link for addr a=" << OBJ_CAST(a);
    return -EINVAL;
  }

  // link local addresses must redirect to controllers
  int port_id = nl->get_port_id(link);
  auto addr = rtnl_addr_get_local(a);
  if (is_ipv6_link_local_address(addr)) {

    rofl::caddress_in6 ipv6_dst = libnl_in6addr_2_rofl(addr, &rv);
    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__ << ": could not parse addr " << addr;
      return rv;
    }

    // All link local addresses have a prefix length of /10
    rofl::caddress_in6 mask = rofl::build_mask_in6(10);

    VLOG(2) << __FUNCTION__ << ": added link local addr " << OBJ_CAST(a);
    rv = sw->l3_unicast_route_add(ipv6_dst, mask, 0, false, false);
    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__
                 << ": could not add unicast route ipv6_dst=" << ipv6_dst
                 << ", mask=" << mask;
      return rv;
    }
  }

  uint16_t vid = vlan->get_vid(link);
  auto lladdr = rtnl_link_get_addr(link);
  rofl::caddress_ll mac = libnl_lladdr_2_rofl(lladdr);

  rv = add_l3_termination(port_id, vid, mac, AF_INET6);
  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__
               << ": failed to setup termination mac port_id=" << port_id
               << ", vid=" << vid << " mac=" << mac << "; rv=" << rv;
    return rv;
  }

  bool is_loopback = (rtnl_link_get_flags(link) & IFF_LOOPBACK);
  if (is_loopback) {
    rv = add_lo_addr_v6(a);
    return rv;
  }

  auto prefixlen = rtnl_addr_get_prefixlen(a);
  rofl::caddress_in6 ipv6_dst = libnl_in6addr_2_rofl(addr, &rv);
  rofl::caddress_in6 mask = rofl::build_mask_in6(prefixlen);

  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": could not parse addr " << addr;
    return rv;
  }

  // check if neighbor is configured on this interface
  // if neighbor exists, list size > 0, then force the address entry on
  // the ASIC
  bool update = false;
  if (std::list<struct rtnl_neigh *> neigh;
      search_neigh_cache(rtnl_addr_get_ifindex(a), addr, AF_INET6, &neigh) == 0) {
    VLOG(3) << __FUNCTION__ << ": list neigh size " << neigh.size();
    if (neigh.size() > 0)
      update = true;
  }

  // TODO support VRF on IPv6 addresses
  if (prefixlen == 32)
    rv = sw->l3_unicast_host_add(ipv6_dst, 0, false, update, 0);
  else
    rv = sw->l3_unicast_route_add(ipv6_dst, mask, 0, false, update, 0);

  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": failed to setup address " << OBJ_CAST(a);
    return rv;
  }

  // Avoid adding table VLAN entry for the following two cases
  // Loopback: does not require entry on the Ingress table
  // Bridges and Bridge Interfaces: Entry already set
  if (!is_loopback && !nl->is_bridge_interface(link)) {
    // add vlan
    bool tagged = !!rtnl_link_is_vlan(link);
    rv = vlan->add_vlan(link, vid, tagged);
    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__ << ": failed to add vlan id " << vid
                 << " (tagged=" << tagged << " to link " << OBJ_CAST(link);
    }
  }

  VLOG(1) << __FUNCTION__ << ": added addr " << OBJ_CAST(a);

  return rv;
}

int nl_l3::add_lo_addr_v6(struct rtnl_addr *a) {
  int rv = 0;
  auto addr = rtnl_addr_get_local(a);

  auto p = nl_addr_alloc(16);
  nl_addr_parse("::1/128", AF_INET, &p);
  std::unique_ptr<nl_addr, decltype(&nl_addr_put)> lo_addr(p, nl_addr_put);

  if (!nl_addr_cmp_prefix(addr, lo_addr.get())) {
    VLOG(1) << __FUNCTION__ << ": skipping loopback address";
    rv = -EINVAL;
    return rv;
  }

  auto prefixlen = rtnl_addr_get_prefixlen(a);
  rofl::caddress_in6 ipv6_dst = libnl_in6addr_2_rofl(addr, &rv);
  auto mask = rofl::build_mask_in6(prefixlen);

  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": could not parse addr " << addr;
    return rv;
  }

  if (prefixlen == 128)
    rv = sw->l3_unicast_host_add(ipv6_dst, 0, false, false);
  else
    rv = sw->l3_unicast_route_add(ipv6_dst, mask, 0, false, false);

  return rv;
}

int nl_l3::del_l3_addr(struct rtnl_addr *a) {
  assert(sw);
  assert(a);

  int rv = 0;
  int family = rtnl_addr_get_family(a);
  struct rtnl_link *link = rtnl_addr_get_link(a);
  uint16_t vid = vlan->get_vid(link);
  uint32_t vrf_id = vlan->get_vrf_id(vid, link);

  if (a == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": addr can't be null";
    return -EINVAL;
  }

  struct nl_addr *addr = rtnl_addr_get_local(a);
  int prefixlen = nl_addr_get_prefixlen(addr);

  // XXX TODO remove vlan

  if (family == AF_INET) {
    rofl::caddress_in4 ipv4_dst = libnl_in4addr_2_rofl(addr, &rv);
    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__ << ": could not parse addr " << addr;
      return rv;
    }

    if (prefixlen == 32) {
      rv = sw->l3_unicast_host_remove(ipv4_dst, vrf_id);
    } else {
      rofl::caddress_in4 mask = rofl::build_mask_in4(prefixlen);
      rv = sw->l3_unicast_route_remove(ipv4_dst, mask, vrf_id);
    }
  } else {
    assert(family == AF_INET6);
    rofl::caddress_in6 ipv6_dst = libnl_in6addr_2_rofl(addr, &rv);
    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__ << ": could not parse addr " << addr;
      return rv;
    }

    if (prefixlen == 128) {
      rv = sw->l3_unicast_host_remove(ipv6_dst, vrf_id);
    } else {
      rofl::caddress_in6 mask = rofl::build_mask_in6(prefixlen);
      rv = sw->l3_unicast_route_remove(ipv6_dst, mask, vrf_id);
    }
  }

  if (link == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": no link for addr a=" << a;
    return -EINVAL;
  }

  if (rtnl_link_get_flags(link) & IFF_LOOPBACK) {
    return 0;
  }

  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__
               << ": failed to remove l3 unicast host(local) rv=" << rv;
    return rv;
  }

  int ifindex = rtnl_addr_get_ifindex(a);
  int port_id = nl->get_port_id(ifindex);

  if (port_id == 0) {
    VLOG(1) << __FUNCTION__ << ": invalid port_id 0";
    return -EINVAL;
  }

  addr = rtnl_link_get_addr(link);
  rofl::caddress_ll mac = libnl_lladdr_2_rofl(addr);

  rv = del_l3_termination(port_id, vid, mac, family);
  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__
               << ": failed to remove l3 termination mac(local) vid=" << vid
               << "; rv=" << rv;
  }

  // del vlan
  bool tagged = !!rtnl_link_is_vlan(link);
  rv = vlan->remove_vlan(link, vid, tagged);
  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": failed to remove vlan id " << vid
               << " (tagged=" << tagged << " to link " << OBJ_CAST(link);
  }

  return rv;
}

int nl_l3::get_l3_addrs(struct rtnl_link *link,
                        std::deque<rtnl_addr *> *addresses) {
  std::unique_ptr<rtnl_addr, decltype(&rtnl_addr_put)> filter(rtnl_addr_alloc(),
                                                              rtnl_addr_put);

  rtnl_addr_set_ifindex(filter.get(), rtnl_link_get_ifindex(link));

  VLOG(1) << __FUNCTION__
          << ": searching l3 addresses from interface=" << OBJ_CAST(link);

  nl_cache_foreach_filter(
      nl->get_cache(cnetlink::NL_ADDR_CACHE), OBJ_CAST(filter.get()),
      [](struct nl_object *o, void *arg) {
        auto *addr = (std::deque<rtnl_addr *> *)arg;
        addr->push_back(ADDR_CAST(o));
      },
      addresses);
  return 0;
}

int nl_l3::get_l3_routes(struct rtnl_link *link,
                         std::deque<rtnl_route *> *routes) {
  std::unique_ptr<rtnl_route, decltype(&rtnl_route_put)> filter(rtnl_route_alloc(),
                                                                rtnl_route_put);
  auto nh = rtnl_route_nh_alloc();

  rtnl_route_nh_set_ifindex(nh, rtnl_link_get_ifindex(link));
  rtnl_route_add_nexthop(filter.get(), nh);
  rtnl_route_set_type(filter.get(), RTN_UNICAST);

  VLOG(1) << __FUNCTION__
          << ": searching l3 routes from interface=" << OBJ_CAST(link);

  nl_cache_foreach_filter(
      nl->get_cache(cnetlink::NL_ROUTE_CACHE), OBJ_CAST(filter.get()),
      [](struct nl_object *o, void *arg) {
        auto *route = (std::deque<rtnl_route *> *)arg;
        route->push_back(ROUTE_CAST(o));
      },
      routes);
  return 0;
}

int nl_l3::add_l3_neigh_egress(struct rtnl_neigh *n, uint32_t *l3_interface_id,
                               uint16_t *vrf_id) {
  int rv;
  assert(n);
  int state = rtnl_neigh_get_state(n);

  switch (state) {
  case NUD_FAILED:
    LOG(INFO) << __FUNCTION__ << ": neighbour not reachable state=failed";
    return -EINVAL;
  case NUD_INCOMPLETE:
    LOG(INFO) << __FUNCTION__ << ": neighbour state=incomplete";
    return 0;
  case NUD_STALE:
    LOG(INFO) << __FUNCTION__ << ": neighbour state=stale";
    break;
  }

  struct nl_addr *d_mac = rtnl_neigh_get_lladdr(n);
  int ifindex = rtnl_neigh_get_ifindex(n);
  auto link = nl->get_link_by_ifindex(ifindex);

  if (link == nullptr)
    return -EINVAL;

  int vid = vlan->get_vid(link.get());
  auto tagged = !!rtnl_link_is_vlan(link.get());
  auto s_mac = rtnl_link_get_addr(link.get());

  if (nl->is_bridge_interface(link.get())) {
    // Handle the Bridge SVI
    // the egress entries for the Bridge SVIs are the ports
    // attached to the bridge
    auto lladdr = rtnl_neigh_get_lladdr(n);
    uint16_t vid = vlan->get_vid(link.get());
    *vrf_id = vlan->get_vrf_id(vid, link.get());

    auto fdb_neigh = nl->search_fdb(vid, lladdr);
    for (auto neigh : fdb_neigh) {
      VLOG(2) << __FUNCTION__ << ": found iface=" << OBJ_CAST(neigh);
      VLOG(2) << __FUNCTION__ << ": found link=" << OBJ_CAST(link.get());

      auto neigh_ifindex = rtnl_neigh_get_ifindex(neigh);
      auto neigh_link = nl->get_link_by_ifindex(neigh_ifindex);

      rv = vlan->add_vlan(neigh_link.get(), vid, tagged);
      if (rv < 0) {
        LOG(ERROR) << __FUNCTION__
                   << ": failed to setup vid ingress vid=" << vid << " on link "
                   << OBJ_CAST(link.get());
        return rv;
      }

      rv = add_l3_egress(neigh_ifindex, vid, s_mac, d_mac, l3_interface_id);

      if (rv < 0) {
        LOG(ERROR) << __FUNCTION__
                   << ": failed to setup vid ingress vid=" << vid << " on link "
                   << OBJ_CAST(link.get());
      }
    }
    return rv;
  }

  // XXX TODO is this still needed?
  rv = vlan->add_vlan(link.get(), vid, tagged);
  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": failed to setup vid ingress vid=" << vid
               << " on link " << OBJ_CAST(link.get());
    return rv;
  }

  rv = add_l3_egress(ifindex, vid, s_mac, d_mac, l3_interface_id);
  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": failed to add l3 egress";
  }

  return rv;
}

int nl_l3::del_l3_neigh_egress(struct rtnl_neigh *n) {
  assert(n);

  auto d_mac = rtnl_neigh_get_lladdr(n);
  int ifindex = rtnl_neigh_get_ifindex(n);
  auto link = nl->get_link_by_ifindex(ifindex);

  if (link == nullptr)
    return -EINVAL;

  uint16_t vid = vlan->get_vid(link.get());
  auto s_mac = rtnl_link_get_addr(link.get());

  if (nl->is_bridge_interface(ifindex)) {
    auto fdb_res = nl->search_fdb(vid, d_mac);

    assert(fdb_res.size() == 1);
    ifindex = rtnl_neigh_get_ifindex(fdb_res.front());
  }

  // XXX TODO del vlan

  // remove egress group reference
  int rv = del_l3_egress(ifindex, vid, s_mac, d_mac);
  if (rv < 0 and rv != -EEXIST) {
    LOG(ERROR) << __FUNCTION__ << ": failed to add l3 egress";
  }

  return rv;
}

int nl_l3::add_l3_neigh(struct rtnl_neigh *n) {
  assert(n);

  int rv;
  uint32_t l3_interface_id = 0;
  struct nl_addr *addr;
  int family = rtnl_neigh_get_family(n);
  uint16_t vrf_id = 0;

  if (n == nullptr)
    return -EINVAL;

  addr = rtnl_neigh_get_dst(n);
  if (family == AF_INET6) {
    auto p = nl_addr_alloc(16);
    nl_addr_parse("fe80::/10", AF_INET6, &p);
    std::unique_ptr<nl_addr, decltype(&nl_addr_put)> lo_addr(p, nl_addr_put);

    if (!nl_addr_cmp_prefix(addr, lo_addr.get())) {
      VLOG(1) << __FUNCTION__ << ": skipping fe80::/10";
      return 0;
    }
  }

  rv = add_l3_neigh_egress(n, &l3_interface_id, &vrf_id);
  LOG(INFO) << __FUNCTION__ << ": adding l3 neigh egress for neigh "
            << OBJ_CAST(n);

  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": add l3 neigh egress failed for neigh "
               << OBJ_CAST(n);
    return rv;
  }

  if (family == AF_INET) {
    rofl::caddress_in4 ipv4_dst = libnl_in4addr_2_rofl(addr, &rv);
    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__ << ": could not parse addr " << addr;
      return rv;
    }
    rv = sw->l3_unicast_host_add(ipv4_dst, l3_interface_id, false, false,
                                 vrf_id);
    VLOG(2) << __FUNCTION__ << ": adding route =" << ipv4_dst
            << " l3 interface id=" << l3_interface_id;

  } else {
    // AF_INET6
    rofl::caddress_in6 ipv6_dst = libnl_in6addr_2_rofl(addr, &rv);
    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__ << ": could not parse addr " << addr;
      return rv;
    }
    rv = sw->l3_unicast_host_add(ipv6_dst, l3_interface_id, false, false,
                                 vrf_id);
  }

  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": add l3 unicast host failed for "
               << OBJ_CAST(n);
    return rv;
  }

  for (auto cb = std::begin(nh_callbacks); cb != std::end(nh_callbacks);) {
    if (cb->second.nh.ifindex == rtnl_neigh_get_ifindex(n) &&
        nl_addr_cmp(cb->second.nh.nh, rtnl_neigh_get_dst(n)) == 0) {
      // XXX TODO add l3_interface?
      cb->first->nh_reachable_notification(cb->second);
      cb = nh_callbacks.erase(cb);
    } else {
      ++cb;
    }
  }

  return rv;
}

int nl_l3::update_l3_neigh(struct rtnl_neigh *n_old, struct rtnl_neigh *n_new) {
  assert(n_old);
  assert(n_new);

  int rv = 0;
  struct nl_addr *n_ll_old;
  struct nl_addr *n_ll_new;

  int ifindex = rtnl_neigh_get_ifindex(n_old);
  uint32_t port_id = nl->get_port_id(ifindex);

  if (port_id == 0) {
    VLOG(1) << __FUNCTION__ << ": port not supported";
    return -EINVAL;
  }

  n_ll_old = rtnl_neigh_get_lladdr(n_old);
  n_ll_new = rtnl_neigh_get_lladdr(n_new);

  if (n_ll_old == nullptr && n_ll_new == nullptr) {
    VLOG(1) << __FUNCTION__ << ": neighbour ll still not reachable";
  } else if (n_ll_old == nullptr && n_ll_new) { // neighbour reachable
    VLOG(2) << __FUNCTION__ << ": neighbour ll reachable";

    rv = add_l3_neigh(n_new);
    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__ << ": failed to add l3 neighbor " << n_new
                 << "; rv=" << rv;
      return rv;
    }

  } else if (n_ll_old && n_ll_new == nullptr) {
    // neighbour unreachable
    VLOG(2) << __FUNCTION__ << ": neighbour ll unreachable";

    int ifindex = rtnl_neigh_get_ifindex(n_old);
    auto link = nl->get_link_by_ifindex(ifindex);

    if (link.get() == nullptr) {
      LOG(ERROR) << __FUNCTION__ << ": link not found ifindex=" << ifindex;
      return -EINVAL;
    }

    int family = rtnl_neigh_get_family(n_new);
    struct nl_addr *addr = rtnl_neigh_get_dst(n_old);
    if (family == AF_INET) {
      rofl::caddress_in4 ipv4_dst = libnl_in4addr_2_rofl(addr, &rv);
      if (rv < 0) {
        LOG(ERROR) << __FUNCTION__ << ": could not parse addr " << addr;
        return rv;
      }

      // delete next hop
      rv = sw->l3_unicast_host_remove(ipv4_dst);
      if (rv < 0) {
        LOG(ERROR) << __FUNCTION__
                   << ": failed to remove l3 unicast host ipv4_dst" << ipv4_dst
                   << "; rv=" << rv;
        return rv;
      }
    } else if (family == AF_INET6) {
      rofl::caddress_in6 ipv6_dst = libnl_in6addr_2_rofl(addr, &rv);
      if (rv < 0) {
        LOG(ERROR) << __FUNCTION__ << ": could not parse addr " << addr;
        return rv;
      }

      // delete next hop
      rv = sw->l3_unicast_host_remove(ipv6_dst);
      if (rv < 0) {
        LOG(ERROR) << __FUNCTION__
                   << ": failed to remove l3 unicast host ipv6_dst" << ipv6_dst
                   << "; rv=" << rv;
        return rv;
      }
    }

    // delete from mapping
    uint16_t vid = vlan->get_vid(link.get());
    rv = del_l3_egress(ifindex, vid, rtnl_link_get_addr(link.get()),
                       rtnl_neigh_get_lladdr(n_old));
  } else if (nl_addr_cmp(n_ll_old, n_ll_new)) {
    // XXX TODO ll addr changed
    VLOG(1) << __FUNCTION__ << ": neighbour ll changed: new neighbor "
            << n_ll_new << " ifindex=" << ifindex;

    auto link = nl->get_link_by_ifindex(ifindex);

    if (link.get() == nullptr) {
      LOG(ERROR) << __FUNCTION__ << ": link not found ifindex=" << ifindex;
      return -EINVAL;
    }

    auto s_mac = rtnl_link_get_addr(link.get());
    uint16_t vid = vlan->get_vid(link.get());

    VLOG(2) << __FUNCTION__ << " : source old mac " << s_mac << " dst old mac  "
            << n_ll_old << " dst new mac " << n_ll_new;

    auto l3_if_old_tuple =
        std::make_tuple(port_id, vid, libnl_lladdr_2_rofl(s_mac),
                        libnl_lladdr_2_rofl(n_ll_old));
    auto l3_if_new_tuple =
        std::make_tuple(port_id, vid, libnl_lladdr_2_rofl(s_mac),
                        libnl_lladdr_2_rofl(n_ll_new));

    // Obtain l3_interface_id
    auto it_old = l3_interface_mapping.find(l3_if_old_tuple);
    if (it_old == l3_interface_mapping.end()) {
      LOG(ERROR) << __FUNCTION__ << ": could not retrieve neighbor";
      return -EINVAL;
    }

    uint32_t l3_interface_id = it_old->second.l3_interface_id;
    rv = sw->l3_egress_update(port_id, vid, libnl_lladdr_2_rofl(s_mac),
                              libnl_lladdr_2_rofl(n_ll_new), &l3_interface_id);
    if (rv < 0) {
      VLOG(2) << __FUNCTION__ << ": failed to update neighbor";
      return -EINVAL;
    }

    auto it_new = l3_interface_mapping.find(l3_if_new_tuple);
    if (it_new != l3_interface_mapping.end()) {
      LOG(ERROR) << __FUNCTION__ << ": neighbor already present";
      return -EINVAL;
    }

    l3_interface_mapping.erase(it_old->first);
    l3_interface_mapping.emplace(
        std::make_pair(l3_if_new_tuple, it_old->second));

  } else {
    // nothing changed besides the nud
    VLOG(2) << __FUNCTION__ << ": nud changed from "
            << rtnl_neigh_get_state(n_old) << " to "
            << rtnl_neigh_get_state(n_new); // TODO print names
  }

  return rv;
}

int nl_l3::del_l3_neigh(struct rtnl_neigh *n) {
  assert(n);

  int rv = 0;
  struct nl_addr *addr = rtnl_neigh_get_dst(n);
  int family = rtnl_neigh_get_family(n);
  bool skip_addr_remove = false;

  if (n == nullptr)
    return -EINVAL;

  int ifindex = rtnl_neigh_get_ifindex(n);
  auto link = nl->get_link_by_ifindex(ifindex);

  if (!link)
    return -EINVAL;

  std::deque<rtnl_addr *> link_addresses;
  get_l3_addrs(link.get(), &link_addresses);
  for (auto i : link_addresses) {
    if (struct nl_addr *link_addr = rtnl_addr_get_local(i);
        nl_addr_cmp(addr, link_addr) == 0) {
      skip_addr_remove = true;
      break;
    }
  }

  if (family == AF_INET && !skip_addr_remove) {
    rofl::caddress_in4 ipv4_dst = libnl_in4addr_2_rofl(addr, &rv);
    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__ << ": could not parse addr " << addr;
      return rv;
    }

    // delete next hop
    rv = sw->l3_unicast_host_remove(ipv4_dst);
  } else if (family == AF_INET6 && !skip_addr_remove) {
    auto p = nl_addr_alloc(16);
    nl_addr_parse("fe80::/10", AF_INET6, &p);
    std::unique_ptr<nl_addr, decltype(&nl_addr_put)> lo_addr(p, nl_addr_put);

    if (!nl_addr_cmp_prefix(addr, lo_addr.get())) {
      VLOG(1) << __FUNCTION__ << ": skipping fe80::/10";
      return 0;
    }

    rofl::caddress_in6 ipv6_dst = libnl_in6addr_2_rofl(addr, &rv);
    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__ << ": could not parse addr " << addr;
      return rv;
    }

    // delete next hop
    rv = sw->l3_unicast_host_remove(ipv6_dst);
  }

  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": failed to remove l3 unicast host ipv4_dst";
    return rv;
  }

  struct nl_addr *s_mac = rtnl_link_get_addr(link.get());
  struct nl_addr *d_mac = rtnl_neigh_get_lladdr(n);
  uint16_t vid = vlan->get_vid(link.get());

  // delete l3 unicast group (mac rewrite)
  if (s_mac && d_mac)
    rv = del_l3_egress(ifindex, vid, s_mac, d_mac);

  // if a route still exists thats pointing to the nexthop,
  // update the route to point to controller and delete the nexthop egress 
  // reference
  if (rv == -EEXIST) {
    for (auto cb = std::begin(nh_unreach_callbacks);
         cb != std::end(nh_unreach_callbacks);) {
      if (cb->second.nh.ifindex == rtnl_neigh_get_ifindex(n) &&
          nl_addr_cmp(cb->second.nh.nh, rtnl_neigh_get_dst(n)) == 0) {
        cb->first->nh_unreachable_notification(n, cb->second);
        cb = nh_unreach_callbacks.erase(cb);
      } else {
        ++cb;
      }
    }
  }

  return rv;
}

int nl_l3::add_l3_egress(int ifindex, const uint16_t vid,
                         const struct nl_addr *s_mac,
                         const struct nl_addr *d_mac,
                         uint32_t *l3_interface_id) {
  assert(s_mac);
  assert(d_mac);

  int rv = 0;

  // XXX TODO handle this on bridge interface
  uint32_t port_id = nl->get_port_id(ifindex);

  if (port_id == 0) {
    VLOG(1) << __FUNCTION__ << ": invalid port_id=0 of ifindex" << ifindex;
    return -EINVAL;
  }

  // setup egress L3 Unicast group
  rofl::caddress_ll src_mac = libnl_lladdr_2_rofl(s_mac);
  rofl::caddress_ll dst_mac = libnl_lladdr_2_rofl(d_mac);
  auto l3_if_tuple = std::make_tuple(port_id, vid, src_mac, dst_mac);
  auto it = l3_interface_mapping.find(l3_if_tuple);

  if (it == l3_interface_mapping.end()) {
    rv = sw->l3_egress_create(port_id, vid, src_mac, dst_mac, l3_interface_id);

    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__
                 << ": failed to setup l3 egress port_id=" << port_id
                 << ", vid=" << vid << ", src_mac=" << src_mac
                 << ", dst_mac=" << dst_mac << "; rv=" << rv;
      return rv;
    }

    l3_interface_mapping.emplace(
        std::make_pair(l3_if_tuple, l3_interface(*l3_interface_id)));
    VLOG(1) << __FUNCTION__
            << ": Layer 3 egress id created, l3_interface=" << *l3_interface_id
            << " port_id=" << port_id;
  } else {
    it->second.refcnt++;
    *l3_interface_id = it->second.l3_interface_id;
  }

  return rv;
}

int nl_l3::del_l3_egress(int ifindex, uint16_t vid, const struct nl_addr *s_mac,
                         const struct nl_addr *d_mac) {
  assert(s_mac);
  assert(d_mac);

  uint32_t port_id = nl->get_port_id(ifindex);

  if (port_id == 0) {
    VLOG(1) << __FUNCTION__ << ": invalid port_id=0 of ifindex=" << ifindex;
    return -EINVAL;
  }

  rofl::caddress_ll src_mac = libnl_lladdr_2_rofl(s_mac);
  rofl::caddress_ll dst_mac = libnl_lladdr_2_rofl(d_mac);
  auto l3_if_tuple = std::make_tuple(port_id, vid, src_mac, dst_mac);
  auto it = l3_interface_mapping.find(l3_if_tuple);

  if (it != l3_interface_mapping.end()) {
    it->second.refcnt--;
    VLOG(2) << __FUNCTION__ << ": port_id=" << port_id << ", vid=" << vid
            << ", s_mac=" << s_mac << ", d_mac=" << d_mac
            << ", refcnt=" << it->second.refcnt;

    if (it->second.refcnt == 0) {
      // remove egress L3 Unicast group
      int rv = sw->l3_egress_remove(it->second.l3_interface_id);

      l3_interface_mapping.erase(it);

      if (rv < 0) {
        LOG(ERROR) << __FUNCTION__
                   << ": failed to remove l3 egress port_id=" << port_id
                   << ", vid=" << vid << ", src_mac=" << src_mac
                   << ", dst_mac=" << dst_mac << "; rv=" << rv;
        return rv;
      }
    }
    return -EEXIST;
  }

  LOG(ERROR) << __FUNCTION__
             << ": l3 interface mapping not found port_id=" << port_id
             << ", vid=" << vid << ", src_mac=" << src_mac
             << ", dst_mac=" << dst_mac;

  return -ENODATA;
}

int nl_l3::add_l3_route(struct rtnl_route *r) {
  assert(r);

  int rv = 0;

  switch (rtnl_route_get_type(r)) {
  case RTN_UNICAST:
    rv = add_l3_unicast_route(r, false);
    break;
  case RTN_UNSPEC:
  case RTN_LOCAL:
  case RTN_BROADCAST:
  case RTN_ANYCAST:
  case RTN_MULTICAST:
  case RTN_BLACKHOLE:
  case RTN_UNREACHABLE:
  case RTN_PROHIBIT:
  case RTN_THROW:
  case RTN_NAT:
  case RTN_XRESOLVE:
    VLOG(1) << __FUNCTION__
            << ": skip route with type=" << (uint32_t)rtnl_route_get_type(r);
    return -ENOTSUP;
  default:
    VLOG(1) << __FUNCTION__ << ": skip route with unknown type="
            << (uint32_t)rtnl_route_get_type(r);
    return -EINVAL;
  }

  // check for reachable addresses
  for (auto cb = std::begin(net_callbacks); cb != std::end(net_callbacks);) {
    if (nl_addr_cmp_prefix(cb->second.addr, rtnl_route_get_dst(r)) == 0) {
      cb->first->net_reachable_notification(cb->second);
      cb = net_callbacks.erase(cb);
    } else {
      ++cb;
    }
  }

  return rv;
}

int nl_l3::update_l3_route(struct rtnl_route *r_old, struct rtnl_route *r_new) {
  int rv = 0;

  assert(r_old);
  assert(r_new);

  if (rtnl_route_get_type(r_old) != rtnl_route_get_type(r_new)) {
    VLOG(1) << __FUNCTION__
            << ": route type change not supported; changed from "
            << rtnl_route_get_type(r_old) << " to "
            << rtnl_route_get_type(r_new);
    return -ENOTSUP;
  }

  switch (rtnl_route_get_type(r_old)) {
  case RTN_UNICAST:
    rv = update_l3_unicast_route(r_old, r_new);
    break;
  case RTN_UNSPEC:
  case RTN_LOCAL:
  case RTN_BROADCAST:
  case RTN_ANYCAST:
  case RTN_MULTICAST:
  case RTN_BLACKHOLE:
  case RTN_UNREACHABLE:
  case RTN_PROHIBIT:
  case RTN_THROW:
  case RTN_NAT:
  case RTN_XRESOLVE:
    VLOG(1) << __FUNCTION__
            << ": skip route with type=" << rtnl_route_get_type(r_old);
    return -ENOTSUP;
  default:
    VLOG(1) << __FUNCTION__
            << ": skip route with unknown type=" << rtnl_route_get_type(r_old);
    return -EINVAL;
  }
  return rv;
}

int nl_l3::del_l3_route(struct rtnl_route *r) {
  assert(r);

  switch (rtnl_route_get_type(r)) {
  case RTN_UNICAST:
    return del_l3_unicast_route(r, false);
  case RTN_UNSPEC:
  case RTN_LOCAL:
  case RTN_BROADCAST:
  case RTN_ANYCAST:
  case RTN_MULTICAST:
  case RTN_BLACKHOLE:
  case RTN_UNREACHABLE:
  case RTN_PROHIBIT:
  case RTN_THROW:
  case RTN_NAT:
  case RTN_XRESOLVE:
    VLOG(2) << __FUNCTION__ << ": skip route";
    return -ENOTSUP;
  default:
    return -EINVAL;
  }
}

int nl_l3::add_l3_termination(uint32_t port_id, uint16_t vid,
                              const rofl::caddress_ll &mac, int af) noexcept {
  int rv = 0;

  // lookup if this already exists
  auto needle = std::make_tuple(port_id, vid, mac, static_cast<uint16_t>(af));
  auto tmac_lookup_range = termination_mac_mapping.equal_range(needle);
  auto i = tmac_lookup_range.first;
  for (; i != tmac_lookup_range.second; ++i) {
    if (i->first == needle) {
      // found, increment refcount
      i->second++;
      return 0;
    }
  }

  // not found, add
  if (tmac_lookup_range.first != termination_mac_mapping.end())
    termination_mac_mapping.emplace_hint(tmac_lookup_range.first,
                                         std::move(needle), 1);
  else
    termination_mac_mapping.emplace(std::move(needle), 1);

  switch (af) {
  case AF_INET:
    rv = sw->l3_termination_add(port_id, vid, mac);
    break;

  case AF_INET6:
    rv = sw->l3_termination_add_v6(port_id, vid, mac);
    break;

  default:
    LOG(FATAL) << __FUNCTION__ << ": invalid address family " << af;
    break;
  }

  return rv;
}

int nl_l3::del_l3_termination(uint32_t port_id, uint16_t vid,
                              const rofl::caddress_ll &mac, int af) noexcept {
  int rv = 0;

  VLOG(4) << __FUNCTION__ << ": trying to delete for port_id=" << port_id
          << ", vid=" << vid << ", mac=" << mac << ", af=" << af;

  // lookup if this already exists
  auto needle = std::make_tuple(port_id, vid, mac, static_cast<uint16_t>(af));
  auto tmac_lookup_range = termination_mac_mapping.equal_range(needle);
  auto i = tmac_lookup_range.first;
  for (; i != tmac_lookup_range.second; ++i) {
    if (i->first == needle) {
      // found, decrement refcount
      --i->second;
      VLOG(4) << __FUNCTION__ << ": found new refcount=" << i->second;
      break;
    }
  }

  // check if not found

  // Modifying the previously existing entries on the VLAN table requires
  // removing the existing entry, since OFDPA requires that the VLAN must be
  // configured with the same VRF
  if (i == termination_mac_mapping.end()) {
    LOG(WARNING)
        << __FUNCTION__
        << ": tried to delete a non existing termination mac for port_id="
        << port_id << ", vid=" << vid << ", mac=" << mac << ", af=" << af;
    return 0;
  }

  // still existing references
  if (i->second > 0) {
    VLOG(4) << __FUNCTION__ << ": got references " << i->second;
    return 0;
  }

  switch (af) {
  case AF_INET:
    rv = sw->l3_termination_remove(port_id, vid, mac);
    break;

  case AF_INET6:
    rv = sw->l3_termination_remove_v6(port_id, vid, mac);
    break;

  default:
    LOG(FATAL) << __FUNCTION__ << ": invalid address family " << af;
    break;
  }

  termination_mac_mapping.erase(i);

  return rv;
}

void nl_l3::get_nexthops_of_route(
    rtnl_route *route, std::deque<struct rtnl_nexthop *> *nhs) noexcept {
  std::deque<struct rtnl_nexthop *> _nhs;

  assert(route);
  assert(nhs);

  // extract next hops
  rtnl_route_foreach_nexthop(
      route,
      [](struct rtnl_nexthop *nh, void *arg) {
        auto nhops = static_cast<std::deque<struct rtnl_nexthop *> *>(arg);
        nhops->push_back(nh);
      },
      &_nhs);

  // verify next hop
  for (auto nh : _nhs) {
    auto ifindex = rtnl_route_nh_get_ifindex(nh);
    uint16_t pport_id = nl->get_port_id(ifindex);
    auto link = nl->get_link_by_ifindex(ifindex);

    // Guarantee that the interface is found
    if (!link.get())
      continue;

    if (pport_id == 0 && !nl->is_bridge_interface(link.get())) {
      VLOG(1) << __FUNCTION__ << ": ignoring next hop " << nh;
      continue;
    }

    nhs->push_back(nh);
  }
}

int nl_l3::get_neighbours_of_route(
    rtnl_route *route, std::deque<struct rtnl_neigh *> *neighs,
    std::deque<nh_stub> *unresolved_nh) noexcept {

  auto route_dst = rtnl_route_get_dst(route);
  std::deque<struct rtnl_nexthop *> nhs;

  assert(route);
  assert(neighs);
  assert(unresolved_nh);

  get_nexthops_of_route(route, &nhs);

  if (nhs.size() == 0)
    return -ENETUNREACH;

  for (auto nh : nhs) {
    int ifindex = rtnl_route_nh_get_ifindex(nh);
    rtnl_neigh *neigh = nullptr;
    nl_addr *nh_addr = rtnl_route_nh_get_gateway(nh);

    if (!nh_addr)
      nh_addr = rtnl_route_nh_get_via(nh);

    if (!nh_addr)
      nh_addr = route_dst;

    if (nh_addr) {
      switch (nl_addr_get_family(nh_addr)) {
      case AF_INET:
      case AF_INET6:
        VLOG(2) << __FUNCTION__ << ": ifindex=" << ifindex << " gw=" << nh_addr;
        break;
      default:
        LOG(ERROR) << "gw " << nh_addr
                   << " unsupported family=" << nl_addr_get_family(nh_addr);
        nh_addr = nullptr;
      }

      neigh = nl->get_neighbour(ifindex, nh_addr);
      VLOG(2) << __FUNCTION__ << ": get neigh=" << OBJ_CAST(neigh)
              << " of nh_addr=" << nh_addr;
    }

    if (neigh)
      neighs->push_back(neigh);
    else
      unresolved_nh->push_back({nh_addr, ifindex});
  }

  return nhs.size();
}

void nl_l3::register_switch_interface(switch_interface *sw) { this->sw = sw; }

void nl_l3::notify_on_net_reachable(net_reachable *f,
                                    struct net_params p) noexcept {
  net_callbacks.emplace_back(f, p);
}

void nl_l3::notify_on_nh_reachable(nh_reachable *f,
                                   struct nh_params p) noexcept {
  nh_callbacks.emplace_back(f, p);
}

void nl_l3::notify_on_nh_unreachable(nh_unreachable *f,
                                     struct nh_params p) noexcept {
  nh_unreach_callbacks.emplace_back(f, p);
}

int nl_l3::get_l3_interface_id(int ifindex, const struct nl_addr *s_mac,
                               const struct nl_addr *d_mac,
                               uint32_t *l3_interface_id, uint16_t vid) {
  assert(l3_interface_id);
  assert(s_mac);
  assert(d_mac);

  auto link = nl->get_link_by_ifindex(ifindex);

  if (link == nullptr)
    return -EINVAL;

  if (!vid)
    vid = vlan->get_vid(link.get());

  uint32_t port_id = nl->get_port_id(ifindex);
  rofl::caddress_ll src_mac = libnl_lladdr_2_rofl(s_mac);
  rofl::caddress_ll dst_mac = libnl_lladdr_2_rofl(d_mac);
  auto needle = std::make_tuple(port_id, vid, src_mac, dst_mac);

  auto it = l3_interface_mapping.find(needle);

  if (it == l3_interface_mapping.end()) {
    LOG(ERROR) << __FUNCTION__ << ": l3_interface_id entry not found for port "
               << port_id << ", src_mac " << src_mac << ", dst_mac " << dst_mac
               << ", vid " << vid;
    return -ENODATA;
  }

  *l3_interface_id = it->second.l3_interface_id;

  VLOG(2) << __FUNCTION__ << ": l3_interface_id " << *l3_interface_id
          << " found for port " << port_id << ", src_mac " << src_mac
          << ", dst_mac " << dst_mac << ", vid " << vid;
  return 0;
}

int nl_l3::add_l3_unicast_route(nl_addr *rt_dst, uint32_t l3_interface_id,
                                bool is_ecmp, bool update_route,
                                uint16_t vrf_id) {
  if (rt_dst == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": rt_dst cannot be nullptr";
    return -EINVAL;
  }

  auto dst_af = nl_addr_get_family(rt_dst);
  int prefixlen = nl_addr_get_prefixlen(rt_dst);
  int rv = 0;

  if (dst_af == AF_INET) {
    rofl::caddress_in4 ipv4_dst = libnl_in4addr_2_rofl(rt_dst, &rv);
    rofl::caddress_in4 mask = rofl::build_mask_in4(prefixlen);

    if (prefixlen == 32) {
      rv = sw->l3_unicast_host_add(ipv4_dst, l3_interface_id, is_ecmp,
                                   update_route, vrf_id);
    } else {
      rv = sw->l3_unicast_route_add(ipv4_dst, mask, l3_interface_id, is_ecmp,
                                    update_route, vrf_id);
    }
    VLOG(2) << __FUNCTION__ << ": adding route =" << ipv4_dst
            << "l3 interface id=" << l3_interface_id;
  } else if (dst_af == AF_INET6) {
    rofl::caddress_in6 ipv6_dst = libnl_in6addr_2_rofl(rt_dst, &rv);
    rofl::caddress_in6 mask = rofl::build_mask_in6(prefixlen);

    if (prefixlen == 128) {
      rv = sw->l3_unicast_host_add(ipv6_dst, l3_interface_id, is_ecmp,
                                   update_route, vrf_id);
    } else {
      rv = sw->l3_unicast_route_add(ipv6_dst, mask, l3_interface_id, is_ecmp,
                                    update_route, vrf_id);
    }
  } else {
    LOG(FATAL) << __FUNCTION__ << ": not reached";
  }

  return rv;
}

int nl_l3::del_l3_unicast_route(nl_addr *rt_dst, uint16_t vrf_id) {
  int rv;
  // remove route pointing to group
  int prefixlen = nl_addr_get_prefixlen(rt_dst);
  int family = nl_addr_get_family(rt_dst);
  if (family == AF_INET) {
    rofl::caddress_in4 ipv4_dst = libnl_in4addr_2_rofl(rt_dst, &rv);

    if (prefixlen == 32) {
      rv = sw->l3_unicast_host_remove(ipv4_dst, vrf_id);
    } else {
      rofl::caddress_in4 mask = rofl::build_mask_in4(prefixlen);
      rv = sw->l3_unicast_route_remove(ipv4_dst, mask, vrf_id);
    }
  } else {
    rofl::caddress_in6 ipv6_dst = libnl_in6addr_2_rofl(rt_dst, &rv);

    if (prefixlen == 128) {
      rv = sw->l3_unicast_host_remove(ipv6_dst, vrf_id);
    } else {
      rofl::caddress_in6 mask = rofl::build_mask_in6(prefixlen);
      rv = sw->l3_unicast_route_remove(ipv6_dst, mask, vrf_id);
    }
  }

  return rv;
}

int nl_l3::add_l3_ecmp_route(rtnl_route *r,
                             const std::set<uint32_t> &l3_interface_ids,
                             bool update_route) {
  assert(r);

  int rv = 0;
  uint32_t vrf_id = rtnl_route_get_table(r);
  uint32_t l3_ecmp_id = -1;
  static uint32_t l3_ecmp_id_next = 1;
  auto it_range = l3_ecmp_mapping.equal_range(l3_interface_ids);
  auto i = it_range.first;

  // check if an ecmp ID already exists
  for (; i != it_range.second; ++i) {
    if (i->first == l3_interface_ids) {
      i->second.refcnt++;
      l3_ecmp_id = i->second.l3_interface_id;

      VLOG(2) << __FUNCTION__
              << ": found ecmp id: " << i->second.l3_interface_id
              << ", refcnt=" << i->second.refcnt;
      break;
    }
  }

  // no l3_ecmp_id found -> create a new one
  if (l3_ecmp_id == (uint32_t)-1) {
    l3_ecmp_id = l3_ecmp_id_next;
    rv = sw->l3_ecmp_add(l3_ecmp_id, l3_interface_ids);
    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__
                 << ": failed to create l3 ecmp id=" << l3_ecmp_id;

      return -EINVAL;
    }

    // register the new l3_ecmp_id
    l3_ecmp_mapping.emplace(
        std::make_pair(l3_interface_ids, l3_interface(l3_ecmp_id)));

    // increment l3_ecmp_id
    l3_ecmp_id_next++;
  }

  // create route
  rv = add_l3_unicast_route(rtnl_route_get_dst(r), l3_ecmp_id, true,
                            update_route, vrf_id);
  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__
               << ": failed to create l3 unicast route with ecmp_id="
               << l3_ecmp_id << ", route: " << OBJ_CAST(r);
  }

  return rv;
}

int nl_l3::del_l3_ecmp_route(rtnl_route *r,
                             const std::set<uint32_t> &l3_interface_ids) {
  assert(r);

  auto it_range = l3_ecmp_mapping.equal_range(l3_interface_ids);

  if (it_range.first == l3_ecmp_mapping.end()) {
    LOG(ERROR) << __FUNCTION__ << ": ecmp group not found for route " << r;
    if (VLOG_IS_ON(4)) {
      std::string str;
      for (auto id : l3_interface_ids) {
        str += std::to_string(id) + " ";
      }
      LOG(INFO) << __FUNCTION__ << ": l3_interfaces: " << str;
    }
    return -EINVAL;
  }

  for (auto i = it_range.first; i != it_range.second; ++i) {
    if (i->first == l3_interface_ids) {
      // found
      i->second.refcnt--;
      VLOG(4) << __FUNCTION__ << ": found l3 interface id for ecmp route id="
              << i->second.l3_interface_id << ", refcount=" << i->second.refcnt
              << ", route " << OBJ_CAST(r);

      if (i->second.refcnt <= 0) {
        int rv = sw->l3_ecmp_remove(i->second.l3_interface_id);
        l3_ecmp_mapping.erase(i);
        return rv;
      } else
        return 0;
    }
  }

  LOG(ERROR) << __FUNCTION__
             << ": tried to delete invalid ecmp interface for route "
             << OBJ_CAST(r);

  return -EINVAL;
}

void nl_l3::nh_reachable_notification(struct nh_params p) noexcept {
  nl_route_query rq;
  auto rt = rq.query_route(p.np.addr);
  auto ad = rtnl_route_get_dst(rt);

  // guarantee the correct prefixlen
  nl_addr_set_prefixlen(ad, nl_addr_get_prefixlen(p.np.addr));

  add_l3_unicast_route(rt, true);
  // return object has to be freed using nl_object_put
  nl_object_put(OBJ_CAST(rt));
}

void nl_l3::nh_unreachable_notification(struct rtnl_neigh *n,
                                        struct nh_params p) noexcept {
  // TODO handle VRF
  add_l3_unicast_route(p.np.addr, 0, false, true, 0);
  del_l3_neigh_egress(n);
  notify_on_nh_reachable(this, p);
}

int nl_l3::add_l3_unicast_route(rtnl_route *r, bool update_route) {
  assert(r);
  int nnhs = rtnl_route_get_nnexthops(r);
  uint32_t vrf_id = rtnl_route_get_table(r);
  if (vrf_id == MAIN_ROUTING_TABLE)
    vrf_id = 0;

  VLOG(2) << __FUNCTION__ << ": adding route= " << OBJ_CAST(r)
          << " update=" << update_route;

  if (nnhs == 0) {
    LOG(WARNING) << __FUNCTION__ << ": no neighbours of route " << OBJ_CAST(r);
    return -ENOTSUP;
  }

  std::deque<struct rtnl_neigh *> neighs;
  std::deque<nh_stub> unresolved_nh;
  int rv = get_neighbours_of_route(r, &neighs, &unresolved_nh);

  // no neighbours reachable
  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": no next hops for route " << OBJ_CAST(r);
    return rv;
  }

  for (auto nh : unresolved_nh) {
    VLOG(2) << __FUNCTION__ << ": got unresolved nh ifindex=" << nh.ifindex
            << ", nh=" << nh.nh;

    // If the next-hop is currently unresolved, we store the route and
    // process it when the nh is found
    if (!is_ipv6_link_local_address(rtnl_route_get_dst(r)) &&
        !is_ipv6_multicast_address(rtnl_route_get_dst(r)) &&
        rtnl_route_get_scope(r) != RT_SCOPE_LINK)
      notify_on_nh_reachable(
          this, nh_params{net_params{rtnl_route_get_dst(r), nh.ifindex}, nh});
  }

  VLOG(2) << __FUNCTION__ << ": got " << neighs.size() << " neighbours ("
          << unresolved_nh.size() << " unresolved next hops)";

  // each route increments the ref counter for all nexthops (l3 interface)
  std::set<uint32_t> l3_interfaces; // all create l3 interface ids
  for (auto n : neighs) {
    uint32_t l3_interface_id = 0;
    auto ifindex = rtnl_neigh_get_ifindex(n);

    auto link = nl->get_link_by_ifindex(ifindex);
    auto vid = vlan->get_vid(link.get());
    struct nl_addr *s_mac = rtnl_link_get_addr(link.get());
    struct nl_addr *d_mac = rtnl_neigh_get_lladdr(n);

    // For the Bridge SVI, the l3 interface already exists
    // so we can just get that one
    if (nl->is_bridge_interface(ifindex)) {
      auto fdb_res = nl->search_fdb(vid, d_mac);

      assert(fdb_res.size() == 1);
      ifindex = rtnl_neigh_get_ifindex(fdb_res.front());
    }

    // add neigh
    rv = add_l3_egress(ifindex, vid, s_mac, d_mac, &l3_interface_id);
    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__ << ": add l3 egress failed for neigh "
                 << OBJ_CAST(n);
      // XXX TODO create l3 neigh later
      continue;
    }

    VLOG(2) << __FUNCTION__ << ": got l3_interface_id=" << l3_interface_id;
    l3_interfaces.emplace(l3_interface_id);

    // Register route to track nexthop changes
    struct nh_stub nh {
      rtnl_neigh_get_dst(n), ifindex
    };
    notify_on_nh_unreachable(
        this, nh_params{net_params{rtnl_route_get_dst(r), nh.ifindex}, nh});
  }

  switch (nnhs) {
  case 0:
    // Not yet determined next-hop, the rule must be installed to the switch
    // pointing to controller and then updated after next-hop registration
    rv = add_l3_unicast_route(rtnl_route_get_dst(r), 0, false, update_route,
                              vrf_id);
    break;
  case 1:
    // Single known next-hop
    rv = add_l3_unicast_route(rtnl_route_get_dst(r), *l3_interfaces.begin(),
                              false, update_route, vrf_id);
    break;
  default:
    // create ecmp group
    rv = add_l3_ecmp_route(r, l3_interfaces, update_route);
    break;
  }

  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__
               << ": failed to add route to dst=" << rtnl_route_get_dst(r)
               << " rv=" << rv;
  }

  // cleanup
  for (auto n : neighs)
    rtnl_neigh_put(n);

  return rv;
}

int nl_l3::update_l3_unicast_route(rtnl_route *r_old, rtnl_route *r_new) {
  int rv = 0;

  // currently we will only handle next hop changes
  add_l3_unicast_route(r_new, true);
  del_l3_unicast_route(r_old, true);

  return rv;
}

int nl_l3::del_l3_unicast_route(rtnl_route *r, bool keep_route) {
  int rv = 0;
  auto dst = rtnl_route_get_dst(r);

  uint16_t vrf_id = rtnl_route_get_table(r);
  // Routing table 254 id matches the main routing table on linux.
  // Adding a vrf with this id will collide with the main routing
  // table, but will enter the wrong info into the OFDPA tables
  if (vrf_id == MAIN_ROUTING_TABLE)
    vrf_id = 0;

  if (!keep_route) {
    rv = del_l3_unicast_route(dst, vrf_id);
    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__ << ": failed to remove dst=" << dst;
      // fallthrough
    }
  }

  std::deque<struct rtnl_neigh *> neighs;
  std::deque<nh_stub> unresolved_nh;
  get_neighbours_of_route(r, &neighs, &unresolved_nh);

  VLOG(2) << __FUNCTION__ << ": number of next hops is "
          << rtnl_route_get_nnexthops(r);

  // Cleanup the unresolved route on deletion. Make sure to only remove the
  // first hit, as we want to keep the second one that will have been added
  // by add_l3_unicast_route() from update_l3_unicact_route().
  // If this is an actual deletion, there should only be one anyway.
  for (auto i : unresolved_nh) {
    auto it = std::find_if(nh_callbacks.begin(), nh_callbacks.end(),
                           [&](std::pair<nh_reachable *, nh_params> &cb) {
                             return cb.first == this &&
                                    cb.second.nh.ifindex == i.ifindex &&
                                    !nl_addr_cmp_prefix(cb.second.np.addr, dst);
                           });

    if (it != nh_callbacks.end())
      nh_callbacks.erase(it);
  }

  if (neighs.size() == 0) {
    LOG(ERROR) << __FUNCTION__ << ": no nexthop for this route " << OBJ_CAST(r);
    return rv;
  }

  if (neighs.size() > 1) {
    // get all l3 interfaces
    std::set<uint32_t> l3_interfaces; // all create l3 interface ids
    for (auto n : neighs) {
      uint32_t l3_interface_id = 0;
      auto ifindex = rtnl_neigh_get_ifindex(n);

      auto link = nl->get_link_by_ifindex(ifindex);
      auto vid = vlan->get_vid(link.get());

      struct nl_addr *s_mac = rtnl_link_get_addr(link.get());
      struct nl_addr *d_mac = rtnl_neigh_get_lladdr(n);

      // For the Bridge SVI, the l3 interface already exists
      // so we can just get that one
      if (nl->is_bridge_interface(ifindex)) {
        auto fdb_res = nl->search_fdb(vid, d_mac);

        assert(fdb_res.size() == 1);
        ifindex = rtnl_neigh_get_ifindex(fdb_res.front());
      }
      // add neigh
      rv = get_l3_interface_id(ifindex, s_mac, d_mac, &l3_interface_id, vid);

      if (rv < 0) {
        LOG(ERROR) << __FUNCTION__
                   << ": could not retrieve l3 interface id for neigh "
                   << OBJ_CAST(n);
        // XXX TODO check how this can be handled
        continue;
      }

      VLOG(4) << __FUNCTION__ << ": got l3_interface_id=" << l3_interface_id;
      l3_interfaces.emplace(l3_interface_id);
    }

    // XXX FIXME delete ecmp first then delete all l3_interfaces
    rv = del_l3_ecmp_route(r, l3_interfaces);
    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__ << ": failed to delete l3 ecmp route "
                 << OBJ_CAST(r);
    }
  }

  // remove egress references
  for (auto n : neighs) {
    auto ifindex = rtnl_neigh_get_ifindex(n);

    auto link = nl->get_link_by_ifindex(ifindex);
    auto vid = vlan->get_vid(link.get());

    struct nl_addr *s_mac = rtnl_link_get_addr(link.get());
    struct nl_addr *d_mac = rtnl_neigh_get_lladdr(n);

    if (nl->is_bridge_interface(ifindex)) {
      auto fdb_res = nl->search_fdb(vid, d_mac);

      assert(fdb_res.size() == 1);
      ifindex = rtnl_neigh_get_ifindex(fdb_res.front());
    }
    rv = del_l3_egress(ifindex, vid, s_mac, d_mac);

    if (rv < 0 and rv != -EEXIST) {
      // clean up
      LOG(ERROR) << __FUNCTION__ << ": del l3 egress failed for neigh "
                 << OBJ_CAST(n);
      // fallthrough
    }

    // drop reference
    rtnl_neigh_put(n);
  }

  return rv;
}

} // namespace basebox
