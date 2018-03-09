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
#include "nl_route_query.hpp"
#include "nl_vxlan.hpp"
#include "tap_manager.hpp"

namespace basebox {

struct pport_vlan {
  uint32_t pport;
  uint16_t vid;

  pport_vlan(uint32_t pport, uint16_t vid) : pport(pport), vid(vid) {}

  bool operator<(const pport_vlan &rhs) const {
    return std::tie(pport, vid) < std::tie(rhs.pport, rhs.vid);
  }

  bool operator==(const pport_vlan &rhs) const {
    return std::tie(pport, vid) == std::tie(rhs.pport, rhs.vid);
  }
};

struct tunnel_nh {
  uint64_t smac;
  uint64_t dmac;
  pport_vlan pv;

  tunnel_nh(uint64_t smac, uint64_t dmac, uint32_t pport, uint16_t vid)
      : smac(smac), dmac(dmac), pv(pport, vid) {}

  bool operator<(const tunnel_nh &rhs) const {
    return std::tie(smac, dmac, pv) < std::tie(rhs.smac, rhs.dmac, rhs.pv);
  }

  bool operator==(const tunnel_nh &rhs) const {
    return std::tie(smac, dmac, pv) == std::tie(rhs.smac, rhs.dmac, rhs.pv);
  }
};

struct endpoint_port {
  uint32_t local_ipv4;
  uint32_t remote_ipv4;
  uint32_t initiator_udp_dst_port;

  endpoint_port(uint32_t local_ipv4, uint32_t remote_ipv4,
                uint32_t initiator_udp_dst_port)
      : local_ipv4(local_ipv4), remote_ipv4(remote_ipv4),
        initiator_udp_dst_port(initiator_udp_dst_port) {}

  bool operator<(const endpoint_port &rhs) const {
    return std::tie(local_ipv4, remote_ipv4, initiator_udp_dst_port) <
           std::tie(rhs.local_ipv4, rhs.remote_ipv4,
                    rhs.initiator_udp_dst_port);
  }

  bool operator==(const endpoint_port &rhs) const {
    return std::tie(local_ipv4, remote_ipv4, initiator_udp_dst_port) ==
           std::tie(rhs.local_ipv4, rhs.remote_ipv4,
                    rhs.initiator_udp_dst_port);
  }
};

} // namespace basebox

namespace std {

template <> struct hash<basebox::pport_vlan> {
  typedef basebox::pport_vlan argument_type;
  typedef std::size_t result_type;
  result_type operator()(argument_type const &v) const noexcept {
    size_t seed = 0;
    hash_combine(seed, v.pport);
    hash_combine(seed, v.vid);
    return seed;
  }
};

template <> struct hash<basebox::tunnel_nh> {
  typedef basebox::tunnel_nh argument_type;
  typedef std::size_t result_type;
  result_type operator()(argument_type const &v) const noexcept {
    size_t seed = 0;
    hash_combine(seed, v.smac);
    hash_combine(seed, v.dmac);
    hash_combine(seed, v.pv);
    return seed;
  }
};

template <> struct hash<basebox::endpoint_port> {
  typedef basebox::endpoint_port argument_type;
  typedef std::size_t result_type;
  result_type operator()(argument_type const &v) const noexcept {
    size_t seed = 0;
    hash_combine(seed, v.local_ipv4);
    hash_combine(seed, v.remote_ipv4);
    hash_combine(seed, v.initiator_udp_dst_port);
    return seed;
  }
};

} // namespace std

