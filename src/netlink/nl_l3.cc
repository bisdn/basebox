/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <net/if.h>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include <gflags/gflags.h>
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

DECLARE_int32(port_untagged_vid);

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
    for (const auto &v : arg) {
      hash_combine(seed, v);
    }
    return seed;
  }
};

template <> struct hash<basebox::nh_stub> {
  using argument_type = basebox::nh_stub;
  using result_type = std::size_t;
  result_type operator()(argument_type const &nh) const noexcept {
    size_t seed = 0;
    if (nh.nh == nullptr) {
      hash_combine(seed, nullptr);
    } else {
      auto len = nl_addr_get_len(nh.nh);

      hash_combine(seed, nl_addr_get_family(nh.nh));
      hash_combine(seed, nl_addr_get_prefixlen(nh.nh));

      if (len > 0) {
        hash_combine(seed, std::string_view{reinterpret_cast<const char *>(
                                                nl_addr_get_binary_addr(nh.nh)),
                                            len});
      }
    }
    hash_combine(seed, nh.ifindex);
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

// key: source port_id, vid, src_mac, af ; value: refcount
std::unordered_set<std::tuple<int, uint16_t, rofl::caddress_ll, uint16_t>>
    termination_mac_entries;

// ECMP mapping
std::unordered_map<std::set<nh_stub>, l3_interface> nh_grp_to_l3_ecmp_mapping;

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

  int ifindex = rtnl_addr_get_ifindex(a);
  auto l = nl->get_link_by_ifindex(ifindex);
  if (l == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": no link for addr a=" << a;
    return -EINVAL;
  }
  struct rtnl_link *link = l.get();

  bool is_loopback = (rtnl_link_get_flags(link) & IFF_LOOPBACK);
  bool is_bridge = rtnl_link_is_bridge(link); // XXX TODO svi as well?
  uint16_t vid = vlan->get_vid(link);
  uint32_t vrf_id = vlan->get_vrf_id(vid, link);

  // if it isn't on loopback and not our interfaces, ignore it
  if (!is_loopback && !nl->is_switch_interface(link)) {
    VLOG(1) << __FUNCTION__ << ": ignoring " << link;
    return -EINVAL;
  }

  // checks if the bridge is already configured with an address
  int master_id = rtnl_link_get_master(link);
  if (master_id && is_bridge &&
      rtnl_link_get_addr(nl->get_link(master_id, AF_BRIDGE))) {
    VLOG(1) << __FUNCTION__ << ": ignoring address on " << link;
    return -EINVAL;
  }

  // XXX TODO split this into several functions
  if (!is_loopback) {
    int port_id = nl->get_port_id(link);
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

  if (prefixlen == 32) {
    rv = sw->l3_unicast_host_add(ipv4_dst, 0, false, update, vrf_id);
    if (rv < 0) {
      // TODO shall we remove the l3_termination mac?
      LOG(ERROR) << __FUNCTION__ << ": failed to setup l3 addr " << addr;
    }
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
                 << " (tagged=" << tagged << " to link " << link;
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

  int ifindex = rtnl_addr_get_ifindex(a);
  auto l = nl->get_link_by_ifindex(ifindex);
  if (l == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": no link for addr a=" << a;
    return -EINVAL;
  }
  struct rtnl_link *link = l.get();

  bool is_loopback = (rtnl_link_get_flags(link) & IFF_LOOPBACK);

  // if it isn't on loopback and not our interfaces, ignore it
  if (!is_loopback && !nl->is_switch_interface(link)) {
    VLOG(1) << __FUNCTION__ << ": ignoring " << link;
    return -EINVAL;
  }

  uint16_t vid = vlan->get_vid(link);

  if (!is_loopback) {
    int port_id = nl->get_port_id(link);
    auto addr = rtnl_link_get_addr(link);
    rofl::caddress_ll mac = libnl_lladdr_2_rofl(addr);

    rv = add_l3_termination(port_id, vid, mac, AF_INET6);
    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__
                 << ": failed to setup termination mac port_id=" << port_id
                 << ", vid=" << vid << " mac=" << mac << "; rv=" << rv;
      return rv;
    }
  }

  if (is_loopback) {
    rv = add_lo_addr_v6(a);
    return rv;
  }

  auto addr = rtnl_addr_get_local(a);
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
      search_neigh_cache(rtnl_addr_get_ifindex(a), addr, AF_INET6, &neigh) ==
      0) {
    VLOG(3) << __FUNCTION__ << ": list neigh size " << neigh.size();
    if (neigh.size() > 0)
      update = true;
  }

  // TODO support VRF on IPv6 addresses
  if (prefixlen == 128) {
    rv = sw->l3_unicast_host_add(ipv6_dst, 0, false, update, 0);
    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__ << ": failed to setup address " << a;
      return rv;
    }
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
                 << " (tagged=" << tagged << " to link " << link;
    }
  }

  VLOG(1) << __FUNCTION__ << ": added addr " << a;

  return rv;
}

int nl_l3::add_lo_addr_v6(struct rtnl_addr *a) {
  int rv = 0;
  auto addr = rtnl_addr_get_local(a);

  auto p = nl_addr_alloc(16);
  nl_addr_parse("::1/128", AF_INET6, &p);
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

  return rv;
}

