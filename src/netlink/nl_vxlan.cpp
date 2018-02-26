#include <cassert>
#include <deque>
#include <string>
#include <tuple>
#include <unordered_map>

#include <netlink/route/link.h>
#include <netlink/route/neighbour.h>
#include <netlink/route/link/bridge.h>
#include <netlink/route/link/vxlan.h>

#include "cnetlink.hpp"
#include "netlink-utils.hpp"
#include "nl_hashing.hpp"
#include "nl_l3.hpp"
#include "nl_output.hpp"
#include "nl_vxlan.hpp"
#include "tap_manager.hpp"

namespace basebox {

struct ifi_vlan {
  int bridge_ifindex;
  uint16_t vid;

  ifi_vlan(int ifindex, uint16_t vid) : bridge_ifindex(ifindex), vid(vid) {}

  bool operator<(const ifi_vlan &rhs) const {
    return std::tie(bridge_ifindex, vid) <
           std::tie(rhs.bridge_ifindex, rhs.vid);
  }

  bool operator==(const ifi_vlan &rhs) const {
    return std::tie(bridge_ifindex, vid) ==
           std::tie(rhs.bridge_ifindex, rhs.vid);
  }
};

} // namespace basebox

namespace std {

template <> struct hash<basebox::ifi_vlan> {
  typedef basebox::ifi_vlan argument_type;
  typedef std::size_t result_type;
  result_type operator()(argument_type const &v) const noexcept {
    size_t seed = 0;
    hash_combine(seed, v.bridge_ifindex);
    hash_combine(seed, v.vid);
    return seed;
  }
};

} // namespace std

namespace basebox {

static std::unordered_multimap<ifi_vlan, nl_vxlan::tunnel_port> access_port_ids;

static struct nl_vxlan::tunnel_port get_tunnel_port(int ifindex,
                                                    uint16_t vlan) {
  assert(ifindex);
  assert(vlan);

  VLOG(2) << __FUNCTION__
          << ": trying to find tunnel_port for ifindex=" << ifindex
          << ", vlan=" << vlan;

  ifi_vlan search(ifindex, vlan);
  auto port_range = access_port_ids.equal_range(search);

  nl_vxlan::tunnel_port invalid(0, 0);
  if (port_range.first == access_port_ids.end()) {
    LOG(WARNING) << __FUNCTION__ << ": no match found for ifindex=" << ifindex
                 << ", vlan=" << vlan;

    return invalid;
  }

  for (auto it = port_range.first; it != port_range.second; ++it) {
    if (it->first == search) {
      VLOG(2) << __FUNCTION__
              << ": found tunnel_port port_id=" << it->second.port_id
              << ", tunnel_id=" << it->second.tunnel_id;
      return it->second;
    }
  }

  LOG(WARNING) << __FUNCTION__ << ": no ifi_vlan matched on ifindex=" << ifindex
               << ", vlan=" << vlan;
  return invalid;
}

nl_vxlan::nl_vxlan(std::shared_ptr<tap_manager> tap_man, nl_l3 *l3,
                   cnetlink *nl)
    : sw(nullptr), tap_man(tap_man), nl(nl), l3(l3) {}

void nl_vxlan::register_switch_interface(switch_interface *sw) {
  this->sw = sw;
}

struct rtnl_link_bridge_vlan
get_matching_vlans(const struct rtnl_link_bridge_vlan *a,
                   const struct rtnl_link_bridge_vlan *b) {
  struct rtnl_link_bridge_vlan r;

  assert(a);
  assert(b);

  for (int k = 0; k < RTNL_LINK_BRIDGE_VLAN_BITMAP_LEN; k++) {
    r.vlan_bitmap[k] = a->vlan_bitmap[k] & b->vlan_bitmap[k];
    r.untagged_bitmap[k] = a->untagged_bitmap[k] & b->untagged_bitmap[k];
  }

  return r;
}

// XXX code duplication
static int find_next_bit(int i, uint32_t x) {
  int j;

  if (i >= 32)
    return -1;

  /* find first bit */
  if (i < 0)
    return __builtin_ffs(x);

  /* mask off prior finds to get next */
  j = __builtin_ffs(x >> i);
  return j ? j + i : 0;
}

void nl_vxlan::create_access_port(uint32_t tunnel_id,
                                  struct rtnl_link *access_port_, uint16_t vid,
                                  bool untagged) {
  assert(access_port_);
  assert(sw);

  int ifindex = rtnl_link_get_ifindex(access_port_);
  const uint32_t pport_no = tap_man->get_port_id(ifindex);
  std::string port_name(rtnl_link_get_name(access_port_));
  port_name += "." + std::to_string(vid);

  sw->ingress_port_vlan_remove(pport_no, vid, untagged);
  int rv;
  int cnt = 0;
  do {
    // XXX TODO this is totally crap even if it works for now
    rv = sw->tunnel_access_port_create(port_id, port_name, pport_no,
                                       vid); // XXX FIXME check rv
    VLOG(2) << __FUNCTION__ << ": rv=" << rv << ", cnt=" << cnt;
    cnt++;
  } while (rv < 0 && cnt < 30);

  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__
               << ": failed to create access port tunnel_id=" << tunnel_id
               << ", vid=" << vid << ", port:" << OBJ_CAST(access_port_);
  }

