/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cassert>
#include <chrono>
#include <deque>
#include <string>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <utility>

#include <gflags/gflags.h>

#include <netlink/cache.h>
#include <netlink/route/link.h>
#include <netlink/route/neighbour.h>
#include <netlink/route/link/bridge.h>
#include <netlink/route/link/vxlan.h>

#include "cnetlink.h"
#include "netlink-utils.h"
#include "nl_bridge.h"
#include "nl_hashing.h"
#include "nl_l3.h"
#include "nl_output.h"
#include "nl_route_query.h"
#include "nl_vxlan.h"

DECLARE_int32(port_untagged_vid);

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

struct access_tunnel_port {
  uint32_t lport_id;
  int tunnel_id;

  access_tunnel_port(int lport_id, int tunnel_id)
      : lport_id(lport_id), tunnel_id(tunnel_id) {}
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

struct tunnel_nh_port {
  int refcnt;
  uint32_t nh_id;
  tunnel_nh_port(uint32_t nh_id) : refcnt(1), nh_id(nh_id) {}
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

struct endpoint_tunnel_port {
  int refcnt; // counts direct usage of l2 addresses and all zero address
  uint32_t lport_id;
  uint32_t nh_id;
  std::map<uint32_t, unsigned> refcnt_vni; // ref count per vni
  endpoint_tunnel_port(uint32_t lport_id, uint32_t nh_id, uint32_t vni)
      : refcnt(1), lport_id(lport_id), nh_id(nh_id) {
    refcnt_vni[vni] = 1;
  }
};

} // namespace basebox

namespace std {

template <> struct hash<basebox::pport_vlan> {
  using argument_type = basebox::pport_vlan;
  using result_type = std::size_t;
  result_type operator()(argument_type const &v) const noexcept {
    size_t seed = 0;
    hash_combine(seed, v.pport);
    hash_combine(seed, v.vid);
    return seed;
  }
};

template <> struct hash<basebox::tunnel_nh> {
  using argument_type = basebox::tunnel_nh;
  using result_type = std::size_t;
  result_type operator()(argument_type const &v) const noexcept {
    size_t seed = 0;
    hash_combine(seed, v.smac);
    hash_combine(seed, v.dmac);
    hash_combine(seed, v.pv);
    return seed;
  }
};

template <> struct hash<basebox::endpoint_port> {
  using argument_type = basebox::endpoint_port;
  using result_type = std::size_t;
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

static std::unordered_multimap<pport_vlan, access_tunnel_port> access_port_ids;

static std::unordered_multimap<tunnel_nh, tunnel_nh_port> tunnel_next_hop_id;

static std::unordered_multimap<endpoint_port, endpoint_tunnel_port> endpoint_id;

static std::map<uint32_t, tunnel_nh> tunnel_next_hop2tnh;

static struct access_tunnel_port get_access_tunnel_port(uint32_t pport,
                                                        uint16_t vlan) {
  assert(pport);
  assert(vlan);

  VLOG(2) << __FUNCTION__
          << ": trying to find access tunnel_port for pport=" << pport
          << ", vlan=" << vlan;

  pport_vlan search(pport, vlan);
  auto port_range = access_port_ids.equal_range(search);
  access_tunnel_port invalid(0, 0);

  for (auto it = port_range.first; it != port_range.second; ++it) {
    if (it->first == search) {
      VLOG(2) << __FUNCTION__
              << ": found tunnel_port port_id=" << it->second.lport_id
              << ", tunnel_id=" << it->second.tunnel_id;
      return it->second;
    }
  }

  VLOG(1) << __FUNCTION__ << ": no pport_vlan matched on pport=" << pport
          << ", vlan=" << vlan;
  return invalid;
}

nl_vxlan::nl_vxlan(std::shared_ptr<nl_l3> l3, cnetlink *nl)
    : sw(nullptr), bridge(nullptr), l3(std::move(l3)), nl(nl) {}

void nl_vxlan::net_reachable_notification(struct net_params params) noexcept {

  // lookup vxlan interface
  auto vxlan_link = nl->get_link(params.ifindex, AF_UNSPEC);

  if (!vxlan_link) {
    VLOG(1) << __FUNCTION__
            << ": interface not found ifindex=" << params.ifindex;
    return;
  }

  if (!rtnl_link_is_vxlan(vxlan_link.get())) {
    VLOG(1) << __FUNCTION__ << ": not a vxlan interface " << vxlan_link.get();
    return;
  }

  auto br_link = nl->get_link(params.ifindex, AF_BRIDGE);

  create_endpoint(vxlan_link.get(), br_link.get(), params.addr);
}

void nl_vxlan::nh_reachable_notification(struct rtnl_neigh *n,
                                         struct nh_params p) noexcept {
  // call create endpoint?
  net_reachable_notification(p.np);
}

void nl_vxlan::register_switch_interface(switch_interface *sw) {
  this->sw = sw;
}

void nl_vxlan::register_bridge(nl_bridge *bridge) {
  this->bridge = bridge;

  nl_cache *c = nl->get_cache(cnetlink::NL_LINK_CACHE);
  std::unique_ptr<rtnl_link, decltype(&rtnl_link_put)> link_filter(
      rtnl_link_alloc(), &rtnl_link_put);
  int rv = rtnl_link_set_type(link_filter.get(), "vxlan");

  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": failed to set link type";
    return;
  }

  std::deque<rtnl_link *> vxlan_links;
  nl_cache_foreach_filter(
      c, OBJ_CAST(link_filter.get()),
      [](struct nl_object *obj, void *arg) {
        LOG(INFO) << "found link " << obj;
        auto vxlan_links = static_cast<std::deque<rtnl_link *> *>(arg);
        nl_object_get(obj);
        vxlan_links->push_back(LINK_CAST(obj));
      },
      &vxlan_links);

  for (auto link : vxlan_links) {
    rv = create_vni(link);

    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__ << ": failed (rv=" << rv
                 << ") to create vni for link " << link;
      continue;
    }

    // currently we process only enslaved interfaces
    if (rtnl_link_get_master(link) == 0)
      continue;

    // XXX TODO maybe before cont.
    rv = create_endpoint(link);
    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__ << ": failed (rv=" << rv
                 << ") to create create endpoint " << link;
    }

    // get neighs on vxlan link
    std::deque<rtnl_neigh *> neighs;
    std::unique_ptr<rtnl_neigh, decltype(&rtnl_neigh_put)> neigh_filter(
        rtnl_neigh_alloc(), &rtnl_neigh_put);

