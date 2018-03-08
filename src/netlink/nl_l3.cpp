#include <tuple>
#include <unordered_map>

#include <glog/logging.h>
#include <netlink/route/addr.h>
#include <netlink/route/link.h>
#include <netlink/route/neighbour.h>
#include <netlink/route/route.h>

#include "cnetlink.hpp"
#include "nl_hashing.hpp"
#include "nl_l3.hpp"
#include "nl_output.hpp"
#include "nl_vlan.hpp"
#include "sai.hpp"
#include "tap_manager.hpp"

namespace std {

template <> struct hash<rofl::caddress_ll> {
  typedef rofl::caddress_ll argument_type;
  typedef std::size_t result_type;
  result_type operator()(argument_type const &lla) const noexcept {
    return std::hash<uint64_t>{}(lla.get_mac());
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

// <port_id, vid, src_mac, dst_mac>
std::unordered_map<
    std::tuple<int, uint16_t, rofl::caddress_ll, rofl::caddress_ll>,
    l3_interface>
    l3_interface_mapping; // XXX FIXME use unordered_multimap

nl_l3::nl_l3(std::shared_ptr<tap_manager> tap_man,
             std::shared_ptr<nl_vlan> vlan, cnetlink *nl)
    : sw(nullptr), tap_man(tap_man), vlan(vlan), nl(nl) {}

rofl::caddress_ll libnl_lladdr_2_rofl(const struct nl_addr *lladdr) {
  // XXX check for family
  return rofl::caddress_ll((uint8_t *)nl_addr_get_binary_addr(lladdr),
                           nl_addr_get_len(lladdr));
}

rofl::caddress_in4 libnl_in4addr_2_rofl(struct nl_addr *addr) {
  struct sockaddr_in sin;
  socklen_t salen = sizeof(sin);
  nl_addr_fill_sockaddr(addr, (struct sockaddr *)&sin, &salen);
  return rofl::caddress_in4(&sin, salen);
}

int nl_l3::add_l3_termination(struct rtnl_addr *a) {
  assert(sw);
  assert(a);

  sw->subscribe_to(switch_interface::SWIF_ARP);

  int rv = 0;
  uint16_t vid = 1;
  bool tagged = false;

  if (a == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": addr can't be null";
    return -EINVAL;
  }

  int ifindex = rtnl_addr_get_ifindex(a);
  int port_id = tap_man->get_port_id(ifindex);

  if (port_id == 0) {
    LOG(ERROR) << __FUNCTION__ << ": invalid port_id 0";
    return -EINVAL;
  }

  struct rtnl_link *link = rtnl_addr_get_link(a);
  if (link == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": no link for addr a=" << a;
    return -EINVAL;
  }

  struct nl_addr *addr = rtnl_link_get_addr(link);
  rofl::caddress_ll mac = libnl_lladdr_2_rofl(addr);

  rv = sw->l3_termination_add(port_id, vid, mac);
  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__
               << ": failed to setup termination mac port_id=" << port_id
               << ", vid=" << vid << " mac=" << mac << "; rv=" << rv;
    return rv;
  }

  // get v4 dst (local v4 addr)
  addr = rtnl_addr_get_local(a);
  rofl::caddress_in4 ipv4_dst = libnl_in4addr_2_rofl(addr);
  rv = sw->l3_unicast_host_add(ipv4_dst,
                               0); // TODO likely move this to separate entity
  if (rv < 0) {
    // TODO shall we remove the l3_termination mac?
    LOG(ERROR) << __FUNCTION__
               << ": failed to setup l3 neigh port_id=" << port_id
               << ", vid=" << vid << "; rv=" << rv;
  }

  // add vlan
  rv = vlan->add_vlan(ifindex, vid, tagged);
  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": failed to add vlan id " << vid
               << " (tagged=" << tagged << " to link " << OBJ_CAST(link);
  }

  return rv;
}

int nl_l3::del_l3_termination(struct rtnl_addr *a) {
  assert(sw);
  assert(a);

  int rv = 0;
  uint16_t vid = 1;

  if (a == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": addr can't be null";
    return -EINVAL;
  }

  struct nl_addr *addr = rtnl_addr_get_local(a);
  rofl::caddress_in4 ipv4_dst = libnl_in4addr_2_rofl(addr);

  // XXX TODO remove vlan adderss

  rv = sw->l3_unicast_host_remove(ipv4_dst);
  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__
               << ": failed to remove l3 unicast host(local) vid=" << vid
               << "; rv=" << rv;
    return rv;
  }