  VLOG(2) << __FUNCTION__ << ": call tunnel_port_tenant_add port_id=" << port_id
          << ", tunnel_id=" << tunnel_id;
  sw->tunnel_port_tenant_add(port_id, tunnel_id); // XXX FIXME check rv

  // XXX TODO could be done just once per tunnel
  sw->overlay_tunnel_add(tunnel_id);

  // XXX TODO check if access port is already existing?
  access_port_ids.emplace(
      std::make_pair(ifi_vlan(ifindex, vid), tunnel_port(port_id, tunnel_id)));

  port_id++;
}

// XXX loop through vlans (again code duplication
// TODO shouldn't be void
void nl_vxlan::create_access_ports(uint32_t tunnel_id,
                                   struct rtnl_link *access_port,
                                   struct rtnl_link_bridge_vlan *br_vlan) {
  assert(access_port);
  assert(br_vlan);

  for (int k = 0; k < RTNL_LINK_BRIDGE_VLAN_BITMAP_LEN; k++) {
    int base_bit;
    uint32_t a = br_vlan->vlan_bitmap[k];

    base_bit = k * 32;
    int i = -1;
    int done = 0;
    while (!done) {
      int j = find_next_bit(i, a);
      if (j > 0) {
        int vid = j - 1 + base_bit;
        bool egress_untagged = false;

        // check if egress is untagged
        if (br_vlan->untagged_bitmap[k] & 1 << (j - 1)) {
          egress_untagged = true;
        }

        LOG(INFO) << __FUNCTION__ << ": same vlan " << vid;
#if 0
        if (egress_vlan_filtered) {
          sw->egress_bridge_port_vlan_add(port, vid, egress_untagged);
        }

        if (ingress_vlan_filtered) {
          sw->ingress_port_vlan_add(port, vid, br_vlan->pvid == vid);
        }
#endif // 0

        // create access ports for each overlapping vlan
        bool untagged = false;
        create_access_port(tunnel_id, access_port, vid, untagged);

        i = j;
      } else {
        done = 1;
      }
    }
  }
}