    rtnl_neigh_set_ifindex(neigh_filter.get(), rtnl_link_get_ifindex(link));
    rtnl_neigh_set_family(neigh_filter.get(), AF_BRIDGE);

    c = nl->get_cache(cnetlink::NL_NEIGH_CACHE);
    nl_cache_foreach_filter(
        c, OBJ_CAST(neigh_filter.get()),
        [](struct nl_object *obj, void *arg) {
          LOG(INFO) << "found neigh " << obj;
          auto neighs = static_cast<std::deque<rtnl_neigh *> *>(arg);
          nl_object_get(obj);
          neighs->push_back(NEIGH_CAST(obj));
        },
        &neighs);

    for (auto neigh : neighs) {
      auto br_link = nl->get_link(rtnl_link_get_ifindex(link), AF_BRIDGE);
      rv = add_l2_neigh(neigh, link, br_link.get());
      if (rv < 0) {
        LOG(ERROR) << __FUNCTION__ << ": failed (rv=" << rv
                   << ") to add l2 neigh " << link;
      }
      rtnl_neigh_put(neigh);
    }
    rtnl_link_put(link);
  }
}

// XXX TODO alter this function to pass the vni instead of tunnel_id
int nl_vxlan::create_access_port(rtnl_link *br_link, uint32_t tunnel_id,
                                 const std::string &access_port_name,
                                 uint32_t pport_no, uint16_t vid, bool untagged,
                                 uint32_t *lport) {
  using namespace std::chrono_literals;
  assert(sw);

  // lookup access port if it is already configured
  auto tp = get_access_tunnel_port(pport_no, vid);
  if (tp.lport_id || tp.tunnel_id) {
    VLOG(1) << __FUNCTION__
            << ": tunnel port already exists (lport_id=" << tp.lport_id
            << ", tunnel_id=" << tp.tunnel_id << "), tunnel_id=" << tunnel_id
            << ", access_port_name=" << access_port_name
            << ", pport_no=" << pport_no << ", vid=" << vid
            << ", untagged=" << untagged;
    return -EINVAL;
  }

  std::string port_name = access_port_name;
  port_name += "." + std::to_string(vid);

  // drop all vlans on port
  sw->egress_bridge_port_vlan_remove(pport_no, vid);
  sw->ingress_port_vlan_remove(pport_no, vid, untagged);

  int rv = 0;
  int cnt = 0;
  do {
    VLOG(3) << __FUNCTION__ << ": rv=" << rv << ", cnt=" << cnt << std::showbase
            << std::hex << ", port_id=" << port_id_cnt
            << ", port_name=" << port_name << std::dec
            << ", pport_no=" << pport_no << ", vid=" << vid
            << ", untagged=" << untagged;
    // XXX TODO this is totally crap even if it works for now
    rv = sw->tunnel_access_port_create(port_id_cnt, port_name, pport_no, vid,
                                       untagged);

    cnt++;
    std::this_thread::sleep_for(10ms);
  } while (rv < 0 && cnt < 50);

  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__
               << ": failed to create access port tunnel_id=" << tunnel_id
               << ", vid=" << vid << ", port:" << access_port_name;
    return rv;
  }

  VLOG(3) << __FUNCTION__
          << ": calling tunnel_port_tenant_add port_id=" << port_id_cnt
          << ", tunnel_id=" << tunnel_id;
  rv = sw->tunnel_port_tenant_add(port_id_cnt, tunnel_id);

  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": failed to add tunnel port " << port_id_cnt
               << " to tenant " << tunnel_id;
    delete_access_port(br_link, pport_no, vid, false);
    return rv;
  }

  if (bridge->is_port_flooding(br_link)) {
    rv = enable_flooding(tunnel_id, port_id_cnt);
    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__
                 << ": failed to add flooding for lport=" << port_id_cnt
                 << " in tenant=" << tunnel_id;
      disable_flooding(tunnel_id, port_id_cnt);
      sw->tunnel_port_tenant_remove(port_id_cnt, tunnel_id);
      delete_access_port(br_link, pport_no, vid, false);
      return rv;
    }
  }

  // XXX TODO check if access port is already existing?
  access_port_ids.emplace(std::make_pair(
      pport_vlan(pport_no, vid), access_tunnel_port(port_id_cnt, tunnel_id)));

  // optionally return lport
  if (lport)
    *lport = port_id_cnt;

  port_id_cnt++;

  return 0;
}

int nl_vxlan::delete_access_port(rtnl_link *br_link, uint32_t pport_no,
                                 uint16_t vid, bool wipe_l2_addresses) {
  assert(br_link);
  pport_vlan search(pport_no, vid);
  auto port_range = access_port_ids.equal_range(search);
  auto it = port_range.first;

  for (; it != port_range.second; ++it) {
    if (it->first == search) {
      VLOG(2) << __FUNCTION__
              << ": found tunnel_port lport_id=" << it->second.lport_id
              << ", tunnel_id=" << it->second.tunnel_id;
      break;
    }
  }

  if (it == access_port_ids.end()) {
    VLOG(1) << __FUNCTION__ << ": no port found for pport_no=" << pport_no
            << ", vid=" << vid;
    return 0;
  }

  if (wipe_l2_addresses) {
    sw->l2_overlay_addr_remove(it->second.tunnel_id, it->second.lport_id,
                               rofl::caddress_ll("ff:ff:ff:ff:ff:ff"));
  }

  if (bridge->is_port_flooding(br_link)) {
    disable_flooding(it->second.tunnel_id, it->second.lport_id);
  }
  sw->tunnel_port_tenant_remove(it->second.lport_id, it->second.tunnel_id);
  sw->tunnel_port_delete(it->second.lport_id);

  access_port_ids.erase(it);

  return 0;
}

int nl_vxlan::get_tunnel_id(rtnl_link *vxlan_link, uint32_t *vni,
                            uint32_t *_tunnel_id) noexcept {
  assert(vxlan_link);

  uint32_t _vni = 0;

  if (rtnl_link_vxlan_get_id(vxlan_link, &_vni) != 0) {
    LOG(ERROR) << __FUNCTION__ << ": no valid vxlan interface " << vxlan_link;
    return -EINVAL;
  }

  if (vni) {
    *vni = _vni;
  }

  return get_tunnel_id(_vni, _tunnel_id);
}