  int ifindex = rtnl_addr_get_ifindex(a);
  int port_id = tap_man->get_port_id(ifindex);

  if (port_id == 0) {
    VLOG(1) << __FUNCTION__ << ": invalid port_id 0";
    return -EINVAL;
  }

  struct rtnl_link *link = rtnl_addr_get_link(a);
  if (link == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": no link for addr a=" << a;
    return -EINVAL;
  }

  addr = rtnl_link_get_addr(link);
  rofl::caddress_ll mac = libnl_lladdr_2_rofl(addr);

  // XXX TODO don't do this in case more l3 addresses are on this link
  rv = sw->l3_termination_remove(port_id, vid, mac);
  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__
               << ": failed to remove l3 termination mac(local) vid=" << vid
               << "; rv=" << rv;
  }

  return rv;
}

int nl_l3::add_l3_neigh(struct rtnl_neigh *n) {
  int rv;

  assert(n);

  if (n == nullptr)
    return -EINVAL;

  LOG(INFO) << __FUNCTION__ << ": n=" << OBJ_CAST(n);

  int state = rtnl_neigh_get_state(n);
  if (state == NUD_FAILED) {
    LOG(INFO) << __FUNCTION__ << ": neighbour not reachable state=failed";
    return -EINVAL;
  }

  int vid = 1; // XXX TODO currently only on vid 1
  assert(vid);
  struct nl_addr *addr = rtnl_neigh_get_lladdr(n);
  rofl::caddress_ll dst_mac = libnl_lladdr_2_rofl(addr);
  int ifindex = rtnl_neigh_get_ifindex(n);
  uint32_t port_id = tap_man->get_port_id(ifindex);

  if (port_id == 0) {
    LOG(ERROR) << __FUNCTION__ << ": invalid port_id=" << port_id;
    return -EINVAL;
  }

  assert(nl);
  std::unique_ptr<rtnl_link, void (*)(rtnl_link *)> link(
      nl->get_link_by_ifindex(ifindex), &rtnl_link_put);

  if (link == nullptr)
    return -EINVAL;

  addr = rtnl_link_get_addr(link.get());
  rofl::caddress_ll src_mac = libnl_lladdr_2_rofl(addr);
  link = nullptr;

  addr = rtnl_neigh_get_dst(n);
  rofl::caddress_in4 ipv4_dst = libnl_in4addr_2_rofl(addr);

  rv = vlan->add_vlan(ifindex, 1, false);
  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": failed to add vlan to interface "
               << OBJ_CAST(link.get());
  }

  // setup egress L3 Unicast group
  uint32_t l3_interface_id = 0;
  auto it = l3_interface_mapping.find(
      std::make_tuple(port_id, vid, src_mac, dst_mac));

  if (it == l3_interface_mapping.end()) {
    rv = sw->l3_egress_create(port_id, vid, src_mac, dst_mac, &l3_interface_id);

    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__
                 << ": failed to setup l3 egress port_id=" << port_id
                 << ", vid=" << vid << ", src_mac=" << src_mac
                 << ", dst_mac=" << dst_mac << "; rv=" << rv;
      return rv;
    }

    auto rv = l3_interface_mapping.emplace(
        std::make_pair(std::make_tuple(port_id, vid, src_mac, dst_mac),
                       l3_interface(l3_interface_id)));

    if (!rv.second) {
      LOG(FATAL) << __FUNCTION__
                 << ": failed to store l3_interface_id port_id=" << port_id
                 << ", vid=" << vid << ", src_mac=" << src_mac
                 << ", dst_mac=" << dst_mac;
    }
  } else {
    assert(it->second.l3_interface_id);
    l3_interface_id = it->second.l3_interface_id;
    it->second.refcnt++;
  }

  // setup next hop
  rv = sw->l3_unicast_host_add(ipv4_dst, l3_interface_id);

  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__
               << ": failed to setup l3 neigh port_id=" << port_id
               << ", vid=" << vid << ", l3_interface_id=" << l3_interface_id
               << "; rv=" << rv;
    return rv;
  }

  return 0;
}