namespace basebox {

static std::unordered_multimap<pport_vlan, nl_vxlan::tunnel_port>
    access_port_ids;

static std::unordered_multimap<tunnel_nh, uint32_t> tunnel_next_hop_id;

static std::unordered_multimap<endpoint_port, uint32_t> endpoint_id;

static struct nl_vxlan::tunnel_port get_tunnel_port(uint32_t pport,
                                                    uint16_t vlan) {
  assert(pport);
  assert(vlan);

  VLOG(2) << __FUNCTION__ << ": trying to find tunnel_port for pport=" << pport
          << ", vlan=" << vlan;

  pport_vlan search(pport, vlan);
  auto port_range = access_port_ids.equal_range(search);

  nl_vxlan::tunnel_port invalid(0, 0);
  if (port_range.first == access_port_ids.end()) {
    LOG(WARNING) << __FUNCTION__ << ": no match found for pport=" << pport
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

  LOG(WARNING) << __FUNCTION__ << ": no pport_vlan matched on pport=" << pport
               << ", vlan=" << vlan;
  return invalid;
}

nl_vxlan::nl_vxlan(std::shared_ptr<tap_manager> tap_man,
                   std::shared_ptr<nl_l3> l3, cnetlink *nl)
    : sw(nullptr), tap_man(tap_man), l3(l3), nl(nl) {}

void nl_vxlan::register_switch_interface(switch_interface *sw) {
  this->sw = sw;
}

// XXX TODO alter this function to pass the vni instead of tunnel_id
void nl_vxlan::create_access_port(uint32_t tunnel_id,
                                  const std::string &access_port_name,
                                  uint32_t pport_no, uint16_t vid,
                                  bool untagged) {
  assert(sw);

  // lookup access port if it is already configured
  auto tp = get_tunnel_port(pport_no, vid);
  if (tp.port_id || tp.tunnel_id) {
    VLOG(1) << __FUNCTION__
            << ": tunnel port already exists (lport_id=" << tp.port_id
            << ", tunnel_id=" << tp.tunnel_id << "), tunnel_id=" << tunnel_id
            << ", access_port_name=" << access_port_name
            << ", pport_no=" << pport_no << ", vid=" << vid
            << ", untagged=" << untagged;
    return;
  }

  std::string port_name = access_port_name;
  port_name += "." + std::to_string(vid);

  // drop all vlans on port
  sw->egress_bridge_port_vlan_remove(pport_no, vid);
  sw->ingress_port_vlan_remove(pport_no, vid, untagged);

  int rv = 0;
  int cnt = 0;
  do {
    VLOG(2) << __FUNCTION__ << ": rv=" << rv << "<< cnt=" << cnt
            << std::showbase << std::hex << ", port_id=" << port_id
            << ", port_name=" << port_name << std::dec
            << ", pport_no=" << pport_no << ", vid=" << vid
            << ", untagged=" << untagged;
    // XXX TODO this is totally crap even if it works for now
    rv = sw->tunnel_access_port_create(port_id, port_name, pport_no, vid,
                                       untagged);

    cnt++;
  } while (rv < 0 && cnt < 100);

  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__
               << ": failed to create access port tunnel_id=" << tunnel_id
               << ", vid=" << vid << ", port:" << access_port_name;
    return;
  }

  VLOG(2) << __FUNCTION__ << ": call tunnel_port_tenant_add port_id=" << port_id
          << ", tunnel_id=" << tunnel_id;
  rv = sw->tunnel_port_tenant_add(port_id, tunnel_id);

  if (rv != 0) {
    LOG(ERROR) << __FUNCTION__ << ": failed to add tunnel port " << port_id
               << " to tenant " << tunnel_id;
    return;
  }

  // XXX TODO could be done just once per tunnel
  sw->overlay_tunnel_add(tunnel_id);

  // XXX TODO check if access port is already existing?
  access_port_ids.emplace(std::make_pair(pport_vlan(pport_no, vid),
                                         tunnel_port(port_id, tunnel_id)));

  port_id++;
}

#if 0
void nl_vxlan::update_access_ports(uint32_t tunnel_id,
    struct rtnl_link *access_port_old,
    struct rtnl_link_bridge_vlan *br_vlan) {
  void nl_bridge::update_vlans(uint32_t port, const std::string &devname,
      const rtnl_link_bridge_vlan *old_br_vlan,
      const rtnl_link_bridge_vlan *new_br_vlan) {
    assert(sw);
    for (int k = 0; k < RTNL_LINK_BRIDGE_VLAN_BITMAP_LEN; k++) {
      int base_bit;
      uint32_t a = old_br_vlan->vlan_bitmap[k];
      uint32_t b = new_br_vlan->vlan_bitmap[k];

      uint32_t c = old_br_vlan->untagged_bitmap[k];
      uint32_t d = new_br_vlan->untagged_bitmap[k];

      uint32_t vlan_diff = a ^ b;
      uint32_t untagged_diff = c ^ d;

      base_bit = k * 32;
      int i = -1;
      int done = 0;
      while (!done) {
        int j = find_next_bit(i, vlan_diff);
        if (j > 0) {
          // vlan added or removed
          int vid = j - 1 + base_bit;
          bool egress_untagged = false;

          // check if egress is untagged
          if (new_br_vlan->untagged_bitmap[k] & 1 << (j - 1)) {
            egress_untagged = true;

            // clear untagged_diff bit
            untagged_diff &= ~((uint32_t)1 << (j - 1));
          }

          if (new_br_vlan->vlan_bitmap[k] & 1 << (j - 1)) {
            // vlan added

            if (egress_vlan_filtered) {
              sw->egress_bridge_port_vlan_add(port, vid, egress_untagged);
            }

            if (ingress_vlan_filtered) {
              sw->ingress_port_vlan_add(port, vid, new_br_vlan->pvid == vid);
            }

          } else {
            // vlan removed

            if (ingress_vlan_filtered) {
              sw->ingress_port_vlan_remove(port, vid, old_br_vlan->pvid == vid);
            }

            if (egress_vlan_filtered) {
              // delete all FM pointing to this group first
              sw->l2_addr_remove_all_in_vlan(port, vid);
              sw->egress_bridge_port_vlan_remove(port, vid);
            }
          }

          i = j;
        } else {
          done = 1;
        }
      }

#if 0 // not yet implemented the update
      done = 0;
      i = -1;
      while (!done) {
        // vlan is existing, but swapping egress tagged/untagged
        int j = find_next_bit(i, untagged_diff);
        if (j > 0) {
          // egress untagged changed
          int vid = j - 1 + base_bit;
          bool egress_untagged = false;

          // check if egress is untagged
          if (new_br_vlan->untagged_bitmap[k] & 1 << (j-1)) {
            egress_untagged = true;
          }

          // XXX implement update
          fm_driver.update_port_vid_egress(devname, vid, egress_untagged);


          i = j;
        } else {
          done = 1;
        }
      }
#endif
    }
  }
}
#endif // 0

int nl_vxlan::get_tunnel_id(rtnl_link *vxlan_link,
                            uint32_t *_tunnel_id) noexcept {
  assert(vxlan_link);

  uint32_t vni = 0;

  if (rtnl_link_vxlan_get_id(vxlan_link, &vni) != 0) {
    LOG(ERROR) << __FUNCTION__ << ": no valid vxlan interface " << vxlan_link;
    return -EINVAL;
  }

  return get_tunnel_id(vni, _tunnel_id);
}

int nl_vxlan::get_tunnel_id(uint32_t vni, uint32_t *_tunnel_id) noexcept {
  auto tunnel_id_it = vni2tunnel.find(vni);
  if (tunnel_id_it == vni2tunnel.end()) {
    LOG(ERROR) << __FUNCTION__ << ": got no tunnel_id for vni=" << vni;
    return -EINVAL;
  }

  *_tunnel_id = tunnel_id_it->second.tunnel_id;

  return 0;
}

#if 0
int nl_vxlan::add_bridge_port(rtnl_link *vxlan_link, rtnl_link *br_port) {
  assert(vxlan_link);
  assert(br_port);

  /* this is currently only used for vxlan interface attachment */

  std::deque<rtnl_link *> bridge_ports;

  ::basebox::get_bridge_ports(rtnl_link_get_master(br_port),
      nl->get_cache(cnetlink::NL_LINK_CACHE),
      &bridge_ports);

  if (bridge_ports.size() == 0) {
    // XXX TODO shouldn't happen, since we always should discover this link
    // itself, so lets check this
    LOG(INFO) << __FUNCTION__ << " XXX got called";
    return 0;
  }

  // check for each bridge port the vlan overlap
  auto vxlan_vlans = rtnl_link_bridge_get_port_vlan(br_port);
  uint32_t _tunnel_id = 0;

  if (get_tunnel_id(vxlan_link, &tunnel_id) != 0)
    return -EINVAL;

  // XXX pvid is currently not taken into account
  for (auto _br_port : bridge_ports) {
    auto br_port_vlans = rtnl_link_bridge_get_port_vlan(_br_port);
    auto matching_vlans = get_matching_vlans(br_port_vlans, vxlan_vlans);

    int ifindex = rtnl_link_get_ifindex(_br_port);
    uint32_t pport_no = tap_man->get_port_id(ifindex);

    if (pport_no == 0) {
      LOG(WARNING) << __FUNCTION__ << ": ignoring unknown port "
        << OBJ_CAST(_br_port);
      continue;
    }

    // XXX TODO what about other endpoint ports?
    create_access_ports(_tunnel_id, pport_no,
        std::string(rtnl_link_get_name(_br_port)),
        &matching_vlans);
  }

  return 0;
}
#endif // 0

int nl_vxlan::create_endpoint_port(struct rtnl_link *link) {
  std::thread rqt;

  assert(link);

  // get group/remote addr
  uint32_t port_id = 0;
  nl_addr *addr = nullptr;
  int rv = rtnl_link_vxlan_get_group(link, &addr);

  if (rv != 0 || addr == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": no group/remote for vxlan interface set";
    return -EINVAL;
  }

  // store group/remote
  std::unique_ptr<nl_addr, void (*)(nl_addr *)> group_(addr, &nl_addr_put);
  int family = nl_addr_get_family(group_.get());

  // XXX TODO check for multicast here, not yet supported

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
      [](struct nl_addr *addr) {
        nl_route_query rq;
        return rq.query_route(addr);
      });
  std::future<struct rtnl_route *> result = task.get_future();
  std::thread(std::move(task), group_.get()).detach();