int nl_vxlan::add_bridge_port(rtnl_link *vxlan_link, rtnl_link *br_port) {
  assert(vxlan_link);
  assert(br_port);

  uint32_t vni = 0;

  if (rtnl_link_vxlan_get_id(vxlan_link, &vni) != 0) {
    LOG(ERROR) << __FUNCTION__ << ": no valid vxlan interface " << vxlan_link;
    return -EINVAL;
  }

  auto tunnel_id_it = vni2tunnel.find(vni);
  if (tunnel_id_it == vni2tunnel.end()) {
    LOG(ERROR) << __FUNCTION__ << ": got no tunnel_id for vni=" << vni;
    return -EINVAL;
  }

  uint32_t tunnel_id = tunnel_id_it->second.tunnel_id;
  std::unique_ptr<rtnl_link, void (*)(rtnl_link *)> filter(rtnl_link_alloc(),
                                                           &rtnl_link_put);
  assert(filter && "out of memory");
  rtnl_link_set_family(filter.get(), AF_BRIDGE);
  rtnl_link_set_master(filter.get(), rtnl_link_get_master(br_port));

  std::deque<rtnl_link *> bridge_ports;
  std::tuple<nl_vxlan *, std::deque<rtnl_link *> *> params = {this,
                                                              &bridge_ports};
  auto cache = nl->get_cache(cnetlink::NL_LINK_CACHE);
  assert(cache);

  nl_cache_foreach_filter(
      cache, OBJ_CAST(filter.get()),
      [](struct nl_object *obj, void *arg) {
        std::tuple<nl_vxlan *, std::deque<rtnl_link *> *> *params =
            static_cast<std::tuple<nl_vxlan *, std::deque<rtnl_link *> *> *>(
                arg);

        LOG(INFO) << __FUNCTION__ << ": got bridge link name="
                  << rtnl_link_get_name(LINK_CAST(obj));

        if (!rtnl_link_get_master(LINK_CAST(obj))) {
          return;
        }

        if (!std::get<0>(*params)->tap_man->get_port_id(
                rtnl_link_get_ifindex(LINK_CAST(obj))))
          return;

        LOG(INFO) << __FUNCTION__ << ": XXX found" << obj;
        // nl_object_get(obj);
        std::get<1>(*params)->push_back(LINK_CAST(obj)); // XXX unique_ptr?
      },
      &params);

  if (bridge_ports.size() == 0) {
    // l2 domain so far
    return 0;
  }

  // check for each bridge port the vlan overlap
  auto vxlan_vlans = rtnl_link_bridge_get_port_vlan(br_port);

  // XXX pvid is currently not taken into account
  for (auto _br_port : bridge_ports) {
    auto br_port_vlans = rtnl_link_bridge_get_port_vlan(_br_port);
    auto matching_vlans = get_matching_vlans(br_port_vlans, vxlan_vlans);
    // XXX TODO what about other endpoint ports?
    create_access_ports(tunnel_id, _br_port, &matching_vlans);
  }

  return 0;
}