int nl_l3::del_l3_addr(struct rtnl_addr *a) {
  assert(sw);
  assert(a);

  int rv = 0;
  int family = rtnl_addr_get_family(a);

  // The link needs to be obtained from the cache, as the link reference
  // may be outdated
  struct rtnl_link *link = nl->get_link(rtnl_addr_get_ifindex(a), AF_UNSPEC);

  // The link object might have already been purged from the cache if the
  // interface was deleted. Not much we can do here in that case until we start
  // keeping track of mac addresses ourselves.
  if (link == nullptr) {
    LOG(ERROR) << __FUNCTION__
               << ": unknown link ifindex=" << rtnl_addr_get_ifindex(a)
               << " of addr=" << a;
    return -EINVAL;
  }

  uint16_t vid = vlan->get_vid(link);
  uint32_t vrf_id = vlan->get_vrf_id(vid, link);

  if (a == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": addr can't be null";
    return -EINVAL;
  }

  bool is_loopback = (rtnl_link_get_flags(link) & IFF_LOOPBACK);

  // if it isn't on loopback and not our interfaces, ignore it
  if (!is_loopback && !nl->is_switch_interface(link)) {
    VLOG(1) << __FUNCTION__ << ": ignoring " << link;
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

    if (prefixlen == 32)
      rv = sw->l3_unicast_host_remove(ipv4_dst, vrf_id);
  } else {
    assert(family == AF_INET6);
    rofl::caddress_in6 ipv6_dst = libnl_in6addr_2_rofl(addr, &rv);
    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__ << ": could not parse addr " << addr;
      return rv;
    }

    if (prefixlen == 128)
      rv = sw->l3_unicast_host_remove(ipv6_dst, vrf_id);
  }

  if (link == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": no link for addr a=" << a;
    return -EINVAL;
  }

  if (is_loopback) {
    return 0;
  }

  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__
               << ": failed to remove l3 unicast host(local) rv=" << rv;
    return rv;
  }

  std::deque<rtnl_addr *> addresses;
  get_l3_addrs(link, &addresses, family);
  if (vid == FLAGS_port_untagged_vid && addresses.empty()) {
    struct rtnl_link *other;

    if (rtnl_link_is_vlan(link)) {
      // get the base link
      other = nl->get_link(rtnl_link_get_link(link), AF_UNSPEC);
    } else {
      // check if we have a vlan interface for VID 1
      other = nl->get_vlan_link(rtnl_link_get_ifindex(link), 1);
    }

    if (other != nullptr)
      get_l3_addrs(other, &addresses, family);
  }

  if (addresses.empty()) {
    int port_id = nl->get_port_id(link);

    addr = rtnl_link_get_addr(link);
    rofl::caddress_ll mac = libnl_lladdr_2_rofl(addr);

    rv = del_l3_termination(port_id, vid, mac, family);
    if (rv < 0 && rv != -ENODATA) {
      LOG(ERROR) << __FUNCTION__
                 << ": failed to remove l3 termination mac(local) vid=" << vid
                 << "; rv=" << rv;
    }
  }

  // del vlan
  // Avoid deleting table VLAN entry for the following two cases
  // Loopback: does not require entry on the Ingress table
  // Bridges and Bridge Interfaces: Entry set through bridge vlan table
  if (!is_loopback && !nl->is_bridge_interface(link)) {
    bool tagged = !!rtnl_link_is_vlan(link);
    rv = vlan->remove_vlan(link, vid, tagged);
    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__ << ": failed to remove vlan id " << vid
                 << " (tagged=" << tagged << " to link " << link;
    }
  }

  return rv;
}

int nl_l3::get_l3_addrs(struct rtnl_link *link,
                        std::deque<rtnl_addr *> *addresses, int af) {
  std::unique_ptr<rtnl_addr, decltype(&rtnl_addr_put)> filter(rtnl_addr_alloc(),
                                                              rtnl_addr_put);

  rtnl_addr_set_ifindex(filter.get(), rtnl_link_get_ifindex(link));
  if (af != AF_UNSPEC)
    rtnl_addr_set_family(filter.get(), af);

  VLOG(1) << __FUNCTION__ << ": searching l3 addresses from interface=" << link;

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
  std::unique_ptr<rtnl_route, decltype(&rtnl_route_put)> filter(
      rtnl_route_alloc(), rtnl_route_put);
  auto nh = rtnl_route_nh_alloc();

  rtnl_route_nh_set_ifindex(nh, rtnl_link_get_ifindex(link));
  rtnl_route_add_nexthop(filter.get(), nh);
  rtnl_route_set_type(filter.get(), RTN_UNICAST);

  VLOG(1) << __FUNCTION__ << ": searching l3 routes from interface=" << link;

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
    if (vrf_id)
      *vrf_id = vlan->get_vrf_id(vid, link.get());

    auto fdb_neigh = nl->search_fdb(vid, lladdr);
    for (auto neigh : fdb_neigh) {
      VLOG(2) << __FUNCTION__ << ": found iface=" << neigh;
      VLOG(2) << __FUNCTION__ << ": found link=" << link.get();

      auto neigh_ifindex = rtnl_neigh_get_ifindex(neigh);
      auto neigh_link = nl->get_link_by_ifindex(neigh_ifindex);

      rv = vlan->add_vlan(neigh_link.get(), vid, tagged);
      if (rv < 0) {
        LOG(ERROR) << __FUNCTION__
                   << ": failed to setup vid ingress vid=" << vid << " on link "
                   << link.get();
        return rv;
      }

      rv = add_l3_egress(neigh_ifindex, vid, s_mac, d_mac, l3_interface_id);

      if (rv < 0) {
        LOG(ERROR) << __FUNCTION__
                   << ": failed to setup vid ingress vid=" << vid << " on link "
                   << link.get();
      }
    }
    return rv;
  }

  // XXX TODO is this still needed?
  rv = vlan->add_vlan(link.get(), vid, tagged);
  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": failed to setup vid ingress vid=" << vid
               << " on link " << link.get();
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
  auto tagged = !!rtnl_link_is_vlan(link.get());
  auto s_mac = rtnl_link_get_addr(link.get());

  if (nl->is_bridge_interface(ifindex)) {
    auto fdb_res = nl->search_fdb(vid, d_mac);

    assert(fdb_res.size() <= 1);
    if (fdb_res.size() == 1) {
      ifindex = rtnl_neigh_get_ifindex(fdb_res.front());
    } else {
      // neighbor already purged from fdb, so check map
      uint32_t portid = 0;
      auto src_mac = libnl_lladdr_2_rofl(s_mac);
      auto dst_mac = libnl_lladdr_2_rofl(d_mac);

      for (auto it : l3_interface_mapping) {
        if (std::get<1>(it.first) == vid && std::get<2>(it.first) == src_mac &&
            std::get<3>(it.first) == dst_mac) {
          portid = std::get<0>(it.first);
          break;
        }
      }

      if (portid == 0) {
        LOG(ERROR) << __FUNCTION__
                   << ": failed to determine physical port of l3 egress entry "
                      "for neigh "
                   << n;
        return -ENODATA;
      }

      ifindex = nl->get_ifindex_by_port_id(portid);
      link = nl->get_link_by_ifindex(ifindex);
    }
  }

  // XXX TODO del vlan

  // remove egress group reference
  int rv = del_l3_egress(ifindex, vid, s_mac, d_mac);
  if (rv < 0 and rv != -EEXIST) {
    LOG(ERROR) << __FUNCTION__
               << ": failed to remove l3 egress ifindex=" << ifindex
               << " vid=" << vid;
  }
  return vlan->remove_vlan(link.get(), vid, tagged);
}