int nl_vxlan::get_tunnel_id(uint32_t vni, uint32_t *_tunnel_id) noexcept {
  auto tunnel_id_it = vni2tunnel.find(vni);
  if (tunnel_id_it == vni2tunnel.end()) {
    VLOG(1) << __FUNCTION__ << ": got no tunnel_id for vni=" << vni;
    return -EINVAL;
  }

  *_tunnel_id = tunnel_id_it->second;

  return 0;
}

int nl_vxlan::create_vni(rtnl_link *link) {
  uint32_t vni = 0, tunnel_id = 0;

  if (!rtnl_link_is_vxlan(link)) {
    return -EINVAL;
  }

  int rv = get_tunnel_id(link, &vni, &tunnel_id);
  if (rv == 0) { // id exists
    // XXX TODO currently we just check the local cache, but
    // we have to verify the switch state as well
    return 0;
  }

  // create tenant on switch
  rv = sw->tunnel_tenant_create(this->tunnel_id_cnt, vni);

  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": failed to create tunnel tenant tunnel_id="
               << this->tunnel_id_cnt << ", vni=" << vni << ", rv=" << rv;
    return -EINVAL;
  }

  // enable tunnel_id
  rv = sw->overlay_tunnel_add(this->tunnel_id_cnt);

  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__
               << ": failed to add overlay tunnel tunnel_id=" << tunnel_id
               << ", rv=" << rv;
    sw->tunnel_tenant_delete(this->tunnel_id_cnt);
    return -EINVAL;
  }

  vni2tunnel.emplace(vni, this->tunnel_id_cnt);
  this->tunnel_id_cnt++;

  return rv;
}

int nl_vxlan::remove_vni(rtnl_link *link) {
  uint32_t vni = 0;
  int rv = rtnl_link_vxlan_get_id(link, &vni);

  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": no vni for vxlan interface set";
    return -EINVAL;
  }

  auto v2t_it = vni2tunnel.find(vni);

  if (v2t_it == vni2tunnel.end()) {
    LOG(ERROR) << __FUNCTION__ << ": could not delete vni " << link;
    return -EINVAL;
  }

  rv = sw->overlay_tunnel_remove(v2t_it->second);

  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": failed with rv=" << rv
               << " to remove overlay tunnel tunnel_id=" << v2t_it->second;
    /* fall through and try to delete tenant anyway */
  }

  rv = sw->tunnel_tenant_delete(v2t_it->second);

  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": failed with rv=" << rv << " to delete vni "
               << vni << " used for tenant " << v2t_it->second;
    return rv;
  }

  vni2tunnel.erase(v2t_it);

  return 0;
}

int nl_vxlan::create_endpoint(rtnl_link *vxlan_link) {
  assert(vxlan_link);

  if (!rtnl_link_is_vxlan(vxlan_link)) {
    return -EINVAL;
  }

  // get group/remote addr
  nl_addr *addr = nullptr;
  int rv = rtnl_link_vxlan_get_group(vxlan_link, &addr);

  if (rv < 0) {
    VLOG(1) << __FUNCTION__ << ": no peer configured";
    return 0;
  }

  assert(addr);

  std::unique_ptr<nl_addr, decltype(&nl_addr_put)> group_(addr, &nl_addr_put);

  if (nl_addr_get_family(addr) != AF_INET) {
    LOG(ERROR) << __FUNCTION__
               << ": detected unsupported remote address family " << vxlan_link;
    return -ENOTSUP;
  }

  // XXX get br_link if master exists

  create_endpoint(vxlan_link, nullptr, group_.get());

  return 0;
}

int nl_vxlan::create_endpoint(rtnl_link *vxlan_link, rtnl_link *br_link,
                              nl_addr *remote_addr) {
  assert(vxlan_link);
  assert(remote_addr);

  uint32_t lport_id = 0;
  int family = nl_addr_get_family(remote_addr);

  // XXX TODO check for multicast here, not yet supported

  if (family != AF_INET) {
    LOG(ERROR) << __FUNCTION__ << ": currently only AF_INET is supported";
    return -EINVAL;
  }

  nl_addr *tmp_addr = nullptr;
  int rv = rtnl_link_vxlan_get_local(vxlan_link, &tmp_addr);

  if (rv != 0 || tmp_addr == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": no local address for vxlan interface set";
    return -EINVAL;
  }

  std::unique_ptr<nl_addr, void (*)(nl_addr *)> local_(tmp_addr, &nl_addr_put);

  family = nl_addr_get_family(local_.get());

  if (family != AF_INET) {
    LOG(ERROR) << __FUNCTION__ << ": currently only AF_INET is supported";
    return -EINVAL;
  }

  uint32_t next_hop_id = 0;
  rv = create_next_hop(vxlan_link, remote_addr, &next_hop_id);
  if (rv == -ENETUNREACH) {
    // NH network not reachable (route missing)
    l3->notify_on_net_reachable(
        this, net_params{remote_addr, rtnl_link_get_ifindex(vxlan_link)});
    return rv;
  } else if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": failed to create next hop rv=" << rv;
    return rv;
  }

  uint32_t tunnel_id, vni;
  rv = get_tunnel_id(vxlan_link, &vni, &tunnel_id);
  if (rv < 0) {
    return -EINVAL;
  }

  rv = create_endpoint(vxlan_link, local_.get(), remote_addr, next_hop_id,
                       &lport_id);

  if (rv < 0) {
    delete_next_hop(next_hop_id);
    LOG(ERROR) << __FUNCTION__ << ": failed to create endpoint";
    return -EINVAL;
  }

  rv = sw->tunnel_port_tenant_add(lport_id, tunnel_id);
  if (rv < 0) {
    delete_endpoint(vxlan_link, local_.get(), remote_addr);
    delete_next_hop(next_hop_id);
    LOG(ERROR) << __FUNCTION__ << ": tunnel_port_tenant_add returned rv=" << rv
               << " for lport_id=" << lport_id << " tunnel_id=" << tunnel_id;
    return -EINVAL;
  }

  // XXX TODO move this and call separate since attachment to bridge must be
  // handled independently
  if (br_link && bridge->is_port_flooding(br_link)) {
    rv = enable_flooding(tunnel_id, lport_id);

    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__
                 << ": failed to add flooding for lport=" << lport_id
                 << " in tenant=" << tunnel_id;
      disable_flooding(tunnel_id, lport_id);
      sw->tunnel_port_tenant_remove(lport_id, tunnel_id);
      delete_endpoint(vxlan_link, local_.get(), remote_addr);
      delete_next_hop(next_hop_id);
    }
  }

  // get neighs on vxlan link
  std::deque<rtnl_neigh *> neighs;
  std::unique_ptr<rtnl_neigh, decltype(&rtnl_neigh_put)> neigh_filter(
      rtnl_neigh_alloc(), &rtnl_neigh_put);

  rtnl_neigh_set_ifindex(neigh_filter.get(), rtnl_link_get_ifindex(vxlan_link));
  // dst must be before setting family
  rtnl_neigh_set_dst(neigh_filter.get(), remote_addr);
  rtnl_neigh_set_family(neigh_filter.get(), AF_BRIDGE);

  auto c = nl->get_cache(cnetlink::NL_NEIGH_CACHE);
  nl_cache_foreach_filter(
      c, OBJ_CAST(neigh_filter.get()),
      [](struct nl_object *obj, void *arg) {
        auto n = NEIGH_CAST(obj);

        // ignore remotes
        if (nl_addr_iszero(rtnl_neigh_get_lladdr(n)))
          return;

        VLOG(3) << "found neigh for endpoint " << obj;
        auto neighs = static_cast<std::deque<rtnl_neigh *> *>(arg);
        neighs->push_back(n);
      },
      &neighs);

  for (auto neigh : neighs) {
    rv = add_l2_neigh(neigh, lport_id, tunnel_id);
    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__ << ": failed (rv=" << rv
                 << ") to add l2 neigh " << neigh;
    }
  }

  return rv;
}