int nl_l3::update_l3_neigh(struct rtnl_neigh *n_old, struct rtnl_neigh *n_new) {
  assert(n_old);
  assert(n_new);

  int rv = 0;
  bool state_changed =
      !(rtnl_neigh_get_state(n_old) == rtnl_neigh_get_state(n_new));
  struct nl_addr *n_ll_old;
  struct nl_addr *n_ll_new;

  int ifindex = rtnl_neigh_get_ifindex(n_old);
  uint32_t port_id = tap_man->get_port_id(ifindex);

  if (port_id == 0) {
    VLOG(1) << __FUNCTION__ << ": invalid port id=" << port_id;
    return -EINVAL;
  }

  if (!state_changed) {
    VLOG(2) << __FUNCTION__ << ": neighbour state not changed states are n_old="
            << rtnl_neigh_get_state(n_old)
            << " n_new=" << rtnl_neigh_get_state(n_new);
    return 0;
  }

  n_ll_old = rtnl_neigh_get_lladdr(n_old);
  n_ll_new = rtnl_neigh_get_lladdr(n_new);

  if (n_ll_old == nullptr && n_ll_new) {
    // XXX neighbour reachable
    VLOG(2) << __FUNCTION__ << ": neighbour ll reachable";

  } else if (n_ll_old && n_ll_new == nullptr) {
    // neighbour unreachable
    VLOG(2) << __FUNCTION__ << ": neighbour ll unreachable";

    int ifindex = rtnl_neigh_get_ifindex(n_old);
    struct rtnl_link *link = nl->get_link_by_ifindex(ifindex);
    struct nl_addr *addr;

    if (link == nullptr)
      return -EINVAL;

    addr = rtnl_neigh_get_dst(n_old);
    rofl::caddress_in4 ipv4_dst = libnl_in4addr_2_rofl(addr);

    // delete next hop
    rv = sw->l3_unicast_host_remove(ipv4_dst);
    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__
                 << ": failed to remove l3 unicast host ipv4_dst" << ipv4_dst
                 << "; rv=" << rv;
      return rv;
    }

    // delete from mapping
    rv = del_l3_egress(ifindex, rtnl_link_get_addr(link),
                       rtnl_neigh_get_lladdr(n_old));

    rtnl_link_put(link);

  } else if (nl_addr_cmp(n_ll_old, n_ll_new)) {
    // XXX TODO ll addr changed
    LOG(WARNING) << __FUNCTION__ << ": neighbour ll changed (not implemented)";
  } else {
    // nothing changed besides the nud
    VLOG(2) << __FUNCTION__ << ": nud changed from "
            << rtnl_neigh_get_state(n_old) << " to "
            << rtnl_neigh_get_state(n_new); // TODO print names
  }

  return rv;
}

int nl_l3::del_l3_neigh(struct rtnl_neigh *n) {
  int rv = 0;
  struct nl_addr *addr;

  assert(n);
  if (n == nullptr)
    return -EINVAL;

  LOG(INFO) << __FUNCTION__ << ": n=" << n;

  addr = rtnl_neigh_get_dst(n);
  rofl::caddress_in4 ipv4_dst = libnl_in4addr_2_rofl(addr);

  // delete next hop
  rv = sw->l3_unicast_host_remove(ipv4_dst);
  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": failed to remove l3 unicast host ipv4_dst"
               << ipv4_dst << "; rv=" << rv;
    return rv;
  }

  // delete l3 unicast group (mac rewrite)
  int ifindex = rtnl_neigh_get_ifindex(n);
  struct rtnl_link *link = nl->get_link_by_ifindex(ifindex);

  if (link == nullptr)
    return -EINVAL;

  struct nl_addr *s_mac = rtnl_link_get_addr(link);
  struct nl_addr *d_mac = rtnl_neigh_get_lladdr(n);

  if (s_mac && d_mac)
    rv = del_l3_egress(ifindex, s_mac, d_mac);

  rtnl_link_put(link);
  return rv;
}