int nl_vxlan::create_endpoint_port(struct rtnl_link *link) {
  uint16_t vlan_id = 1; // XXX FIXME currently hardcoded to vid 1
  std::thread rqt;

  assert(link);

  // get group/remote addr
  nl_addr *addr = nullptr;
  int rv = rtnl_link_vxlan_get_group(link, &addr);

  if (rv != 0 || addr == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": no group/remote for vxlan interface set";
    return -EINVAL;
  }

  // store group/remote
  std::unique_ptr<nl_addr, void (*)(nl_addr *)> group_(addr, &nl_addr_put);

  // XXX TODO check for multicast here
  // XXX check if group is a neighbour

  int family = nl_addr_get_family(group_.get());

  if (family != AF_INET) {
    LOG(ERROR) << __FUNCTION__ << ": currently only AF_INET is supported";
    return -EINVAL;
  }

  rv = rtnl_link_vxlan_get_local(link, &addr);

  if (rv != 0 || addr == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": no local address for vxlan interface set";
    return -EINVAL;
  }

  std::unique_ptr<nl_addr, void (*)(nl_addr *)> local_(addr, &nl_addr_put);

  family = nl_addr_get_family(local_.get());

  if (family != AF_INET) {
    LOG(ERROR) << __FUNCTION__ << ": currently only AF_INET is supported";
    return -EINVAL;
  }

  // spin off a thread to query the next hop
  std::packaged_task<struct rtnl_route *(struct nl_addr *)> task(
      [this](struct nl_addr *addr) { return rq.query_route(addr); });
  std::future<struct rtnl_route *> result = task.get_future();
  std::thread(std::move(task), group_.get()).detach();

  uint32_t vni = 0;
  rv = rtnl_link_vxlan_get_id(link, &vni);
  if (rv != 0) {
    LOG(ERROR) << __FUNCTION__ << ": no vni for vxlan interface set";
    return -EINVAL;
  }

  // create tenant on switch
  sw->tunnel_tenant_create(tunnel_id,
                           vni); // XXX FIXME check rv

  vni2tunnel.emplace(
      vni,
      tunnel_port(port_id, tunnel_id)); // XXX TODO this should likely correlate
                                        // with the remote address

  result.wait();
  std::unique_ptr<rtnl_route, void (*)(rtnl_route *)> route(result.get(),
                                                            &rtnl_route_put);

  if (route.get() == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": could not retrieve route to "
               << group_.get();
    return -EINVAL;
  }

  VLOG(2) << __FUNCTION__ << ": route " << OBJ_CAST(route.get());

  int nnh = rtnl_route_get_nnexthops(route.get());

  if (nnh != 1) {
    // ecmp
    LOG(ERROR) << __FUNCTION__ << ": ecmp not supported";
    // XXX TODO remove tunnel
    return -ENOTSUP;
  }

  std::deque<struct rtnl_neigh *> neighs;
  std::pair<nl_l3 *, std::deque<struct rtnl_neigh *> *> data =
      std::make_pair(l3, &neighs);

  LOG(INFO) << __FUNCTION__ << ": XXX this=" << this;
  rtnl_route_foreach_nexthop(
      route.get(),
      [](struct rtnl_nexthop *nh, void *arg) {
        std::pair<nl_l3 *, std::deque<struct rtnl_neigh *> *> *data =
            static_cast<
                std::pair<nl_l3 *, std::deque<struct rtnl_neigh *> *> *>(arg);
        std::deque<struct rtnl_neigh *> *neighs = data->second;

        // XXX annotate that next hop is likely directly reachable

        struct rtnl_neigh *neigh =
            static_cast<nl_l3 *>(data->first)->nexthop_resolution(nh, nullptr);
        if (neigh) {
          LOG(INFO) << __FUNCTION__ << "; found neighbour: "
                    << reinterpret_cast<struct nl_object *>(neigh);
          neighs->push_back(neigh);
        }
      },
      &data);

  if (neighs.size() > 1) {
    LOG(ERROR) << __FUNCTION__
               << ": currently only 1 neighbour is supported, got "
               << neighs.size();
    // XXX TODO remove tunnel
    return -ENOTSUP;
  }

  if (neighs.size() == 0) {
    // XXX check if remote is a neighbour
    // XXX TODO remove tunnel
    LOG(ERROR) << __FUNCTION__ << ": neighs.size()=" << neighs.size();
    return -ENOTSUP;
  }

  std::unique_ptr<rtnl_neigh, void (*)(rtnl_neigh *)> neigh_(neighs.front(),
                                                             &rtnl_neigh_put);
  neighs.pop_front();

  // get outgoing interface
  uint32_t ifindex = rtnl_neigh_get_ifindex(neigh_.get());
  uint32_t physical_port = tap_man->get_port_id(ifindex);

  if (physical_port == 0) {
    LOG(ERROR) << __FUNCTION__ << ": no port_id for ifindex=" << ifindex;
    // XXX TODO remove tunnel
    return -EINVAL;
  }

  rtnl_link *local_link = nl->get_link_by_ifindex(ifindex);
  if (local_link == nullptr) {
    // XXX TODO remove tunnel
    LOG(ERROR) << __FUNCTION__ << ": XXX ";
    return -EINVAL;
  }

  addr = rtnl_link_get_addr(local_link);
  if (addr == nullptr) {
    // XXX TODO remove tunnel
    LOG(ERROR) << __FUNCTION__ << ": XXX ";
    return -EINVAL;
  }

  uint64_t src_mac = nlall2uint64(addr);

  // get neigh and set dst_mac
  addr = rtnl_neigh_get_lladdr(neigh_.get());
  if (addr == nullptr) {
    // XXX TODO remove tunnel
    LOG(ERROR) << __FUNCTION__ << ": XXX ";
    return -EINVAL;
  }

  uint64_t dst_mac = nlall2uint64(addr);

  // FIXME XXX check if we can alter the untagged entry to a tagged one and
  // switch it back afterwards

  // create next hop
  LOG(INFO) << __FUNCTION__ << std::hex << std::showbase
            << ": calling tunnel_next_hop_create next_hop_id=" << next_hop_id
            << ", src_mac=" << src_mac << ", dst_mac=" << dst_mac
            << ", physical_port=" << physical_port << ", vlan_id=" << vlan_id;
  sw->tunnel_next_hop_create(next_hop_id, src_mac, dst_mac, physical_port,
                             vlan_id); // XXX FIXME check rv

  int ttl = rtnl_link_vxlan_get_ttl(link);

  if (ttl == 0)
    ttl = 45; // XXX TODO is this a sane default?

  uint32_t remote_ipv4 = 0;
  memcpy(&remote_ipv4, nl_addr_get_binary_addr(group_.get()),
         sizeof(remote_ipv4));
  remote_ipv4 = ntohl(remote_ipv4);

  uint32_t local_ipv4 = 0;
  memcpy(&local_ipv4, nl_addr_get_binary_addr(local_.get()),
         sizeof(local_ipv4));
  local_ipv4 = ntohl(local_ipv4);

  uint32_t initiator_udp_dst_port = 4789;
  rv = rtnl_link_vxlan_get_port(link, &initiator_udp_dst_port);
  if (rv != 0) {
    // XXX TODO remove tunnel
    LOG(WARNING) << __FUNCTION__
                 << ": vxlan dstport not specified. Falling back to "
                 << initiator_udp_dst_port;
  }

  uint32_t terminator_udp_dst_port = 4789;
  bool use_entropy = true;
  uint32_t udp_src_port_if_no_entropy = 0;

  // create endpoint port
  LOG(INFO) << __FUNCTION__ << std::hex << std::showbase
            << ": calling tunnel_enpoint_create port_id=" << port_id
            << ", name=" << rtnl_link_get_name(link)
            << ", remote=" << remote_ipv4 << ", local=" << local_ipv4
            << ", ttl=" << ttl << ", next_hop_id=" << next_hop_id
            << ", terminator_udp_dst_port=" << terminator_udp_dst_port
            << ", initiator_udp_dst_port=" << initiator_udp_dst_port
            << ", use_entropy=" << use_entropy;
  sw->tunnel_enpoint_create(
      port_id, std::string(rtnl_link_get_name(link)), // XXX string_view
      remote_ipv4, local_ipv4, ttl, next_hop_id, terminator_udp_dst_port,
      initiator_udp_dst_port, udp_src_port_if_no_entropy,
      use_entropy); // XXX check rv

  sw->tunnel_port_tenant_add(port_id, tunnel_id); // XXX check rv

  next_hop_id++;
  port_id++;
  tunnel_id++;

  return 0;
}