int nl_vxlan::create_endpoint(rtnl_link *vxlan_link, nl_addr *local_,
                              nl_addr *group_, uint32_t _next_hop_id,
                              uint32_t *lport_id) {
  assert(group_);
  assert(local_);
  assert(vxlan_link);

  int ttl = rtnl_link_vxlan_get_ttl(vxlan_link);

  if (ttl == 0)
    ttl = 45; // XXX TODO is this a sane default?

  uint32_t vni;
  int rv = rtnl_link_vxlan_get_id(vxlan_link, &vni);

  if (rv < 0) {
    LOG(FATAL) << __FUNCTION__ << ": something went south";
  }

  uint32_t remote_ipv4 = 0;
  memcpy(&remote_ipv4, nl_addr_get_binary_addr(group_), sizeof(remote_ipv4));
  remote_ipv4 = ntohl(remote_ipv4);

  uint32_t local_ipv4 = 0;
  memcpy(&local_ipv4, nl_addr_get_binary_addr(local_), sizeof(local_ipv4));
  local_ipv4 = ntohl(local_ipv4);

  uint32_t initiator_udp_dst_port = 4789;
  rv = rtnl_link_vxlan_get_port(vxlan_link, &initiator_udp_dst_port);
  if (rv != 0) {
    LOG(WARNING) << __FUNCTION__
                 << ": vxlan dstport not specified. Falling back to "
                 << initiator_udp_dst_port;
  }

  uint32_t terminator_udp_dst_port = 4789;
  bool use_entropy = true;
  uint32_t udp_src_port_if_no_entropy = 0;
  auto ep = endpoint_port(local_ipv4, remote_ipv4, initiator_udp_dst_port);
  auto ep_it = endpoint_id.equal_range(ep);

  for (auto it = ep_it.first; it != ep_it.second; ++it) {
    if (it->first == ep) {
      VLOG(1) << __FUNCTION__
              << ": found an endpoint_port lport_id=" << it->second.lport_id;
      *lport_id = it->second.lport_id;
      it->second.refcnt++;
      it->second.refcnt_vni[vni]++;

      VLOG(1) << __FUNCTION__ << ": refcnt=" << it->second.refcnt
              << ", refcnt_vni[" << vni << "]=" << it->second.refcnt_vni[vni];
      return 0;
    }
  }

  // create endpoint port
  VLOG(3) << __FUNCTION__ << std::hex << std::showbase
          << ": calling tunnel_enpoint_create lport_id=" << lport_id
          << ", name=" << rtnl_link_get_name(vxlan_link)
          << ", remote=" << remote_ipv4 << ", local=" << local_ipv4
          << ", ttl=" << ttl << ", next_hop_id=" << _next_hop_id
          << ", terminator_udp_dst_port=" << terminator_udp_dst_port
          << ", initiator_udp_dst_port=" << initiator_udp_dst_port
          << ", use_entropy=" << use_entropy;
  rv = sw->tunnel_enpoint_create(
      this->port_id_cnt, std::string(rtnl_link_get_name(vxlan_link)),
      remote_ipv4, local_ipv4, ttl, _next_hop_id, terminator_udp_dst_port,
      initiator_udp_dst_port, udp_src_port_if_no_entropy, use_entropy);

  if (rv != 0) {
    LOG(ERROR) << __FUNCTION__
               << ": failed to create tunnel enpoint lport_id=" << std::hex
               << std::showbase << lport_id
               << ", name=" << rtnl_link_get_name(vxlan_link)
               << ", remote=" << remote_ipv4 << ", local=" << local_ipv4
               << ", ttl=" << ttl << ", next_hop_id=" << _next_hop_id
               << ", terminator_udp_dst_port=" << terminator_udp_dst_port
               << ", initiator_udp_dst_port=" << initiator_udp_dst_port
               << ", use_entropy=" << use_entropy << ", rv=" << rv;
    return -EINVAL;
  }

  endpoint_id.emplace(
      ep, endpoint_tunnel_port(this->port_id_cnt, _next_hop_id, vni));
  *lport_id = this->port_id_cnt++;
  return 0;
}

int nl_vxlan::enable_flooding(uint32_t tunnel_id, uint32_t lport_id) {
  int rv = 0;

  VLOG(1) << __FUNCTION__ << ": enable flooding on lport_id=" << lport_id
          << ", tunnel_id=" << tunnel_id;
  rv = sw->add_l2_overlay_flood(tunnel_id, lport_id);
  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": failed to add tunnel port " << port_id_cnt
               << " to flooding for tenant " << tunnel_id;
    return -EINVAL;
  }

  return rv;
}

int nl_vxlan::disable_flooding(uint32_t tunnel_id, uint32_t lport_id) {
  int rv = 0;

  VLOG(1) << __FUNCTION__ << ": disable flooding on lport_id=" << lport_id
          << ", tunnel_id=" << tunnel_id;
  rv = sw->del_l2_overlay_flood(tunnel_id, lport_id);
  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": failed to remove tunnel port "
               << port_id_cnt << " from flooding group in tenant " << tunnel_id;
    return -EINVAL;
  }

  return rv;
}