int nl_l3::add_l3_neigh(struct rtnl_neigh *n) {
  assert(n);

  int rv;
  uint32_t l3_interface_id = 0;
  struct nl_addr *addr;
  int family = rtnl_neigh_get_family(n);
  uint16_t vrf_id = 0;
  bool add_host_entry = true;

  if (n == nullptr)
    return -EINVAL;

  addr = rtnl_neigh_get_dst(n);
  if (family == AF_INET6) {
    auto p = nl_addr_alloc(16);
    nl_addr_parse("fe80::/10", AF_INET6, &p);
    std::unique_ptr<nl_addr, decltype(&nl_addr_put)> lo_addr(p, nl_addr_put);

    if (!nl_addr_cmp_prefix(addr, lo_addr.get())) {
      VLOG(1) << __FUNCTION__ << ": skipping fe80::/10";
      // we must not route IPv6 LL addresses, so do not add a host entry
      add_host_entry = false;
      rv = 0;
    }
  }

  if (add_host_entry) {
    // unless we have route, we should not route l3 neighbors
    struct nh_stub nh {
      addr, rtnl_neigh_get_ifindex(n)
    };

    if (is_l3_neigh_routable(n)) {
      routable_l3_neighs.emplace(nh);
    } else {
      unroutable_l3_neighs.emplace(nh);
      return -ENETUNREACH;
    }

    rv = add_l3_neigh_egress(n, &l3_interface_id, &vrf_id);
    LOG(INFO) << __FUNCTION__ << ": adding l3 neigh egress for neigh " << n;

    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__ << ": add l3 neigh egress failed for neigh "
                 << n;
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
      LOG(ERROR) << __FUNCTION__ << ": add l3 unicast host failed for " << n;
      return rv;
    }
  }

  for (auto cb = std::begin(nh_callbacks); cb != std::end(nh_callbacks);) {
    if (cb->second.nh.ifindex == rtnl_neigh_get_ifindex(n) &&
        nl_addr_cmp(cb->second.nh.nh, rtnl_neigh_get_dst(n)) == 0) {
      // XXX TODO add l3_interface?
      cb->first->nh_reachable_notification(n, cb->second);
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

  if (!nl->is_switch_interface(ifindex)) {
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

    rv = del_l3_neigh(n_old);
    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__ << ": failed to remove l3 neighbor " << n_old
                 << "; rv=" << rv;
      return rv;
    }
  } else if (nl_addr_cmp(n_ll_old, n_ll_new)) {
    // XXX TODO ll addr changed
    VLOG(1) << __FUNCTION__ << ": neighbour ll changed: new neighbor "
            << n_ll_new << " ifindex=" << ifindex;

    struct nh_stub nh {
      rtnl_neigh_get_dst(n_old), ifindex
    };
    if (unroutable_l3_neighs.count(nh) > 0) {
      VLOG(1) << __FUNCTION__ << ": ignoring update on unroutable neighbor";
      return -EINVAL;
    }

    auto link = nl->get_link_by_ifindex(ifindex);

    if (link.get() == nullptr) {
      LOG(ERROR) << __FUNCTION__ << ": link not found ifindex=" << ifindex;
      return -EINVAL;
    }

    auto s_mac = rtnl_link_get_addr(link.get());
    uint16_t vid = vlan->get_vid(link.get());

    if (nl->is_bridge_interface(ifindex)) {
      auto fdb_res = nl->search_fdb(vid, n_ll_new);

      assert(fdb_res.size() == 1);
      ifindex = rtnl_neigh_get_ifindex(fdb_res.front());
    }

    uint32_t port_id = nl->get_port_id(ifindex);

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
  bool skip_egress_remove = false;

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

  if (n == nullptr)
    return -EINVAL;

  int ifindex = rtnl_neigh_get_ifindex(n);
  auto link = nl->get_link_by_ifindex(ifindex);

  if (!link)
    return -EINVAL;

  struct nh_stub nh {
    addr, rtnl_neigh_get_ifindex(n)
  };
  if (unroutable_l3_neighs.erase(nh) > 0) {
    VLOG(2) << __FUNCTION__ << ": l3 neigh was disabled, nothing to do for "
            << n;
    return 0;
  }

  routable_l3_neighs.erase(nh);

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
      // we never added a host route, and therefore no egress interface
      skip_egress_remove = true;
    } else {
      rofl::caddress_in6 ipv6_dst = libnl_in6addr_2_rofl(addr, &rv);
      if (rv < 0) {
        LOG(ERROR) << __FUNCTION__ << ": could not parse addr " << addr;
        return rv;
      }

      // delete next hop
      rv = sw->l3_unicast_host_remove(ipv6_dst);
    }
  }

  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": failed to remove l3 unicast host ipv4_dst";
    return rv;
  }

  // delete l3 unicast group for host entry (if existed)
  if (!skip_egress_remove)
    rv = del_l3_neigh_egress(n);

  // if a route still exists thats pointing to the nexthop,
  // update the route to point to controller and delete the nexthop egress
  // reference
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
  auto it = termination_mac_entries.find(needle);
  if (it != termination_mac_entries.end())
    return 0;

  termination_mac_entries.emplace(std::move(needle));

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

  if (rv == 0)

    VLOG(3) << __FUNCTION__ << ": added l3 termination for port=" << port_id
            << " vid=" << vid << " mac=" << mac << " af=" << af;

  return rv;
}