int nl_vxlan::add_l2_neigh(rtnl_neigh *neigh, rtnl_link *l) {
  // XXX check if link is a vxlan interface or a tap
  assert(rtnl_link_get_family(l) == AF_UNSPEC);

  uint32_t lport = 0;
  uint32_t tunnel_id = 0;
  enum cnetlink::link_type lt = nl->kind_to_link_type(rtnl_link_get_type(l));

  switch (lt) {
    /* find according endpoint port */
  case cnetlink::LT_VXLAN: {
    uint32_t vni;
    int rv = rtnl_link_vxlan_get_id(l, &vni);

    if (rv != 0) {
      LOG(FATAL) << __FUNCTION__ << "something went south";
      return -EINVAL;
    }

    auto v2t_it = vni2tunnel.find(vni);

    if (v2t_it == vni2tunnel.end()) {
      LOG(ERROR) << __FUNCTION__ << ": tunnel_id not found for vni=" << vni;
      return -EINVAL;
    }

    lport = v2t_it->second.port_id;
    tunnel_id = v2t_it->second.tunnel_id;
  } break;
    /* find according access port */
  case cnetlink::LT_TUN: {
    int ifindex = rtnl_link_get_ifindex(l);
    uint16_t vlan = rtnl_neigh_get_vlan(neigh);
    auto port = get_tunnel_port(ifindex, vlan);
    lport = port.port_id;
    tunnel_id = port.tunnel_id;
  } break;
  default:
    LOG(ERROR) << __FUNCTION__ << ": not supported";
    return -EINVAL;
    break;
  }

  if (lport == 0 || tunnel_id == 0) {
    LOG(ERROR) << __FUNCTION__
               << ": could not find vxlan port details to add neigh "
               << OBJ_CAST(neigh) << ", lport=" << lport
               << ", tunnel_id=" << tunnel_id;
    return -EINVAL;
  }

  nl_addr *nl_mac = rtnl_neigh_get_lladdr(neigh);
  rofl::caddress_ll mac((uint8_t *)nl_addr_get_binary_addr(nl_mac),
                        nl_addr_get_len(nl_mac));

  VLOG(2) << __FUNCTION__ << ": adding l2 overlay addr for lport=" << lport
          << ", tunnel_id=" << tunnel_id << ", mac=" << mac;
  sw->l2_overlay_addr_add(lport, tunnel_id, mac);
  return 0;
}

} // namespace basebox