int nl_vxlan::delete_endpoint(rtnl_link *vxlan_link) {
  nl_addr *tmp_addr = nullptr;
  int rv = rtnl_link_vxlan_get_local(vxlan_link, &tmp_addr);

  if (rv != 0 || tmp_addr == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": no local address for vxlan interface set";
    return -EINVAL;
  }

  std::unique_ptr<nl_addr, void (*)(nl_addr *)> local_(tmp_addr, &nl_addr_put);

  // get group/remote addr
  nl_addr *remote = nullptr;
  rv = rtnl_link_vxlan_get_group(vxlan_link, &remote);

  if (rv < 0) {
    VLOG(1) << __FUNCTION__ << ": no peer configured";
    return 0;
  }

  std::unique_ptr<nl_addr, decltype(&nl_addr_put)> remote_(remote,
                                                           &nl_addr_put);

  delete_endpoint(vxlan_link, local_.get(), remote_.get());
  return 0;
}

static auto lookup_endpoint(rtnl_link *vxlan_link, nl_addr *local_,
                            nl_addr *remote_) {
  uint32_t remote_ipv4 = 0;
  memcpy(&remote_ipv4, nl_addr_get_binary_addr(remote_), sizeof(remote_ipv4));
  remote_ipv4 = ntohl(remote_ipv4);

  uint32_t local_ipv4 = 0;
  memcpy(&local_ipv4, nl_addr_get_binary_addr(local_), sizeof(local_ipv4));
  local_ipv4 = ntohl(local_ipv4);

  uint32_t vni;
  int rv = rtnl_link_vxlan_get_id(vxlan_link, &vni);

  if (rv < 0) {
    LOG(FATAL) << __FUNCTION__ << ": not a vxlan link " << vxlan_link;
  }

  uint32_t initiator_udp_dst_port = 4789;
  rv = rtnl_link_vxlan_get_port(vxlan_link, &initiator_udp_dst_port);
  if (rv != 0) {
    LOG(WARNING) << __FUNCTION__
                 << ": vxlan dstport not specified. Falling back to "
                 << initiator_udp_dst_port;
  }

  auto ep = endpoint_port(local_ipv4, remote_ipv4, initiator_udp_dst_port);
  auto ep_it = endpoint_id.equal_range(ep);

  if (ep_it.first == endpoint_id.end()) {
    VLOG(1) << __FUNCTION__
            << ": endpoint not found having the following parameter local addr "
            << local_ << ", remote addr " << remote_ << ", on link "
            << vxlan_link;
    return endpoint_id.end();
  }

  for (; ep_it.first != ep_it.second; ++ep_it.first) {
    if (ep_it.first->first == ep) {
      VLOG(1) << __FUNCTION__ << ": found an endpoint_port lport_id="
              << ep_it.first->second.lport_id;

      return ep_it.first;
    }
  }

  /* not found */
  return endpoint_id.end();
}

int nl_vxlan::delete_endpoint(rtnl_link *vxlan_link, nl_addr *local_,
                              nl_addr *remote_) {
  assert(remote_);
  assert(local_);
  assert(vxlan_link);

  uint32_t tunnel_id = 0;
  uint32_t vni;
  int rv = get_tunnel_id(vxlan_link, &vni, &tunnel_id);
  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": cannot retrieve tunnel ids of "
               << vxlan_link;
    return -EINVAL;
  }

  auto ep_it = lookup_endpoint(vxlan_link, local_, remote_);
  if (ep_it == endpoint_id.end()) {
    LOG(WARNING) << __FUNCTION__ << ": endpoint already deleted for "
                 << vxlan_link;
    return 0;
  }

  uint32_t lport_id = ep_it->second.lport_id;

  ep_it->second.refcnt--;
  unsigned refcnt_vni = --ep_it->second.refcnt_vni[vni];

  VLOG(1) << __FUNCTION__ << ": refcnt=" << ep_it->second.refcnt
          << ", refcnt_vni[" << vni << "]=" << ep_it->second.refcnt_vni[vni];

  // XXX TODO should be checked using the bridge port
  disable_flooding(tunnel_id, lport_id); // TODO needed?

  if (refcnt_vni == 0) {
    rv = sw->tunnel_port_tenant_remove(lport_id, tunnel_id);

    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__ << ": failed to remove port=" << lport_id
                 << " from tenant=" << tunnel_id << ", rv=" << rv;
      return rv;
    }
  }

  if (ep_it->second.refcnt == 0) {
    // TODO is everything deleted that is pointing here?
    rv = sw->tunnel_port_delete(lport_id);
    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__
                 << ": failed to remove endpoint lport_id=" << lport_id
                 << " tenant_id=" << tunnel_id << ", rv=" << rv;
    }

    // delete next hop
    rv = delete_next_hop(ep_it->second.nh_id);

    endpoint_id.erase(ep_it);
  }

  return rv;
}

int nl_vxlan::create_next_hop(rtnl_link *vxlan_link, nl_addr *remote,
                              uint32_t *next_hop_id) {
  int rv;
  std::packaged_task<struct rtnl_route *(struct nl_addr *)> task(
      [](struct nl_addr *addr) {
        nl_route_query rq;
        return rq.query_route(addr);
      });
  std::future<struct rtnl_route *> result = task.get_future();
  std::thread(std::move(task), remote).detach();

  VLOG(4) << __FUNCTION__ << ": wait for task to finish";
  result.wait();
  VLOG(4) << __FUNCTION__ << ": task finished";
  std::unique_ptr<rtnl_route, void (*)(rtnl_route *)> route(result.get(),
                                                            &rtnl_route_put);

  if (route.get() == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": could not retrieve route to " << remote;
    return -EINVAL;
  }

  int nnh = rtnl_route_get_nnexthops(route.get());

  VLOG(2) << __FUNCTION__ << ": route " << route.get() << " with " << nnh
          << " next hop(s)";
  // XXX TODO implement ecmp
  if (nnh > 1) {
    // ecmp
    LOG(WARNING) << __FUNCTION__
                 << ": ecmp not supported, only first next hop will be used.";
  }

  std::deque<struct rtnl_nexthop *> nhs;
  l3->get_nexthops_of_route(route.get(), &nhs);
  int ifindex = 0;
  nl_addr *nh_addr = nullptr;

  if (nhs.size() == 0)
    return -ENETUNREACH;

  // check if we have a ip next hop
  for (auto nh : nhs) {
    ifindex = rtnl_route_nh_get_ifindex(nh);
    nh_addr = rtnl_route_nh_get_gateway(nh);

    if (!nh_addr)
      nh_addr = rtnl_route_nh_get_via(nh);

    if (nh_addr)
      break;
  }

  // if not, directly use the remote as next hop
  // the ifindex is the one of the ip less nexthop (which marks a on-link route)
  if (!nh_addr)
    nh_addr = remote;

  auto neigh = nl->get_neighbour(ifindex, nh_addr);
  if (!neigh || rtnl_neigh_get_lladdr(neigh) == nullptr) {

    VLOG(3) << __FUNCTION__ << ": got unresolved nh ifindex=" << ifindex
            << ", nh=" << nh_addr;
    l3->notify_on_nh_reachable(
        this, nh_params{net_params{remote, rtnl_link_get_ifindex(vxlan_link)},
                        nh_stub{nh_addr, ifindex}});
    return -EDESTADDRREQ;
  }

  rv = create_next_hop(neigh, next_hop_id);
  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": failed to create next hop " << neigh;
    return -EINVAL;
  }

  return rv;
}