int nl_l3::del_l3_termination(uint32_t port_id, uint16_t vid,
                              const rofl::caddress_ll &mac, int af) noexcept {
  int rv = 0;

  VLOG(4) << __FUNCTION__ << ": trying to delete for port_id=" << port_id
          << ", vid=" << vid << ", mac=" << mac << ", af=" << af;

  // lookup if this exists
  auto needle = std::make_tuple(port_id, vid, mac, static_cast<uint16_t>(af));
  auto it = termination_mac_entries.find(needle);
  if (it == termination_mac_entries.end()) {
    LOG(WARNING)
        << __FUNCTION__
        << ": tried to delete a non existing termination mac for port_id="
        << port_id << ", vid=" << vid << ", mac=" << mac << ", af=" << af;
    return -ENODATA;
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

  termination_mac_entries.erase(it);

  return rv;
}

int nl_l3::update_l3_termination(int port_id, uint16_t vid,
                                 struct nl_addr *old_mac,
                                 struct nl_addr *new_mac) noexcept {
  int rv = 0;

  auto o_mac = libnl_lladdr_2_rofl(old_mac);
  auto n_mac = libnl_lladdr_2_rofl(new_mac);

  // parse the AF list and remove the entry from the termination mac set
  // call the switch function to remove and insert the entry with the
  // new mac address.
  if (termination_mac_entries.find(std::make_tuple(
          port_id, vid, o_mac, AF_INET)) != termination_mac_entries.end()) {
    rv = del_l3_termination(port_id, vid, o_mac, AF_INET);
    if (rv < 0)
      VLOG(3) << __FUNCTION__
              << ": failed to remove termination mac port=" << port_id
              << " vid=" << vid << " mac=" << o_mac;
    rv = add_l3_termination(port_id, vid, n_mac, AF_INET);
    if (rv < 0) {
      VLOG(3) << __FUNCTION__
              << ": failed to add termination mac port=" << port_id
              << " vid=" << vid << " mac=" << n_mac;
      return rv;
    }

    VLOG(2) << __FUNCTION__
            << ": updated Termination MAC for port_id=" << port_id
            << " old mac address=" << o_mac << " new mac address=" << n_mac
            << " AF=" << AF_INET;
  }

  if (termination_mac_entries.find(std::make_tuple(
          port_id, vid, o_mac, AF_INET6)) != termination_mac_entries.end()) {
    rv = del_l3_termination(port_id, vid, o_mac, AF_INET6);
    if (rv < 0)
      VLOG(3) << __FUNCTION__
              << ": failed to remove termination mac port=" << port_id
              << " vid=" << vid << " mac=" << o_mac;
    rv = add_l3_termination(port_id, vid, n_mac, AF_INET6);
    if (rv < 0) {
      VLOG(3) << __FUNCTION__
              << ": failed to add termination mac port=" << port_id
              << " vid=" << vid << " mac=" << n_mac;
      return rv;
    }

    VLOG(2) << __FUNCTION__
            << ": updated Termination MAC for port_id=" << port_id
            << " old mac address=" << o_mac << " new mac address=" << n_mac
            << " AF=" << AF_INET6;
  }

  return rv;
}

int nl_l3::update_l3_egress(int port_id, uint16_t vid, struct nl_addr *old_mac,
                            struct nl_addr *new_mac) noexcept {

  int rv = 0;

  auto o_mac = libnl_lladdr_2_rofl(old_mac);
  auto n_mac = libnl_lladdr_2_rofl(new_mac);

  // search the l3 interface mapping for an entry corresponding to the
  // old mac address to be updated
  std::unordered_map<
      std::tuple<int, uint16_t, rofl::caddress_ll, rofl::caddress_ll>,
      l3_interface>
      update_l3;

  // vlan, mac address combination is unique, get the entries that match
  // not parsing for the port id solves the issue for bridge interfaces
  // that portid is 0 and the entries are gotten from the fdb
  for (auto it : l3_interface_mapping) {
    if (std::get<1>(it.first) == vid && std::get<2>(it.first) == o_mac)
      update_l3.emplace(it.first, it.second);
  }

  if (update_l3.empty()) {
    VLOG(4) << __FUNCTION__
            << ": no mapping found, no egress entry to be updated";
    return 0;
  }

  // update the switch egress entry with the new mac address
  for (auto it : update_l3) {
    uint32_t l3_iface_id = it.second.l3_interface_id;
    rv = sw->l3_egress_update(std::get<0>(it.first), vid, n_mac,
                              std::get<3>(it.first), &l3_iface_id);

    auto l3_if_new_tuple =
        std::make_tuple(port_id, vid, n_mac, std::get<3>(it.first));

    // updates the l3 interface map, erasing the entry to the old mac address
    // and inserting the new one
    l3_interface_mapping.erase(it.first);
    l3_interface_mapping.emplace(std::make_pair(l3_if_new_tuple, it.second));

    VLOG(2) << __FUNCTION__ << ": updated egress l3 for port_id=" << port_id
            << " old mac address=" << std::get<3>(it.first)
            << " new mac address=" << n_mac
            << " l3 interface id=" << l3_iface_id;
  }

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
    auto link = nl->get_link_by_ifindex(ifindex);

    // Guarantee that the interface is found
    if (!link.get())
      continue;

    if (!nl->is_switch_interface(link.get())) {
      VLOG(1) << __FUNCTION__ << ": ignoring next hop " << nh;
      continue;
    }

    nhs->push_back(nh);
  }
}