  uint32_t vni = 0;
  rv = rtnl_link_vxlan_get_id(link, &vni);
  if (rv != 0) {
    LOG(ERROR) << __FUNCTION__ << ": no vni for vxlan interface set";
    return -EINVAL;
  }

  // create tenant on switch
  rv = sw->tunnel_tenant_create(tunnel_id, vni);

  if (rv != 0) {
    LOG(ERROR) << __FUNCTION__
               << ": failed to create tunnel tenant tunnel_id=" << tunnel_id
               << ", vni=" << vni << ", rv=" << rv;
  }

  VLOG(4) << __FUNCTION__ << ": wait for rq_task to finish";
  result.wait();
  VLOG(4) << __FUNCTION__ << ": rq_task finished";
  std::unique_ptr<rtnl_route, void (*)(rtnl_route *)> route(result.get(),
                                                            &rtnl_route_put);

  if (route.get() == nullptr) {
    // XXX TODO delete tunnel tenanet
    LOG(ERROR) << __FUNCTION__ << ": could not retrieve route to "
               << group_.get();
    return -EINVAL;
  }

  VLOG(2) << __FUNCTION__ << ": route " << OBJ_CAST(route.get());

  int nnh = rtnl_route_get_nnexthops(route.get());

  if (nnh != 1) {
    // ecmp
    // XXX TODO delete tunnel tenanet
    LOG(ERROR) << __FUNCTION__ << ": ecmp not supported";
    return -ENOTSUP;
  }