int nl_vxlan::create_next_hop(rtnl_neigh *neigh, uint32_t *next_hop_id) {
  int rv;

  assert(neigh);
  assert(next_hop_id_cnt);

  // get outgoing interface
  uint32_t ifindex = rtnl_neigh_get_ifindex(neigh);
  auto local_link = nl->get_link_by_ifindex(ifindex);

  if (local_link.get() == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": invalid link ifindex=" << ifindex;
    return -EINVAL;
  }

  uint32_t physical_port = nl->get_port_id(local_link.get());

  if (physical_port == 0) {
    // XXX retry this neigh later?
    LOG(ERROR) << __FUNCTION__ << ": no port_id for ifindex=" << ifindex;
    return -EINVAL;
  }

  nl_addr *addr = rtnl_link_get_addr(local_link.get());
  if (addr == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": invalid link (no ll addr) "
               << local_link.get();
    return -EINVAL;
  }

  uint64_t src_mac = nlall2uint64(addr);

  // get neigh and set dst_mac
  addr = rtnl_neigh_get_lladdr(neigh);
  if (addr == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": invalid neigh (no ll addr) " << neigh;
    return -EINVAL;
  }

  uint64_t dst_mac = nlall2uint64(addr);
  uint16_t vlan_id =
      FLAGS_port_untagged_vid; // XXX TODO currently hardcoded to untagged vid
  auto tnh = tunnel_nh(src_mac, dst_mac, physical_port, vlan_id);
  auto tnh_it = tunnel_next_hop_id.equal_range(tnh);

  for (auto it = tnh_it.first; it != tnh_it.second; ++it) {
    if (it->first == tnh) {
      VLOG(1) << __FUNCTION__
              << ": found a tunnel next hop match using next_hop_id="
              << it->second.nh_id;
      *next_hop_id = it->second.nh_id;
      it->second.refcnt++;
      return 0;
    }
  }

  // create next hop
  VLOG(3) << __FUNCTION__ << std::hex << std::showbase
          << ": calling tunnel_next_hop_create next_hop_id=" << next_hop_id_cnt
          << ", src_mac=" << src_mac << ", dst_mac=" << dst_mac
          << ", physical_port=" << physical_port << ", vlan_id=" << vlan_id;
  rv = sw->tunnel_next_hop_create(next_hop_id_cnt, src_mac, dst_mac,
                                  physical_port, vlan_id);

  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": tunnel_next_hop_create returned rv=" << rv
               << " for the following parameter: next_hop_id="
               << next_hop_id_cnt << ", src_mac=" << src_mac
               << ", dst_mac=" << dst_mac << ", physical_port=" << physical_port
               << ", vlan_id=" << vlan_id;
    return rv;
  }

  tunnel_next_hop_id.emplace(tnh, next_hop_id_cnt);
  tunnel_next_hop2tnh.emplace(next_hop_id_cnt, tnh);
  *next_hop_id = next_hop_id_cnt++;

  return rv;
}

int nl_vxlan::delete_next_hop(rtnl_neigh *neigh) {
  assert(neigh);
  assert(next_hop_id_cnt); // XXX TODO implement better id counting

  // get outgoing interface
  uint32_t ifindex = rtnl_neigh_get_ifindex(neigh);
  auto local_link = nl->get_link_by_ifindex(ifindex);

  if (local_link.get() == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": invalid link ifindex=" << ifindex;
    return -EINVAL;
  }

  uint32_t physical_port = nl->get_port_id(local_link.get());

  if (physical_port == 0) {
    LOG(ERROR) << __FUNCTION__ << ": no port_id for ifindex=" << ifindex;
    return -EINVAL;
  }

  nl_addr *addr = rtnl_link_get_addr(local_link.get());
  if (addr == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": invalid link (no ll addr) "
               << local_link.get();
    return -EINVAL;
  }

  uint64_t src_mac = nlall2uint64(addr);

  // get neigh and set dst_mac
  addr = rtnl_neigh_get_lladdr(neigh);
  if (addr == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": invalid neigh (no ll addr) " << neigh;
    return -EINVAL;
  }

  uint64_t dst_mac = nlall2uint64(addr);
  uint16_t vlan_id =
      FLAGS_port_untagged_vid; // XXX TODO currently hardcoded to untagged vid
  auto tnh = tunnel_nh(src_mac, dst_mac, physical_port, vlan_id);

  return delete_next_hop(tnh);
}

int nl_vxlan::delete_next_hop(uint32_t nh_id) {
  auto nh2tnh_it = tunnel_next_hop2tnh.find(nh_id);

  if (nh2tnh_it == tunnel_next_hop2tnh.end()) {
    LOG(WARNING) << __FUNCTION__ << ": not found nh_id=" << nh_id;
    return -ENODATA;
  }

  auto tnh = tunnel_nh(nh2tnh_it->second.smac, nh2tnh_it->second.dmac,
                       nh2tnh_it->second.pv.pport, nh2tnh_it->second.pv.vid);

  return delete_next_hop(tnh);
}