int nl_l3::get_neighbours_of_route(rtnl_route *route,
                                   std::set<nh_stub> *nhs) noexcept {

  assert(route);
  assert(nhs);

  uint32_t nhid = rtnl_route_get_nhid(route);

  if (rtnl_route_get_nnexthops(route) > 0) {
    std::deque<struct rtnl_nexthop *> rnhs;
    get_nexthops_of_route(route, &rnhs);

    if (rnhs.size() == 0)
      return -ENETUNREACH;

    for (auto nh : rnhs) {
      int ifindex = rtnl_route_nh_get_ifindex(nh);
      nl_addr *nh_addr = rtnl_route_nh_get_gateway(nh);

      if (!nh_addr)
        nh_addr = rtnl_route_nh_get_via(nh);

      if (nh_addr) {
        switch (nl_addr_get_family(nh_addr)) {
        case AF_INET:
        case AF_INET6:
          VLOG(2) << __FUNCTION__ << ": ifindex=" << ifindex
                  << " gw=" << nh_addr;
          break;
        default:
          LOG(ERROR) << "gw " << nh_addr
                     << " unsupported family=" << nl_addr_get_family(nh_addr);
          continue;
        }

        nhs->emplace(nh_stub{nh_addr, ifindex, nhid});
      }
    }
  } else {
    std::deque<struct rtnl_nh *> rnhs;

    if (nhid > 0) {
      auto route_nh = nl->get_nh_by_id(nhid);
      if (route_nh == nullptr)
        return -EINVAL;

      int group_size = rtnl_nh_get_group_size(route_nh);
      if (group_size > 0) {
        for (int i = 0; i < group_size; i++) {
          auto nh = nl->get_nh_by_id(rtnl_nh_get_group_entry(route_nh, i));
          if (!nh)
            continue;

          rnhs.push_back(nh);
        }
        // no need for the route nh anymore
        rtnl_nh_put(route_nh);
      } else {
        rnhs.push_back(route_nh);
      }
    } else {
      return -EINVAL;
    }

    for (auto nh : rnhs) {
      int ifindex = rtnl_nh_get_oif(nh);
      auto nh_addr = rtnl_nh_get_gateway(nh);

      auto link = nl->get_link_by_ifindex(ifindex);
      if (!link.get())
        continue;

      if (!nl->is_switch_interface(link.get())) {
        VLOG(1) << __FUNCTION__ << ": ignoring next hop " << nh;
        continue;
      }

      if (nh_addr) {
        switch (nl_addr_get_family(nh_addr)) {
        case AF_INET:
        case AF_INET6:
          VLOG(2) << __FUNCTION__ << ": ifindex=" << ifindex
                  << " gw=" << nh_addr;
          break;
        default:
          LOG(ERROR) << "gw " << nh_addr
                     << " unsupported family=" << nl_addr_get_family(nh_addr);
          continue;
        }
      }

      nhs->emplace(nh_stub{nh_addr, ifindex, nhid});
    }

    for (auto nh : rnhs)
      rtnl_nh_put(nh);
  }

  return nhs->size();
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
    VLOG(1) << __FUNCTION__ << ": l3_interface_id entry not found for port "
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

int nl_l3::get_l3_interface_id(const nh_stub &nh, uint32_t *l3_interface_id) {
  rtnl_neigh *neigh = nl->get_neighbour(nh.ifindex, nh.nh);

  VLOG(2) << __FUNCTION__ << ": get neigh=" << neigh << " of nh_addr=" << nh.nh;

  if (!neigh || rtnl_neigh_get_lladdr(neigh) == nullptr) {
    VLOG(2) << __FUNCTION__ << ": neigh=" << neigh << " is not reachable";
    rtnl_neigh_put(neigh);
    return -EHOSTUNREACH;
  }

  auto ifindex = nh.ifindex;
  auto link = nl->get_link_by_ifindex(nh.ifindex);
  auto vid = vlan->get_vid(link.get());
  struct nl_addr *s_mac = rtnl_link_get_addr(link.get());
  struct nl_addr *d_mac = rtnl_neigh_get_lladdr(neigh);

  // For the Bridge SVI, the l3 interface already exists
  // so we can just get that one
  if (nl->is_bridge_interface(ifindex)) {
    auto fdb_res = nl->search_fdb(vid, d_mac);

    assert(fdb_res.size() == 1);
    ifindex = rtnl_neigh_get_ifindex(fdb_res.front());
  }
  auto rv = get_l3_interface_id(ifindex, s_mac, d_mac, l3_interface_id, vid);
  rtnl_neigh_put(neigh);

  return rv;
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

void nl_l3::nh_reachable_notification(struct rtnl_neigh *n,
                                      struct nh_params p) noexcept {
  auto route = nl->get_route_by_nh_params(p);
  if (route == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": failed to find route for dst=" << p.np.addr
               << " on ifindex=" << p.nh.ifindex << " for nh" << p.nh.nh;
    return;
  }

  std::set<nh_stub> nhs;
  std::set<uint32_t> l3_interface_ids;
  get_neighbours_of_route(route.get(), &nhs);

  uint32_t route_dst;
  uint32_t vrf_id = rtnl_route_get_table(route.get());
  if (vrf_id == MAIN_ROUTING_TABLE)
    vrf_id = 0;

  for (auto rnh : nhs) {
    uint32_t l3_interface_id;
    int rv;

    if (rnh == p.nh) {
      rv = add_l3_neigh_egress(n, &l3_interface_id);
    } else {
      rv = get_l3_interface_id(rnh, &l3_interface_id);
    }

    if (rv == 0)
      l3_interface_ids.insert(l3_interface_id);
  }

  notify_on_nh_unreachable(this, p);

  assert(l3_interface_ids.size() > 0);

  if (nhs.size() > 1) {
    get_l3_ecmp_group(nhs, &route_dst);
    sw->l3_ecmp_update(route_dst, l3_interface_ids);
  } else {
    route_dst = *l3_interface_ids.begin();
  }

  add_l3_unicast_route(p.np.addr, route_dst, nhs.size() > 1, true, vrf_id);
}

void nl_l3::nh_unreachable_notification(struct rtnl_neigh *n,
                                        struct nh_params p) noexcept {
  auto route = nl->get_route_by_nh_params(p);
  if (route == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": failed to find route for dst=" << p.np.addr
               << " on ifindex=" << p.nh.ifindex << " for nh" << p.nh.nh;
    // let's assume we failed to removed the notification, but the route is
    // already removed
    del_l3_neigh_egress(n);
    return;
  }

  uint32_t vrf_id = rtnl_route_get_table(route.get());
  if (vrf_id == MAIN_ROUTING_TABLE)
    vrf_id = 0;

  std::set<nh_stub> nhs;
  std::set<uint32_t> l3_interface_ids;
  get_neighbours_of_route(route.get(), &nhs);

  uint32_t route_dst;

  for (auto rnh : nhs) {
    uint32_t l3_interface_id;

    if (rnh == p.nh)
      continue;

    auto rv = get_l3_interface_id(rnh, &l3_interface_id);
    if (rv == 0)
      l3_interface_ids.insert(l3_interface_id);
  }

  if (nhs.size() > 1) {
    get_l3_ecmp_group(nhs, &route_dst);
    sw->l3_ecmp_update(route_dst, l3_interface_ids);
  }

  if (l3_interface_ids.size() == 0)
    route_dst = 0;

  add_l3_unicast_route(p.np.addr, route_dst, nhs.size() > 1, true, vrf_id);
  del_l3_neigh_egress(n);
  notify_on_nh_reachable(this, p);
}

int nl_l3::add_l3_unicast_route(rtnl_route *r, bool update_route) {
  assert(r);
  uint32_t vrf_id = rtnl_route_get_table(r);
  if (vrf_id == MAIN_ROUTING_TABLE)
    vrf_id = 0;

  VLOG(2) << __FUNCTION__ << ": adding route= " << r
          << " update=" << update_route;

  // FRR may occationally install link-local routes again with a different
  // priority. Since we cannot handle multiple routes with different
  // priorities/metrics yet, ignore the duplicated route.
  if (rtnl_route_get_priority(r) > 0 &&
      rtnl_route_get_protocol(r) != RTPROT_KERNEL) {
    nl_route_query rq;

    auto route = rq.query_route(rtnl_route_get_dst(r), RTM_F_FIB_MATCH);

    if (route) {
      bool duplicate = false;
      VLOG(2) << __FUNCTION__ << ": got route " << route << " for dst "
              << rtnl_route_get_dst(r);
      if (rtnl_route_get_protocol(route) == RTPROT_KERNEL &&
          nl_addr_cmp_prefix(rtnl_route_get_dst(r),
                             rtnl_route_get_dst(route)) == 0) {
        // we already have a kernel route for the same dst
        duplicate = true;
      }
      nl_object_put(OBJ_CAST(route));

      // there already is a kernel route, so ignore this one
      if (duplicate)
        return 0;
    } else {
      // huh?
      VLOG(2) << __FUNCTION__ << ": no route for dst " << rtnl_route_get_dst(r);
    }
  }

  std::set<nh_stub> nhs;
  int rv = get_neighbours_of_route(r, &nhs);

  // no neighbours reachable
  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": no next hops for route " << r;
    return rv;
  }

  std::deque<struct rtnl_neigh *> neighs;
  // each route increments the ref counter for all nexthops (l3 interface)
  std::set<uint32_t> l3_interfaces; // all create l3 interface ids
  int unresolved_nhs = 0;

  for (auto nh : nhs) {
    uint32_t l3_interface_id = 0;
    auto n = nl->get_neighbour(nh.ifindex, nh.nh);
    VLOG(2) << __FUNCTION__ << ": get neigh=" << n << " of nh_addr=" << nh.nh;

    if (n == nullptr || rtnl_neigh_get_lladdr(n) == nullptr) {
      VLOG(2) << __FUNCTION__ << ": got unresolved nh ifindex=" << nh.ifindex
              << ", nh=" << nh.nh;
      unresolved_nhs++;

      // If the next-hop is currently unresolved, we store the route and
      // process it when the nh is found
      if (!is_ipv6_link_local_address(rtnl_route_get_dst(r)) &&
          !is_ipv6_multicast_address(rtnl_route_get_dst(r)))
        notify_on_nh_reachable(
            this, nh_params{net_params{rtnl_route_get_dst(r), nh.ifindex}, nh});
      if (n)
        rtnl_neigh_put(n);
      continue;
    }

    // add neigh
    rv = add_l3_neigh_egress(n, &l3_interface_id);
    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__ << ": add l3 egress failed for neigh " << n;
      // XXX TODO create l3 neigh later
      continue;
    }

    VLOG(2) << __FUNCTION__ << ": got l3_interface_id=" << l3_interface_id;
    l3_interfaces.emplace(l3_interface_id);

    notify_on_nh_unreachable(
        this, nh_params{net_params{rtnl_route_get_dst(r), nh.ifindex}, nh});
  }

  VLOG(2) << __FUNCTION__ << ": got " << l3_interfaces.size() << " neighbours ("
          << unresolved_nhs << " unresolved next hops)";

  uint32_t route_dst_interface;

  if (nhs.size() <= 1) {
    if (l3_interfaces.size() == 0) {
      // Not yet resolved next-hop, or on-link route, the rule must be installed
      // to the switch pointing to controller and then updated after neighbor
      // registration.
      route_dst_interface = 0;
    } else {
      // single, resolved nexthop
      route_dst_interface = *l3_interfaces.begin(); /* resolved nexthop */
    }
  } else {
    // ecmp route
    add_l3_ecmp_group(nhs, &route_dst_interface);

    if (l3_interfaces.size() == 0) {
      // No yet resolved next-hops, the rule must be installed
      // to the switch pointing to controller and then updated after neighbor
      // registration.
      route_dst_interface = 0;
    } else {
      // At least one resolved nexthop, update ecmp group
      sw->l3_ecmp_update(route_dst_interface, l3_interfaces);
    }
  }

  rv = add_l3_unicast_route(rtnl_route_get_dst(r), route_dst_interface,
                            nhs.size() > 1, update_route, vrf_id);
  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__
               << ": failed to add route to dst=" << rtnl_route_get_dst(r)
               << " rv=" << rv;
  }

  if (rtnl_route_guess_scope(r) == RT_SCOPE_LINK) {
    VLOG(2) << __FUNCTION__ << ": enabling l3 neighs reachable by route " << r;
    auto l3_neighs =
        get_l3_neighs_of_prefix(unroutable_l3_neighs, rtnl_route_get_dst(r));
    for (auto n : l3_neighs) {
      auto neigh = nl->get_neighbour(n.ifindex, n.nh);
      if (!neigh) {
        // should we remove it from unroutable?
        VLOG(2) << __FUNCTION__ << ": unknown l3 neigh ifindex=" << n.ifindex
                << ", addr=" << n.nh;
        continue;
      }

      VLOG(2) << __FUNCTION__ << ": enabling l3 neigh " << neigh;
      unroutable_l3_neighs.erase(n);
      add_l3_neigh(neigh);
      rtnl_neigh_put(neigh);
    }
  }

  // cleanup
  for (auto n : neighs)
    rtnl_neigh_put(n);

  if (!update_route) {
    // check for reachable addresses
    for (auto cb = std::begin(net_callbacks); cb != std::end(net_callbacks);) {
      if (nl_addr_cmp_prefix(cb->second.addr, rtnl_route_get_dst(r)) == 0) {
        cb->first->net_reachable_notification(cb->second);
        cb = net_callbacks.erase(cb);
      } else {
        ++cb;
      }
    }
  }

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

  if (rtnl_route_get_priority(r) > 0 &&
      rtnl_route_get_protocol(r) != RTPROT_KERNEL) {
    nl_route_query rq;

    auto route = rq.query_route(rtnl_route_get_dst(r), RTM_F_FIB_MATCH);

    if (route) {
      bool duplicate = false;
      VLOG(2) << __FUNCTION__ << ": got route " << route << " for dst "
              << rtnl_route_get_dst(r);
      if (rtnl_route_get_protocol(route) == RTPROT_KERNEL &&
          nl_addr_cmp_prefix(rtnl_route_get_dst(r),
                             rtnl_route_get_dst(route)) == 0) {
        // we have a kernel route for the same dst
        duplicate = true;
      }
      nl_object_put(OBJ_CAST(route));

      // there is still a kernel route, so ignore this one
      if (duplicate)
        return 0;
    } else {
      // no route anymore, this is fine
      VLOG(2) << __FUNCTION__ << ": no route for dst " << rtnl_route_get_dst(r);
    }
  }

  if (!keep_route) {
    // OpenFlow / OF-DPA does not support per-interface routes, so we can only
    // have one IPv6LL route. Therefore make sure we only delete it if there are
    // no interfaces left with one.
    if (is_ipv6_link_local_address(dst)) {
      std::unique_ptr<rtnl_route, decltype(&rtnl_route_put)> filter(
          rtnl_route_alloc(), rtnl_route_put);
      std::deque<rtnl_route *> routes;

      rtnl_route_set_type(filter.get(), RTN_UNICAST);
      rtnl_route_set_dst(filter.get(), dst);
      rtnl_route_set_table(filter.get(), rtnl_route_get_table(r));

      nl_cache_foreach_filter(
          nl->get_cache(cnetlink::NL_ROUTE_CACHE), OBJ_CAST(filter.get()),
          [](struct nl_object *o, void *arg) {
            auto *routes = (std::deque<rtnl_route *> *)arg;
            auto route = ROUTE_CAST(o);

            if (rtnl_route_get_nnexthops(route) != 1)
              return;

            if (rtnl_route_guess_scope(route) != RT_SCOPE_LINK)
              return;

            nl_object_get(o);
            routes->push_back(route);
          },
          &routes);

      for (auto route : routes) {
        auto nh = rtnl_route_nexthop_n(route, 0);
        if (!keep_route &&
            nl->is_switch_interface(rtnl_route_nh_get_ifindex(nh))) {
          keep_route = true;
        }

        rtnl_route_put(route);
      }
    }

    if (keep_route) {
      VLOG(2) << __FUNCTION__
              << ": IPv6LL route still present on other interfaces, not "
                 "removing it";
      return 0;
    }

    rv = del_l3_unicast_route(dst, vrf_id);
    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__ << ": failed to remove dst=" << dst;
      // fallthrough
    }
  }

  if (rtnl_route_guess_scope(r) == RT_SCOPE_LINK) {
    VLOG(2) << __FUNCTION__ << ": disabling l3 neighs reachable by route " << r;
    auto l3_neighs =
        get_l3_neighs_of_prefix(routable_l3_neighs, rtnl_route_get_dst(r));
    for (auto n : l3_neighs) {
      auto neigh = nl->get_neighbour(n.ifindex, n.nh);
      if (!neigh) {
        // neigh is already purged from nl cache, so neigh will be
        // removed when handling the DEL_NEIGH notification
        VLOG(2) << __FUNCTION__
                << ": skipping non-existing l3 neigh ifindex=" << n.ifindex
                << ", addr=" << n.nh;
        continue;
      }

      VLOG(2) << __FUNCTION__ << ": disabling l3 neigh " << neigh;
      del_l3_neigh(neigh);
      rtnl_neigh_put(neigh);
      unroutable_l3_neighs.emplace(n);
    }
  }

  std::deque<struct rtnl_neigh *> neighs;
  std::set<nh_stub> nhs;
  rv = get_neighbours_of_route(r, &nhs);

  // no neighbours reachable
  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": no next hops for route " << r;
    return rv;
  }

  VLOG(2) << __FUNCTION__ << ": number of next hops is " << rv;

  if (nhs.size() > 1)
    del_l3_ecmp_group(nhs);

  for (auto nh : nhs) {
    auto neigh = nl->get_neighbour(nh.ifindex, nh.nh);
    VLOG(2) << __FUNCTION__ << ": get neigh=" << neigh
            << " of nh_addr=" << nh.nh;

    if (neigh == nullptr || rtnl_neigh_get_lladdr(neigh) == nullptr) {
      VLOG(2) << __FUNCTION__ << ": got unresolved nh ifindex=" << nh.ifindex
              << ", nh=" << nh.nh;
      // Cleanup the unresolved route on deletion. Make sure to only remove the
      // first hit, as we want to keep the second one that will have been added
      // by add_l3_unicast_route() from update_l3_unicact_route().
      // If this is an actual deletion, there should only be one anyway.
      auto it = std::find_if(nh_callbacks.begin(), nh_callbacks.end(),
                             [&](std::pair<nh_reachable *, nh_params> &cb) {
                               return cb.first == this && cb.second.nh == nh &&
                                      !nl_addr_cmp(cb.second.np.addr, dst);
                             });

      if (it != nh_callbacks.end())
        nh_callbacks.erase(it);
      if (neigh)
        rtnl_neigh_put(neigh);
    } else {
      auto it =
          std::find_if(nh_unreach_callbacks.begin(), nh_unreach_callbacks.end(),
                       [&](std::pair<nh_unreachable *, nh_params> &cb) {
                         return cb.first == this && cb.second.nh == nh &&
                                !nl_addr_cmp(cb.second.np.addr, dst);
                       });

      if (it != nh_unreach_callbacks.end())
        nh_unreach_callbacks.erase(it);
      // remove egress reference
      rv = del_l3_neigh_egress(neigh);

      if (rv < 0 and rv != -EEXIST) {
        // clean up
        LOG(ERROR) << __FUNCTION__ << ": del l3 egress failed for neigh "
                   << neigh;
        // fallthrough
      }
      rtnl_neigh_put(neigh);
    }
  }

  return rv;
}