  std::deque<struct rtnl_neigh *> neighs;
  nl_l3::nh_lookup_params p = {&neighs, route.get(), nl};

  l3->get_neighbours_of_route(route.get(), &p);

  if (neighs.size() == 0) {
    // XXX TODO remove tunnel
    LOG(ERROR) << __FUNCTION__ << ": neighs.size()=" << neighs.size();
    return -ENOTSUP;
  }

  std::unique_ptr<rtnl_neigh, void (*)(rtnl_neigh *)> neigh_(neighs.front(),
                                                             &rtnl_neigh_put);
  neighs.pop_front();

  // clean others
  for (auto n : neighs)
    rtnl_neigh_put(n);

  uint32_t _next_hop_id = 0;
  rv = create_next_hop(neigh_.get(), &_next_hop_id);
  if (rv < 0) {
    // XXX TODO remove tunnel
    LOG(ERROR) << __FUNCTION__ << ": failed to create next hop "
               << OBJ_CAST(neigh_.get());
    return -EINVAL;
  }

  rv = create_endpoint(link, std::move(local_), std::move(group_), _next_hop_id,
                       &port_id);
  if (rv < 0) {
    // XXX TODO remove next hop?
    // XXX TODO remove tunnel
    LOG(ERROR) << __FUNCTION__ << ": failed to create endpoint";
    return -EINVAL;
  }