void nl_l3::register_switch_interface(switch_interface *sw) { this->sw = sw; }

int nl_l3::del_l3_egress(int ifindex, const struct nl_addr *s_mac,
                         const struct nl_addr *d_mac) {
  assert(s_mac);
  assert(d_mac);

  uint32_t port_id = tap_man->get_port_id(ifindex);

  if (port_id == 0) {
    VLOG(1) << __FUNCTION__ << ": invalid port_id=0 of ifindex" << ifindex;
    return -EINVAL;
  }

  int rv = 0;
  int vid = 1; // XXX TODO currently only on vid 1
  rofl::caddress_ll src_mac = libnl_lladdr_2_rofl(s_mac);
  rofl::caddress_ll dst_mac = libnl_lladdr_2_rofl(d_mac);
  auto it = l3_interface_mapping.find(
      std::make_tuple(port_id, vid, src_mac, dst_mac));

  if (it == l3_interface_mapping.end()) {
    LOG(ERROR) << __FUNCTION__
               << ": l3 interface mapping not found port_id=" << port_id
               << ", vid=" << vid << ", src_mac=" << src_mac
               << ", dst_mac=" << dst_mac;
    return -EINVAL;
  }

  it->second.refcnt--;

  if (it->second.refcnt == 0) {
    // remove egress L3 Unicast group
    rv = sw->l3_egress_remove(it->second.l3_interface_id);

    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__
                 << ": failed to setup l3 egress port_id=" << port_id
                 << ", vid=" << vid << ", src_mac=" << src_mac
                 << ", dst_mac=" << dst_mac << "; rv=" << rv;
      return rv;
    }

    l3_interface_mapping.erase(it);
  }

  return rv;
}

struct rtnl_neigh *nl_l3::nexthop_resolution(struct rtnl_nexthop *nh,
                                             void *arg) {
  struct nl_addr *nh_addr;
  int ifindex;
  struct rtnl_neigh *neigh = nullptr;

  assert(nl);
  LOG(INFO) << __FUNCTION__ << ": nh: " << nh;

  ifindex = rtnl_route_nh_get_ifindex(nh);

#if 0
  nh_addr = rtnl_route_nh_get_via(nh);
  LOG(INFO) << __FUNCTION__ << ": ifindex=" << ifindex;

  if (nh_addr) {
    switch (nl_addr_get_family(nh_addr)) {
      case AF_INET:
      case AF_INET6:
        LOG(INFO) << "via " << nh_addr;
        break;
      default:
        LOG(INFO) << "via " << nh_addr
          << " unsupported family=" << nl_addr_get_family(nh_addr);
        break;
    }
    struct rtnl_neigh *n = nl->get_neighbour(ifindex, nh_addr);
    LOG(INFO) << __FUNCTION__ << "; found neighbour: " << n;
    rtnl_neigh_put(n);
  } else {
    LOG(INFO) << "no via";
  }
#endif // 0

  nh_addr = rtnl_route_nh_get_gateway(nh);

  if (nh_addr) {
    switch (nl_addr_get_family(nh_addr)) {
    case AF_INET:
    case AF_INET6:
      LOG(INFO) << "gw " << nh_addr;
      break;
    default:
      LOG(INFO) << "gw " << nh_addr
                << " unsupported family=" << nl_addr_get_family(nh_addr);
      break;
    }

    neigh = nl->get_neighbour(ifindex, nh_addr);
  } else {
    LOG(INFO) << __FUNCTION__ << ": no gw";
    // lookup neigh in neigh cache, direct?
  }

  return neigh;
}

} // namespace basebox