int nl_l3::add_l3_ecmp_group(const std::set<nh_stub> &nhs,
                             uint32_t *l3_ecmp_id) {
  std::set<uint32_t> empty;

  auto it = nh_grp_to_l3_ecmp_mapping.find(nhs);
  if (it != nh_grp_to_l3_ecmp_mapping.end()) {
    it->second.refcnt++;
    *l3_ecmp_id = it->second.l3_interface_id;

    VLOG(2) << __FUNCTION__ << ": found ecmp id: " << it->second.l3_interface_id
            << ", refcnt=" << it->second.refcnt;
  } else {
    sw->l3_ecmp_add(l3_ecmp_id, empty);
    nh_grp_to_l3_ecmp_mapping.emplace(
        std::make_pair(nhs, l3_interface(*l3_ecmp_id)));
    VLOG(2) << __FUNCTION__ << ": created ecmp id: " << *l3_ecmp_id;
  }

  return 0;
}

int nl_l3::get_l3_ecmp_group(const std::set<nh_stub> &nhs,
                             uint32_t *l3_ecmp_id) {
  auto it = nh_grp_to_l3_ecmp_mapping.find(nhs);

  if (it != nh_grp_to_l3_ecmp_mapping.end()) {
    *l3_ecmp_id = it->second.l3_interface_id;
    VLOG(2) << __FUNCTION__ << ": found ecmp id: " << it->second.l3_interface_id
            << ", refcnt=" << it->second.refcnt;
  } else {
    VLOG(2) << __FUNCTION__ << ": failed to find ecmp id";
    return -EINVAL;
  }

  return 0;
}