int nl_vxlan::delete_next_hop(const struct tunnel_nh &tnh) {
  auto tnh_it = tunnel_next_hop_id.equal_range(tnh);
  auto it = tnh_it.first;
  bool found = false;

  for (; it != tnh_it.second; ++it) {
    if (it->first == tnh) {
      it->second.refcnt--;
      VLOG(1) << __FUNCTION__
              << ": found a tunnel next hop match using next_hop_id="
              << it->second.nh_id << ", refcnt=" << it->second.refcnt;
      found = true;
      break;
    }
  }

  if (!found) {
    LOG(WARNING) << __FUNCTION__ << ": tried to delete invalid next hop";
    return -EINVAL;
  }

  if (it->second.refcnt == 0) {
    int rv = sw->tunnel_next_hop_delete(it->second.nh_id);

    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__ << ": failed to delete next hop next_hop_id="
                 << it->second.nh_id << ", rv=" << rv;
    }

    tunnel_next_hop2tnh.erase(it->second.nh_id);
    tunnel_next_hop_id.erase(it);
  }

  return 0;
}

int nl_vxlan::add_l2_neigh(rtnl_neigh *neigh, rtnl_link *link,
                           rtnl_link *br_link) {
  assert(link);
  assert(neigh);
  assert(rtnl_link_get_family(link) == AF_UNSPEC); // the actual interface

  int rv = 0;
  uint32_t lport = 0;
  uint32_t tunnel_id = 0;
  enum link_type lt = get_link_type(link);
  auto neigh_mac = rtnl_neigh_get_lladdr(neigh);
  auto link_mac = rtnl_link_get_addr(link);

  if (nl_addr_cmp(neigh_mac, link_mac) == 0) {
    VLOG(2) << __FUNCTION__
            << ": (silently) ignoring interface address of link " << link;
    return 0;
  }

  switch (lt) {
    /* find according endpoint port */
  case LT_VXLAN: {

    LOG(INFO) << __FUNCTION__ << ": add neigh " << neigh
              << " on vxlan interface " << link;

    bool is_endpoint = nl_addr_iszero(neigh_mac);

    nl_addr *local;
    rv = rtnl_link_vxlan_get_local(link, &local);
    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__ << "local addr not set";
      return -EINVAL;
    }

    nl_addr *remote = rtnl_neigh_get_dst(neigh);
    if (remote == nullptr) {
      LOG(ERROR) << __FUNCTION__ << ": dst not set";
      return -EINVAL;
    }

    // check remote address
    if (is_endpoint) {
      nl_addr *group = nullptr;

      rv = rtnl_link_vxlan_get_group(link, &group);
      if (rv == 0) {
        std::unique_ptr<nl_addr, decltype(&nl_addr_put)> grp(group,
                                                             &nl_addr_put);
        if (nl_addr_cmp(grp.get(), remote) == 0) {
          VLOG(2) << __FUNCTION__ << ": ignoring redundant endpoint address "
                  << remote;
          return 0;
        }
      }
    }

    // XXX TODO could check on type unicast?

    uint32_t vni;
    rv = get_tunnel_id(link, &vni, &tunnel_id);
    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__ << ": could not look up vni/tunnel id of "
                 << link;
      return -EINVAL;
    }

    auto ep_it = lookup_endpoint(link, local, remote);
    if (ep_it != endpoint_id.end()) {
      lport = ep_it->second.lport_id;
    }

    if (is_endpoint) {
      // first or additional endpoint

      if (lport == 0) {
        // setup tmp remote to pass to create remote

        LOG(INFO) << __FUNCTION__
                  << ": create new enpoint with remote_ipv4=" << remote;

        rv = create_endpoint(link, br_link, remote);

      } else {
        // existing remote but new tunnel_id/vni on link
        rv = sw->tunnel_port_tenant_add(lport, tunnel_id);
        ep_it->second.refcnt++;
        ep_it->second.refcnt_vni[vni]++;

        VLOG(1) << __FUNCTION__ << ": refcnt=" << ep_it->second.refcnt
                << ", refcnt_vni[" << vni
                << "]=" << ep_it->second.refcnt_vni[vni];

        if (rv < 0) {
          LOG(ERROR) << __FUNCTION__ << ": failed to add lport=" << lport
                     << " to tenant=" << tunnel_id;
          return rv;
        }

        if (bridge->is_port_flooding(br_link)) {
          rv = enable_flooding(tunnel_id, lport);

          if (rv < 0) {
            LOG(ERROR) << __FUNCTION__
                       << ": failed to add flooding for lport=" << lport
                       << " in tenant=" << tunnel_id;
            return rv;
          }
        }
      }

      // update access ports
      bridge->update_access_ports(link, br_link, tunnel_id, true);

      return rv;
    } // endpoint

    // TODO second check on the same here
    if (ep_it == endpoint_id.end()) {
      LOG(ERROR) << __FUNCTION__ << ": found no endpoint for neighbour "
                 << neigh;
      return -EINVAL;
    }

    rv = add_l2_neigh(neigh, lport, tunnel_id);
    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__
                 << ": failed to add flooding for lport=" << lport
                 << " in tenant=" << tunnel_id;
      return rv;
    }

    ep_it->second.refcnt++;
    ep_it->second.refcnt_vni[vni]++;

    VLOG(1) << __FUNCTION__ << ": refcnt=" << ep_it->second.refcnt
            << ", refcnt_vni[" << vni << "]=" << ep_it->second.refcnt_vni[vni];
  } break;
    /* find according access port */
  default: {
    int ifindex = rtnl_link_get_ifindex(link);
    uint16_t vlan = rtnl_neigh_get_vlan(neigh);
    uint32_t pport = nl->get_port_id(ifindex);

    if (pport == 0) {
      LOG(WARNING) << __FUNCTION__ << ": ignoring unknown link " << link;
      return -EINVAL;
    }

    auto port = get_access_tunnel_port(pport, vlan);
    lport = port.lport_id;
    tunnel_id = port.tunnel_id;

    rv = add_l2_neigh(neigh, lport, tunnel_id);
  } break;
  }

  return rv;
}

int nl_vxlan::add_l2_neigh(rtnl_neigh *neigh, uint32_t lport,
                           uint32_t tunnel_id) {
  if (lport == 0 || tunnel_id == 0) {
    LOG(ERROR) << __FUNCTION__
               << ": could not find vxlan port details to add neigh " << neigh
               << ", lport=" << lport << ", tunnel_id=" << tunnel_id;
    return -EINVAL;
  }

  bool permanent =
      !!(rtnl_neigh_get_state(neigh) & (NUD_NOARP | NUD_PERMANENT));

  auto neigh_mac = rtnl_neigh_get_lladdr(neigh);
  rofl::caddress_ll mac((uint8_t *)nl_addr_get_binary_addr(neigh_mac),
                        nl_addr_get_len(neigh_mac));
  VLOG(2) << __FUNCTION__ << ": adding l2 overlay addr for lport=" << lport
          << ", tunnel_id=" << tunnel_id << ", mac=" << mac;
  sw->l2_overlay_addr_add(lport, tunnel_id, mac, permanent);

  return 0;
}