  vni2tunnel.emplace(
      vni, tunnel_port(port_id,
                       tunnel_id)); // XXX TODO this should likely correlate
                                    // with the remote address
  // FIXME TODO remove vni2tunnel as soon as vxlan interface is gone

  rv = sw->tunnel_port_tenant_add(port_id, tunnel_id); // XXX check rv
  LOG(INFO) << __FUNCTION__
            << ": XXX tunnel_port_tenant_add returned rv=" << rv;

  tunnel_id++;

  return 0;
}

int nl_vxlan::create_endpoint(
    rtnl_link *link, std::unique_ptr<nl_addr, void (*)(nl_addr *)> local_,
    std::unique_ptr<nl_addr, void (*)(nl_addr *)> group_, uint32_t _next_hop_id,
    uint32_t *port_id) {

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
  int rv = rtnl_link_vxlan_get_port(link, &initiator_udp_dst_port);
  if (rv != 0) {
    LOG(WARNING) << __FUNCTION__
                 << ": vxlan dstport not specified. Falling back to "
                 << initiator_udp_dst_port;
  }

  uint32_t terminator_udp_dst_port = 4789;
  bool use_entropy = true;
  uint32_t udp_src_port_if_no_entropy = 0;

  auto ep = endpoint_port(remote_ipv4, local_ipv4, initiator_udp_dst_port);
  auto ep_it = endpoint_id.equal_range(ep);

  for (auto it = ep_it.first; it != ep_it.second; ++it) {
    if (it->first == ep) {
      VLOG(1) << __FUNCTION__
              << ": found an endpoint_port port_id=" << it->second;
      *port_id = it->second;
      return 0;
    }
  }

  // create endpoint port
  VLOG(2) << __FUNCTION__ << std::hex << std::showbase
          << ": calling tunnel_enpoint_create port_id=" << port_id
          << ", name=" << rtnl_link_get_name(link) << ", remote=" << remote_ipv4
          << ", local=" << local_ipv4 << ", ttl=" << ttl
          << ", next_hop_id=" << _next_hop_id
          << ", terminator_udp_dst_port=" << terminator_udp_dst_port
          << ", initiator_udp_dst_port=" << initiator_udp_dst_port
          << ", use_entropy=" << use_entropy;
  rv = sw->tunnel_enpoint_create(
      this->port_id, std::string(rtnl_link_get_name(link)), remote_ipv4,
      local_ipv4, ttl, _next_hop_id, terminator_udp_dst_port,
      initiator_udp_dst_port, udp_src_port_if_no_entropy, use_entropy);

  if (rv != 0) {
    LOG(ERROR) << __FUNCTION__
               << ": failed to create tunnel enpoint port_id=" << std::hex
               << std::showbase << port_id
               << ", name=" << rtnl_link_get_name(link)
               << ", remote=" << remote_ipv4 << ", local=" << local_ipv4
               << ", ttl=" << ttl << ", next_hop_id=" << _next_hop_id
               << ", terminator_udp_dst_port=" << terminator_udp_dst_port
               << ", initiator_udp_dst_port=" << initiator_udp_dst_port
               << ", use_entropy=" << use_entropy << ", rv=" << rv;
    return -EINVAL;
  }

  endpoint_id.emplace(ep, this->port_id);
  *port_id = this->port_id++;
  return 0;
}