int nl_l3::del_l3_ecmp_group(const std::set<nh_stub> &nhs) {
  auto it = nh_grp_to_l3_ecmp_mapping.find(nhs);
  if (it == nh_grp_to_l3_ecmp_mapping.end()) {
    VLOG(2) << __FUNCTION__ << ": failed to find ecmp id";
    return -EINVAL;
  }

  it->second.refcnt--;
  VLOG(2) << __FUNCTION__ << ": found ecmp id: " << it->second.l3_interface_id
          << ", refcnt=" << it->second.refcnt;

  if (it->second.refcnt <= 0) {
    int rv = sw->l3_ecmp_remove(it->second.l3_interface_id);
    nh_grp_to_l3_ecmp_mapping.erase(nhs);
    return rv;
  }

  return 0;
}

bool nl_l3::is_l3_neigh_routable(struct rtnl_neigh *n) {
  bool routable = false;
  nl_route_query rq;

  auto route = rq.query_route(rtnl_neigh_get_dst(n));
  if (route) {
    VLOG(2) << __FUNCTION__ << ": got route " << route << " for neigh " << n;

    if (rtnl_route_guess_scope(route) == RT_SCOPE_LINK) {
      VLOG(2) << __FUNCTION__ << ": neigh is routable in by us";
      routable = true;
    }
    nl_object_put(OBJ_CAST(route));
  } else {
    VLOG(2) << __FUNCTION__ << ": no route for neigh " << n;
  }

  return routable;
}

std::deque<nh_stub> nl_l3::get_l3_neighs_of_prefix(std::set<nh_stub> &list,
                                                   nl_addr *prefix) {
  std::deque<nh_stub> ret;

  auto l3_neighs =
      std::equal_range(list.begin(), list.end(), prefix, l3_prefix_comp);
  std::deque<nh_stub> nhs;
  for (auto n = l3_neighs.first; n != l3_neighs.second; ++n) {
    ret.push_back(*n);
  }
  return ret;
}

} // namespace basebox