int nl_vxlan::delete_l2_neigh(rtnl_neigh *neigh, rtnl_link *link,
                              rtnl_link *br_link) {
  assert(link);
  assert(neigh);
  assert(rtnl_link_get_family(link) == AF_UNSPEC); // the vxlan interface

  enum link_type lt = get_link_type(link);
  auto neigh_mac = rtnl_neigh_get_lladdr(neigh);

  switch (lt) {
  case LT_VXLAN: {
    /* find according endpoint port */
    LOG(INFO) << __FUNCTION__ << ": neigh " << neigh << " on vxlan interface "
              << link;

    uint32_t dst_port;
    int rv = rtnl_link_vxlan_get_port(link, &dst_port);

    if (rv < 0) {
      LOG(FATAL) << __FUNCTION__ << ": something went south";
    }

    nl_addr *local;

    rv = rtnl_link_vxlan_get_local(link, &local);

    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__ << "local addr not set";
      return -EINVAL;
    }

    std::unique_ptr<nl_addr, void (*)(nl_addr *)> local_(local, &nl_addr_put);
    int family = nl_addr_get_family(local_.get());

    if (family != AF_INET) {
      LOG(ERROR) << __FUNCTION__ << ": currently only AF_INET is supported";
      return -EINVAL;
    }

    uint32_t local_ipv4 = 0;

    memcpy(&local_ipv4, nl_addr_get_binary_addr(local_.get()),
           sizeof(local_ipv4));
    local_ipv4 = ntohl(local_ipv4);

    nl_addr *dst = rtnl_neigh_get_dst(neigh);

    // TODO could check on type unicast
    if (dst == nullptr) {
      LOG(ERROR) << __FUNCTION__ << ": destination is not set for neigh "
                 << neigh;
      return -EINVAL;
    }

    if (nl_addr_iszero(neigh_mac)) {
      nl_addr *group = nullptr;

      rv = rtnl_link_vxlan_get_group(link, &group);
      if (rv == 0) {
        std::unique_ptr<nl_addr, decltype(&nl_addr_put)> grp(group,
                                                             &nl_addr_put);
        // skip this in case the dst addr is the vxlan remote
        if (nl_addr_cmp(grp.get(), dst) == 0) {
          VLOG(2) << __FUNCTION__ << ": ignoring redundant endpoint address "
                  << dst;
          return 0;
        }
      }

      delete_endpoint(link, local, dst);
    } else {
      // just mac on endpoint deleted -> drop bridging entry
      delete_l2_neigh_endpoint(link, local, dst, neigh_mac);
    }
  } break;

  default: {
    uint16_t vlan = rtnl_neigh_get_vlan(neigh);
    delete_l2_neigh_access(link, vlan, neigh_mac);
  } break;
  } // switch lt

  return 0;
}

int nl_vxlan::delete_l2_neigh_endpoint(rtnl_link *vxlan_link, nl_addr *local,
                                       nl_addr *remote, nl_addr *neigh_mac) {
  uint32_t vni;
  uint32_t tunnel_id;
  int rv = get_tunnel_id(vxlan_link, &vni, &tunnel_id);
  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": could not retrieve tunnel_id";
    return -EINVAL;
  }

  rv = delete_l2_neigh(tunnel_id, neigh_mac);
  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": could not delete link local neighbour "
               << neigh_mac << " in tunnel_id=" << tunnel_id;
    return -EINVAL;
  }

  auto ep_it = lookup_endpoint(vxlan_link, local, remote);
  if (ep_it == endpoint_id.end()) {
    LOG(ERROR)
        << __FUNCTION__
        << ": endpoint not found having the following parameter local addr "
        << local << ", remote addr " << remote << ", on link " << vxlan_link;
    return -EINVAL;
  }

  ep_it->second.refcnt--;
  auto refcnt_vni = --ep_it->second.refcnt_vni[vni];

  if (refcnt_vni == 0) {
    auto lport_id = ep_it->second.lport_id;
    rv = sw->tunnel_port_tenant_remove(lport_id, tunnel_id);

    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__ << ": failed to remove port=" << lport_id
                 << " from tenant=" << tunnel_id << ", rv=" << rv;
      return rv;
    }
  }

  if (ep_it->second.refcnt == 0) {
    // TODO is everything deleted that is pointing here?
    auto lport_id = ep_it->second.lport_id;
    rv = sw->tunnel_port_delete(lport_id);
    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__
                 << ": failed to remove endpoint lport_id=" << lport_id
                 << " tenant_id=" << tunnel_id << ", rv=" << rv;
    }

    // delete next hop
    rv = delete_next_hop(ep_it->second.nh_id);

    endpoint_id.erase(ep_it);
  }

  VLOG(2) << __FUNCTION__ << ": refcnt=" << ep_it->second.refcnt
          << ", refcnt_vni[" << vni << "]=" << ep_it->second.refcnt_vni[vni];
  return 0;
}

int nl_vxlan::delete_l2_neigh_access(rtnl_link *link, uint16_t vlan,
                                     nl_addr *neigh_mac) {
  /* find according access port */
  int ifindex = rtnl_link_get_ifindex(link);
  uint32_t pport = nl->get_port_id(ifindex);

  if (pport == 0) {
    LOG(WARNING) << __FUNCTION__ << ": ignoring unknown link " << link;
    return -EINVAL;
  }

  auto port = get_access_tunnel_port(pport, vlan);
  uint32_t tunnel_id = port.tunnel_id;

  return delete_l2_neigh(tunnel_id, neigh_mac);
}

int nl_vxlan::delete_l2_neigh(uint32_t tunnel_id, nl_addr *neigh_mac) {
  if (tunnel_id == 0) {
    LOG(ERROR) << __FUNCTION__
               << ": could not find vxlan port details to add neigh mac "
               << neigh_mac << ", tunnel_id=" << tunnel_id;
    return -EINVAL;
  }

  rofl::caddress_ll mac((uint8_t *)nl_addr_get_binary_addr(neigh_mac),
                        nl_addr_get_len(neigh_mac));

  VLOG(2) << __FUNCTION__
          << ": removing l2 overlay addr from tunnel_id=" << tunnel_id
          << ", mac=" << mac;
  sw->l2_overlay_addr_remove(tunnel_id, 0, mac);
  return 0;
}

} // namespace basebox