int nl_vxlan::create_next_hop(rtnl_neigh *neigh, uint32_t *_next_hop_id) {
  int rv;

  assert(neigh);
  assert(next_hop_id);

  // get outgoing interface
  uint32_t ifindex = rtnl_neigh_get_ifindex(neigh);
  uint32_t physical_port = tap_man->get_port_id(ifindex);

  if (physical_port == 0) {
    LOG(ERROR) << __FUNCTION__ << ": no port_id for ifindex=" << ifindex;
    return -EINVAL;
  }

  rtnl_link *local_link = nl->get_link_by_ifindex(ifindex);
  if (local_link == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": invalid link ifindex=" << ifindex;
    return -EINVAL;
  }

  nl_addr *addr = rtnl_link_get_addr(local_link);
  if (addr == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": invalid link (no ll addr) "
               << OBJ_CAST(local_link);
    return -EINVAL;
  }

  uint64_t src_mac = nlall2uint64(addr);

  // get neigh and set dst_mac
  addr = rtnl_neigh_get_lladdr(neigh);
  if (addr == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": invalid neigh (no ll addr) "
               << OBJ_CAST(neigh);
    return -EINVAL;
  }

  uint64_t dst_mac = nlall2uint64(addr);
  uint16_t vlan_id = 1; // XXX FIXME currently hardcoded to vid 1

  auto tnh = tunnel_nh(src_mac, dst_mac, physical_port, vlan_id);
  auto tnh_it = tunnel_next_hop_id.equal_range(tnh);

  for (auto it = tnh_it.first; it != tnh_it.second; ++it) {
    if (it->first == tnh) {
      VLOG(1) << __FUNCTION__
              << ": found a tunnel next hop match using next_hop_id="
              << it->second;
      *_next_hop_id = it->second;
      return 0;
    }
  }

  // create next hop
  LOG(INFO) << __FUNCTION__ << std::hex << std::showbase
            << ": calling tunnel_next_hop_create next_hop_id=" << next_hop_id
            << ", src_mac=" << src_mac << ", dst_mac=" << dst_mac
            << ", physical_port=" << physical_port << ", vlan_id=" << vlan_id;
  rv = sw->tunnel_next_hop_create(next_hop_id, src_mac, dst_mac, physical_port,
                                  vlan_id);
  LOG(INFO) << __FUNCTION__
            << ": XXX tunnel_next_hop_create returned rv=" << rv;

  if (rv == 0) {
    tunnel_next_hop_id.emplace(tnh, next_hop_id);
    *_next_hop_id = next_hop_id++;
  }

  return rv;
}

int nl_vxlan::add_l2_neigh(rtnl_neigh *neigh, rtnl_link *l) {
  assert(l);
  assert(neigh);
  assert(rtnl_link_get_family(l) == AF_UNSPEC);

  uint32_t lport = 0;
  uint32_t tunnel_id = 0;
  enum link_type lt = kind_to_link_type(rtnl_link_get_type(l));
  auto neigh_mac = rtnl_neigh_get_lladdr(neigh);

  switch (lt) {
    /* find according endpoint port */
  case LT_VXLAN: {
    uint32_t vni;
    int rv = rtnl_link_vxlan_get_id(l, &vni);

    if (nl_addr_iszero(neigh_mac)) {
      // XXX FIXME first or additional endpoint
      // auto dst = rtnl_neigh_get_dst(neigh);
      // XXX FIXME setup BUM FRAMES here

      VLOG(1) << __FUNCTION__ << ": ignored " << OBJ_CAST(neigh) << " at "
              << OBJ_CAST(l);
      return 0;
    }

    LOG(INFO) << __FUNCTION__ << ": add neigh " << OBJ_CAST(neigh)
              << " on vxlan interface " << OBJ_CAST(l);

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
  case LT_TUN: {
    int ifindex = rtnl_link_get_ifindex(l);
    uint16_t vlan = rtnl_neigh_get_vlan(neigh);
    uint32_t pport = tap_man->get_port_id(ifindex);

    if (pport == 0) {
      LOG(WARNING) << __FUNCTION__ << ": ignoring unknown link " << OBJ_CAST(l);
      return -EINVAL;
    }

    auto port = get_tunnel_port(pport, vlan);
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

  auto link_mac = rtnl_link_get_addr(l);

  if (nl_addr_cmp(neigh_mac, link_mac) == 0) {
    VLOG(2) << __FUNCTION__ << ": ignoring interface address of link "
            << OBJ_CAST(l);
    return -ENOTSUP;
  }

  rofl::caddress_ll mac((uint8_t *)nl_addr_get_binary_addr(neigh_mac),
                        nl_addr_get_len(neigh_mac));

  VLOG(2) << __FUNCTION__ << ": adding l2 overlay addr for lport=" << lport
          << ", tunnel_id=" << tunnel_id << ", mac=" << mac;
  sw->l2_overlay_addr_add(lport, tunnel_id, mac);
  return 0;
}

} // namespace basebox
