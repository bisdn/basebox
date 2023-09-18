/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cassert>
#include <cstring>
#include <map>
#include <utility>
#include <linux/if_packet.h>
#include <linux/if_bridge.h>

#include <glog/logging.h>
#include <netlink/route/link.h>
#ifdef HAVE_NETLINK_ROUTE_BRIDGE_VLAN_H
#include <netlink/route/bridge_vlan.h>
#endif
#ifdef HAVE_NETLINK_ROUTE_MDB_H
#include <netlink/route/mdb.h>
#endif
#include <netlink/route/link/vlan.h>
#include <netlink/route/link/vxlan.h>
#include <netlink/route/neighbour.h>
#include <rofl/common/openflow/cofport.h>

#include "cnetlink.h"
#include "netlink-utils.h"
#include "nl_bridge.h"
#include "nl_output.h"
#include "nl_vlan.h"
#include "nl_vxlan.h"
#include "sai.h"
#include "port_manager.h"

namespace basebox {

nl_bridge::nl_bridge(switch_interface *sw,
                     std::shared_ptr<port_manager> port_man, cnetlink *nl,
                     std::shared_ptr<nl_vlan> vlan,
                     std::shared_ptr<nl_vxlan> vxlan)
    : bridge(nullptr), sw(sw), port_man(std::move(port_man)), nl(nl),
      vlan(std::move(vlan)), vxlan(std::move(vxlan)),
      l2_cache(nl_cache_alloc(nl_cache_ops_lookup("route/neigh")),
               nl_cache_free) {
  memset(&empty_br_vlan, 0, sizeof(rtnl_link_bridge_vlan));
  memset(&vxlan_dom_bitmap, 0, sizeof(vxlan_dom_bitmap));
}

nl_bridge::~nl_bridge() { rtnl_link_put(bridge); }

static rofl::caddress_in4 libnl_in4addr_2_rofl(struct nl_addr *addr, int *rv) {
  struct sockaddr_in sin;
  socklen_t salen = sizeof(sin);

  assert(rv);
  *rv = nl_addr_fill_sockaddr(addr, (struct sockaddr *)&sin, &salen);
  return rofl::caddress_in4(&sin, salen);
}

void nl_bridge::set_bridge_interface(rtnl_link *bridge) {
  assert(bridge);
  assert((rtnl_link_get_type(bridge) != nullptr &&
          std::string("bridge").compare(rtnl_link_get_type(bridge)) == 0) ||
         (rtnl_link_get_family(bridge) == AF_BRIDGE &&
          rtnl_link_get_ifindex(bridge) == rtnl_link_get_master(bridge)));

  this->bridge = bridge;
  nl_object_get(OBJ_CAST(bridge));

  if (!get_vlan_filtering())
    LOG(FATAL) << __FUNCTION__
               << " unsupported: bridge configured with vlan_filtering 0";
}

bool nl_bridge::is_bridge_interface(int ifindex) {
  assert(bridge);

  return (ifindex == rtnl_link_get_ifindex(bridge)) ? true : false;
}

bool nl_bridge::is_bridge_interface(rtnl_link *link) {
  assert(bridge);
  assert(link);

  return is_bridge_interface(rtnl_link_get_ifindex(link));
}

// Read sysfs to obtain the value for the VLAN protocol on the switch.
// Only two values are suported: 0x8100 and 0x88a8
uint32_t nl_bridge::get_vlan_proto() {
  std::string portname(rtnl_link_get_name(bridge));
  return nl->load_from_file(
      "/sys/class/net/" + portname + "/bridge/vlan_protocol", 16);
}

int nl_bridge::set_vlan_proto(rtnl_link *link) {
  int rv = 0;
  if (get_vlan_proto() != ETH_P_8021AD)
    return rv;

  auto port_id = nl->get_port_id(link);
  if (nbi::get_port_type(port_id) == nbi::port_type_lag) {
    auto members = nl->get_bond_members_by_lag(link);
    for (auto mem : members)
      rv = sw->set_egress_tpid(mem);

  } else {
    rv = sw->set_egress_tpid(port_id);
  }

  return rv;
}

int nl_bridge::delete_vlan_proto(rtnl_link *link) {
  int rv = 0;
  if (get_vlan_proto() != ETH_P_8021AD)
    return rv;

  auto port_id = nl->get_port_id(link);
  if (nbi::get_port_type(port_id) == nbi::port_type_lag) {
    auto members = nl->get_bond_members_by_lag(link);
    for (auto mem : members)
      rv = sw->delete_egress_tpid(mem);

  } else {
    rv = sw->delete_egress_tpid(port_id);
  }

  return rv;
}

// Read sysfs to obtain the value for the VLAN filtering value on the switch.
// baseboxd currently only supports VLAN-aware bridges, set with the
// vlan_filtering flag to 1.
uint32_t nl_bridge::get_vlan_filtering() {
  std::string portname(rtnl_link_get_name(bridge));
  return nl->load_from_file("/sys/class/net/" + portname +
                            "/bridge/vlan_filtering");
}

static bool br_vlan_equal(const rtnl_link_bridge_vlan *lhs,
                          const rtnl_link_bridge_vlan *rhs) {
  assert(lhs);
  assert(rhs);
  return (memcmp(lhs, rhs, sizeof(struct rtnl_link_bridge_vlan)) == 0);
}

static bool is_vid_set(unsigned vid, uint32_t *addr) {
  if (vid >= RTNL_LINK_BRIDGE_VLAN_BITMAP_MAX) {
    LOG(FATAL) << __FUNCTION__ << ": vid " << vid << " out of range";
    return false;
  }

  return !!(addr[vid / 32] & (((uint32_t)1) << (vid % 32)));
}

static void set_vid(unsigned vid, uint32_t *addr) {
  if (vid < RTNL_LINK_BRIDGE_VLAN_BITMAP_MAX)
    addr[vid / 32] |= (((uint32_t)1) << (vid % 32));
}

static void unset_vid(unsigned vid, uint32_t *addr) {
  if (vid < RTNL_LINK_BRIDGE_VLAN_BITMAP_MAX)
    addr[vid / 32] &= ~(((uint32_t)1) << (vid % 32));
}

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

void nl_bridge::add_interface(rtnl_link *link) {

  assert(rtnl_link_get_family(link) == AF_BRIDGE);

  if (bridge == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": cannot be done without bridge";
    return;
  }

  if (rtnl_link_get_ifindex(bridge) != rtnl_link_get_master(link)) {
    LOG(INFO) << __FUNCTION__ << ": link " << rtnl_link_get_name(link)
              << " is no slave of " << rtnl_link_get_name(bridge);
    return;
  }

  auto state = rtnl_link_bridge_get_port_state(link);
  auto port_id = nl->get_port_id(link);
  set_port_stp_state(port_id, state);

  // configure bonds and physical ports (non members of bond)
  update_vlans(nullptr, link);

  set_vlan_proto(link);
}

void nl_bridge::update_interface(rtnl_link *old_link, rtnl_link *new_link) {
  assert(rtnl_link_get_family(old_link) == AF_BRIDGE);
  assert(rtnl_link_get_family(new_link) == AF_BRIDGE);

  // sanity checks
  if (bridge == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": cannot be done without bridge";
    return;
  }

  if (rtnl_link_get_ifindex(bridge) != rtnl_link_get_master(new_link)) {
    LOG(INFO) << __FUNCTION__ << ": link " << rtnl_link_get_name(new_link)
              << " is no slave of " << rtnl_link_get_name(bridge);
    return;
  }

  auto old_state = rtnl_link_bridge_get_port_state(old_link);
  auto new_state = rtnl_link_bridge_get_port_state(new_link);
  std::string state;

  if (old_state != new_state) {
    LOG(INFO) << __FUNCTION__ << " STP state changed, old=" << old_state
              << " new=" << new_state;

    auto port_id = nl->get_port_id(new_link);
    set_port_stp_state(port_id, new_state);
    return;
  }

  update_vlans(old_link, new_link);
}

void nl_bridge::delete_interface(rtnl_link *link) {
  assert(rtnl_link_get_family(link) == AF_BRIDGE);

  // sanity checks
  if (bridge == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": cannot be done without bridge";
    return;
  }

  if (rtnl_link_get_ifindex(bridge) != rtnl_link_get_master(link)) {
    LOG(INFO) << __FUNCTION__ << ": link " << rtnl_link_get_name(link)
              << " is no slave of " << rtnl_link_get_name(bridge);
    return;
  }

  update_vlans(link, nullptr);

  delete_vlan_proto(link);

  auto port_id = nl->get_port_id(link);
  // interface default is to STP state forward by default
  set_port_stp_state(port_id, BR_STATE_FORWARDING);
}

void nl_bridge::update_vlans(rtnl_link *old_link, rtnl_link *new_link) {
  assert(sw);
  assert(bridge); // already checked

  rtnl_link_bridge_vlan *old_br_vlan, *new_br_vlan;
  rtnl_link *_link;

  if (old_link == nullptr) {
    // link added
    old_br_vlan = &empty_br_vlan;
    new_br_vlan = rtnl_link_bridge_get_port_vlan(new_link);
    _link = nl->get_link(rtnl_link_get_ifindex(new_link), AF_UNSPEC);
  } else if (new_link == nullptr) {
    // link deleted
    old_br_vlan = rtnl_link_bridge_get_port_vlan(old_link);
    new_br_vlan = &empty_br_vlan;
    _link = nl->get_link(rtnl_link_get_ifindex(old_link), AF_UNSPEC);
  } else {
    // link updated
    old_br_vlan = rtnl_link_bridge_get_port_vlan(old_link);
    new_br_vlan = rtnl_link_bridge_get_port_vlan(new_link);
    _link = nl->get_link(rtnl_link_get_ifindex(new_link), AF_UNSPEC);
  }

  if (old_br_vlan == nullptr) {
    old_br_vlan = &empty_br_vlan;
  }

  if (new_br_vlan == nullptr) {
    new_br_vlan = &empty_br_vlan;
  }

  if (_link == nullptr) {
    // XXX FIXME in case a vxlan has been deleted the vxlan_domain and
    // vxlan_dom_bitmap need an update, maybe this can be handled already from
    // the link_deleted of the vxlan itself?
    LOG(WARNING) << __FUNCTION__
                 << ": could not get parent link of bridge interface. This "
                    "case needs further checks if everything got already "
                    "deleted.";
    return;
  }

  // bond slave added
  if (/* old_link == nullptr && */ get_link_type(_link) == LT_BOND_SLAVE) {
    int master = rtnl_link_get_master(_link);
    auto _l = nl->get_link(master, AF_BRIDGE);
    old_br_vlan = &empty_br_vlan;
    new_br_vlan = rtnl_link_bridge_get_port_vlan(_l);
  }

  // check for vid changes
  if (br_vlan_equal(old_br_vlan, new_br_vlan)) {
    VLOG(2) << __FUNCTION__ << ": vlans did not change";
    return;
  }

  link_type lt = get_link_type(_link);
  uint32_t pport_no = 0;
  uint32_t tunnel_id = -1;
  std::deque<rtnl_link *> bridge_ports;

  if (lt == LT_VXLAN) {
    assert(nl);
    nl->get_bridge_ports(rtnl_link_get_master(_link), &bridge_ports);

    if (vxlan->get_tunnel_id(_link, nullptr, &tunnel_id) != 0) {
      LOG(ERROR) << __FUNCTION__ << ": failed to get vni of link "
                 << OBJ_CAST(_link);
    }
  } else {
    pport_no = nl->get_port_id(_link);
    if (pport_no == 0) {
      LOG(ERROR) << __FUNCTION__
                 << ": invalid pport_no=0 of link: " << OBJ_CAST(_link);
      return;
    }
  }
  auto members = nl->get_bond_members_by_lag(_link);

  // check for pvid changes
  if (new_br_vlan->pvid != old_br_vlan->pvid &&
      vlan->is_vid_valid(old_br_vlan->pvid)) {
    int rv = vlan->remove_ingress_pvid(_link, old_br_vlan->pvid);

    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__
                 << ": failed to remove ingress vlan=" << old_br_vlan->pvid
                 << " link=" << OBJ_CAST(_link);
      return;
    }
  }

  for (int i = 0; i < RTNL_LINK_BRIDGE_VLAN_BITMAP_LEN; i++) {
    int base_bit;
    uint32_t vlan_old = old_br_vlan->vlan_bitmap[i];
    uint32_t vlan_new = new_br_vlan->vlan_bitmap[i];
    uint32_t vlan_diff = vlan_old ^ vlan_new;

    uint32_t untagged_old = old_br_vlan->untagged_bitmap[i];
    uint32_t untagged_new = new_br_vlan->untagged_bitmap[i];
    uint32_t untagged_diff = untagged_old ^ untagged_new;

    // vids handled by vlan_diff will
    // already have the proper egress_untagged state
    untagged_diff &= ~vlan_diff;

    base_bit = i * 32;
    for (int j = find_next_bit(-1, vlan_diff); j > 0;
         j = find_next_bit(j, vlan_diff)) {
      // vlan added or removed
      int vid = j - 1 + base_bit;
      uint32_t vid_mask = 1U << (j - 1);
      bool egress_untagged = false;

      // check if egress is untagged
      if (untagged_new & vid_mask) {
        egress_untagged = true;
      }

      if (vlan_new & vid_mask) {
        // vlan added
        if (lt == LT_VXLAN) {
          // update vxlan domain
          if (!is_vid_set(vid, vxlan_dom_bitmap)) {
            VLOG(1) << __FUNCTION__ << ": new vxlan domain vid=" << vid
                    << ", tunnel_id=" << tunnel_id;

            vxlan_domain.emplace(vid, tunnel_id);
            set_vid(vid, vxlan_dom_bitmap);
          } else {
            // XXX TODO check the map
          }

          // update all bridge ports to be access ports
          update_access_ports(_link, new_link ? new_link : old_link, vid,
                              tunnel_id, bridge_ports, true);
        } else {
          assert(pport_no);
          if (is_vid_set(vid, vxlan_dom_bitmap)) {
            // configure as access port
            std::string port_name = std::string(rtnl_link_get_name(_link));
            auto vxd_it = vxlan_domain.find(vid);

            if (vxd_it != vxlan_domain.end()) {
              vxlan->create_access_port((new_link) ? new_link : old_link,
                                        vxd_it->second, port_name, pport_no,
                                        vid, egress_untagged, nullptr);
            } else {
              LOG(FATAL) << __FUNCTION__
                         << ": should not happen, something is broken";
            }
          } else {
            // normal vlan port
            VLOG(3) << __FUNCTION__ << ": add vid=" << vid
                    << " on pport_no=" << pport_no
                    << " link: " << OBJ_CAST(_link);

            // the PVID is already being handled outside of the loop
            vlan->add_bridge_vlan(_link, vid, false, !egress_untagged);
            // restore multicast state in case there are existing mdb entries
            sw->l2_multicast_group_rejoin_all_in_vlan(pport_no, vid);

#ifdef HAVE_NETLINK_ROUTE_BRIDGE_VLAN_H
            // we might have missed the initial port vlan message since it
            // may come before the attachment message, make sure that any
            // existing vlans have their expected stp states.
            if (old_link == nullptr &&
                (bridge_stp_states.get_pvlan_state(pport_no, vid) == -EINVAL)) {
              // default state is FORWARDING in the kernel
              int state = BR_STATE_FORWARDING;

              // ask the bridge vlan cache for the current state
              struct rtnl_bridge_vlan *bvlan =
                  rtnl_bridge_vlan_get(nl->get_cache(cnetlink::NL_BVLAN_CACHE),
                                       rtnl_link_get_ifindex(_link), vid);
              if (bvlan) {
                state = rtnl_bridge_vlan_get_state(bvlan);
                rtnl_bridge_vlan_put(bvlan);
              }
              VLOG(3) << __FUNCTION__ << ": initialize vid=" << vid
                      << " on pport_no=" << pport_no
                      << " to stp state=" << state
                      << " link: " << OBJ_CAST(_link);
              add_port_vlan_stp_state(pport_no, vid, state);
            }
#endif
          }
        }
      } else {
        // vlan removed
        if (lt == LT_VXLAN) {
          unset_vid(vid, vxlan_dom_bitmap);
          vxlan_domain.erase(vid);

          // update all bridge ports to be normal bridge ports
          update_access_ports(_link, new_link ? new_link : old_link, vid,
                              tunnel_id, bridge_ports, false);
        } else {
          VLOG(3) << __FUNCTION__ << ": remove vid=" << vid
                  << " on pport_no=" << pport_no
                  << " link: " << OBJ_CAST(_link);

          // delete all cache entries first
          std::unique_ptr<rtnl_neigh, decltype(&rtnl_neigh_put)> filter(
              rtnl_neigh_alloc(), rtnl_neigh_put);

          rtnl_neigh_set_ifindex(filter.get(), rtnl_link_get_ifindex(_link));
          rtnl_neigh_set_master(filter.get(), rtnl_link_get_master(_link));
          rtnl_neigh_set_family(filter.get(), AF_BRIDGE);
          rtnl_neigh_set_vlan(filter.get(), vid);
          rtnl_neigh_set_flags(filter.get(), NTF_MASTER | NTF_EXT_LEARNED);
          rtnl_neigh_set_state(filter.get(), NUD_REACHABLE);

          nl_cache_foreach_filter(
              l2_cache.get(), OBJ_CAST(filter.get()),
              [](struct nl_object *o, void *arg) {
                VLOG(3) << "l2_cache remove object " << o;
                nl_cache_remove(o);
              },
              nullptr);

          // then delete all FM pointing to this group
          sw->l2_addr_remove_all_in_vlan(pport_no, vid);
          sw->l2_multicast_group_leave_all_in_vlan(pport_no, vid);

          // the PVID is already being handled outside of the loop
          vlan->remove_bridge_vlan(_link, vid, false, !egress_untagged);
        }
      }
    }

    // Process untagged changes
    for (int j = find_next_bit(-1, untagged_diff); j > 0;
         j = find_next_bit(j, untagged_diff)) {
      // egress untagged changed
      int vid = j - 1 + base_bit;
      uint32_t vid_mask = 1U << (j - 1);
      bool egress_untagged = false;

      // check if egress is untagged
      if (untagged_new & vid_mask) {
        egress_untagged = true;
      }

      VLOG(3) << __FUNCTION__ << ": change vid=" << vid
              << " on pport_no=" << pport_no
              << " egress untagged=" << egress_untagged
              << " link: " << OBJ_CAST(_link);

      vlan->update_egress_bridge_vlan(_link, vid, egress_untagged);
    }
  }

  // check for pvid changes
  if (new_br_vlan->pvid != old_br_vlan->pvid &&
      vlan->is_vid_valid(new_br_vlan->pvid)) {
    int rv = vlan->add_ingress_pvid(_link, new_br_vlan->pvid);

    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__
                 << ": failed to update pvid=" << new_br_vlan->pvid
                 << " link=" << OBJ_CAST(_link);
      return;
    }
  }
}

std::deque<rtnl_neigh *> nl_bridge::get_fdb_entries_of_port(rtnl_link *br_port,
                                                            uint16_t vid,
                                                            nl_addr *lladdr) {

  std::unique_ptr<rtnl_neigh, decltype(&rtnl_neigh_put)> filter(
      rtnl_neigh_alloc(), rtnl_neigh_put);

  rtnl_neigh_set_ifindex(filter.get(), rtnl_link_get_ifindex(br_port));
  rtnl_neigh_set_master(filter.get(), rtnl_link_get_ifindex(bridge));
  rtnl_neigh_set_family(filter.get(), AF_BRIDGE);

  if (vid)
    rtnl_neigh_set_vlan(filter.get(), vid);

  if (lladdr)
    rtnl_neigh_set_lladdr(filter.get(), lladdr);

  VLOG(3) << __FUNCTION__ << ": searching for " << OBJ_CAST(filter.get());
  std::deque<rtnl_neigh *> neighs;
  nl_cache_foreach_filter(
      nl->get_cache(cnetlink::NL_NEIGH_CACHE), OBJ_CAST(filter.get()),
      [](struct nl_object *o, void *arg) {
        auto *neighs = (std::deque<rtnl_neigh *> *)arg;
        neighs->push_back(NEIGH_CAST(o));
        VLOG(3) << "needs to be updated " << o;
      },
      &neighs);

  return neighs;
}

int nl_bridge::update_access_ports(rtnl_link *vxlan_link, rtnl_link *br_link,
                                   const uint32_t tunnel_id, bool add) {
  if (vxlan_link == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": vxlan_link cannot be nullptr";
    return -EINVAL;
  }

  if (br_link == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": br_link cannot be nullptr";
    return -EINVAL;
  }

  // XXX get pvid from br_link
  auto br_port_vlans = rtnl_link_bridge_get_port_vlan(br_link);
  auto vid = br_port_vlans->pvid;

  auto vxlan_domain_it = vxlan_domain.find(vid);
  if (vxlan_domain_it != vxlan_domain.end()) {
    if (vxlan_domain_it->second != tunnel_id) {
      LOG(ERROR) << __FUNCTION__ << ": different tunnel_id ("
                 << vxlan_domain_it->second
                 << ") in vxlan_domain expected id=" << tunnel_id;
    }
    VLOG(2) << __FUNCTION__
            << ": access ports already configured for vid=" << vid
            << " and tunnel_id=" << tunnel_id;
    // already vxlan domain
    return 0;
  }

  // XXX get all bridge_ports
  std::deque<rtnl_link *> bridge_ports;
  nl->get_bridge_ports(rtnl_link_get_master(br_link), &bridge_ports);

  update_access_ports(vxlan_link, br_link, vid, tunnel_id, bridge_ports, add);

  // update vxlan_domain
  vxlan_domain.emplace(vid, tunnel_id);

  return 0;
}

void nl_bridge::update_access_ports(rtnl_link *vxlan_link, rtnl_link *br_link,
                                    const uint16_t vid,
                                    const uint32_t tunnel_id,
                                    const std::deque<rtnl_link *> &bridge_ports,
                                    bool add) {
  // XXX pvid is currently not taken into account
  for (auto _br_port : bridge_ports) {
    auto br_port_vlans = rtnl_link_bridge_get_port_vlan(_br_port);

    if (rtnl_link_get_ifindex(vxlan_link) == rtnl_link_get_ifindex(_br_port))
      continue;

    if (br_port_vlans == nullptr)
      continue;

    if (!is_vid_set(vid, br_port_vlans->vlan_bitmap))
      continue;

    bool untagged = is_vid_set(vid, br_port_vlans->untagged_bitmap);
    int ifindex = rtnl_link_get_ifindex(_br_port);
    uint32_t pport_no = nl->get_port_id(ifindex);

    if (pport_no == 0) {
      LOG(WARNING) << __FUNCTION__ << ": ignoring unknown port "
                   << OBJ_CAST(_br_port);
      continue;
    }

    rtnl_link *link = nl->get_link(rtnl_link_get_ifindex(_br_port), AF_UNSPEC);

    VLOG(2) << __FUNCTION__ << ": vid=" << vid << ", untagged=" << untagged
            << ", tunnel_id=" << tunnel_id << ", add=" << add
            << ", ifindex=" << ifindex << ", pport_no=" << pport_no
            << ", port: " << OBJ_CAST(_br_port);

    if (add) {
      std::string port_name = std::string(rtnl_link_get_name(_br_port));

      // this is no longer a normal bridge interface thus we delete all the
      // bridging entries
      sw->l2_addr_remove_all_in_vlan(pport_no, vid);
      sw->l2_multicast_group_leave_all_in_vlan(pport_no, vid);
      vxlan->create_access_port(_br_port, tunnel_id, port_name, pport_no, vid,
                                untagged, nullptr);

      auto neighs = get_fdb_entries_of_port(_br_port, vid);
      for (auto n : neighs) {
        // ignore ll addr of bridge on slave
        if (nl_addr_cmp(rtnl_link_get_addr(bridge), rtnl_neigh_get_lladdr(n)) ==
            0) {
          continue;
        }
        VLOG(3) << ": needs to be updated " << OBJ_CAST(n);
        vxlan->add_l2_neigh(n, link, br_link);
      }

    } else {
      // delete access port and all bridging entries
      vxlan->delete_access_port(_br_port, pport_no, vid, true);

      vlan->add_bridge_vlan(_br_port, vid, br_port_vlans->pvid == vid,
                            !untagged);

      auto neighs = get_fdb_entries_of_port(_br_port, vid);
      for (auto n : neighs) {
        // ignore ll addr of bridge on slave
        if (nl_addr_cmp(rtnl_link_get_addr(bridge), rtnl_neigh_get_lladdr(n)) ==
            0) {
          continue;
        }
        VLOG(3) << ": needs to be updated " << OBJ_CAST(n);
        add_neigh_to_fdb(n);
      }
      sw->l2_multicast_group_rejoin_all_in_vlan(pport_no, vid);
    }
  }
}

void nl_bridge::add_neigh_to_fdb(rtnl_neigh *neigh, bool update) {
  assert(sw);
  assert(neigh);

  int ifindex = rtnl_neigh_get_ifindex(neigh);
  uint32_t port = nl->get_port_id(ifindex);
  if (port == 0) {
    VLOG(1) << __FUNCTION__ << ": unknown port for neigh " << OBJ_CAST(neigh);
    return;
  }

  nl_addr *mac = rtnl_neigh_get_lladdr(neigh);
  int vid = rtnl_neigh_get_vlan(neigh);

  // Dont add the fdb entry does not have any vlan assigned,
  // rtnl_neigh_get_vlan() returns -1 when vid is unset,
  // which is not a valid VID.
  if (vid < 0) {
    VLOG(2) << __FUNCTION__
            << ": vlan not set, skipping add neigh=" << OBJ_CAST(neigh);
    return;
  }

  // for sure this is master (sw bridged)
  if (rtnl_neigh_get_master(neigh) &&
      !(rtnl_neigh_get_flags(neigh) & NTF_MASTER)) {
    rtnl_neigh_set_flags(neigh, NTF_MASTER);
  }

  // check if entry already exists in cache
  std::unique_ptr<rtnl_neigh, decltype(&rtnl_neigh_put)> n(rtnl_neigh_alloc(),
                                                           rtnl_neigh_put);

  rtnl_neigh_set_master(n.get(), rtnl_neigh_get_master(neigh));
  rtnl_neigh_set_family(n.get(), AF_BRIDGE);
  rtnl_neigh_set_vlan(n.get(), vid);
  rtnl_neigh_set_lladdr(n.get(), mac);
  rtnl_neigh_set_flags(n.get(), NTF_MASTER | NTF_EXT_LEARNED);
  rtnl_neigh_set_state(n.get(), NUD_REACHABLE);

  std::unique_ptr<rtnl_neigh, decltype(&rtnl_neigh_put)> n_lookup(
      NEIGH_CAST(nl_cache_search(l2_cache.get(), OBJ_CAST(n.get()))),
      rtnl_neigh_put);

  if (n_lookup && rtnl_neigh_get_ifindex(n_lookup.get()) == ifindex) {
    return;
  }
  rtnl_neigh_set_ifindex(n.get(), ifindex);

  bool permanent =
      !!(rtnl_neigh_get_state(neigh) & (NUD_NOARP | NUD_PERMANENT));

  rofl::caddress_ll _mac((uint8_t *)nl_addr_get_binary_addr(mac),
                         nl_addr_get_len(mac));

  LOG(INFO) << __FUNCTION__ << ": add mac=" << _mac << " to bridge "
            << rtnl_link_get_name(bridge) << " on port=" << port
            << " vlan=" << (unsigned)vid << ", permanent=" << permanent;
  LOG(INFO) << __FUNCTION__ << ": object: " << OBJ_CAST(neigh);
  sw->l2_addr_add(port, vid, _mac, true, permanent, update);

  if (!permanent) {
    // take over dynamic entries from kernel
    rtnl_neigh_set_flags(n.get(), NTF_EXT_LEARNED);

    nl_msg *msg = nullptr;
    rtnl_neigh_build_add_request(n.get(), NLM_F_REPLACE, &msg);
    assert(msg);

    // send the message and create new fdb entry
    if (nl->send_nl_msg(msg) < 0) {
      LOG(ERROR) << __FUNCTION__ << ": failed to send netlink message";
      return;
    }

    // cache the entry
    if (nl_cache_add(l2_cache.get(), OBJ_CAST(n.get())) < 0) {
      LOG(ERROR) << __FUNCTION__ << ": failed to add entry to l2_cache "
                 << OBJ_CAST(n.get());
    }
  }
}

void nl_bridge::remove_neigh_from_fdb(rtnl_neigh *neigh) {
  assert(sw);

  int vid = rtnl_neigh_get_vlan(neigh);

  // Dont delete the fdb entry does not have any vlan assigned,
  // rtnl_neigh_get_vlan() returns -1 when vid is unset,
  // which is not a valid VID. We dont add, so we shouldnt remove
  // as well
  if (vid < 0) {
    VLOG(2) << __FUNCTION__
            << ": vlan not set, skipping remove neigh=" << OBJ_CAST(neigh);
    return;
  }

  nl_addr *addr = rtnl_neigh_get_lladdr(neigh);

  if (nl_addr_cmp(rtnl_link_get_addr(bridge), addr) == 0) {
    // ignore ll addr of bridge on slave
    return;
  }

  // lookup l2_cache as well
  std::unique_ptr<rtnl_neigh, decltype(&rtnl_neigh_put)> n_lookup(
      NEIGH_CAST(nl_cache_search(l2_cache.get(), OBJ_CAST(neigh))),
      rtnl_neigh_put);

  if (n_lookup) {
    nl_cache_remove(OBJ_CAST(n_lookup.get()));
  }

  const uint32_t port = nl->get_port_id(rtnl_neigh_get_ifindex(neigh));
  rofl::caddress_ll mac((uint8_t *)nl_addr_get_binary_addr(addr),
                        nl_addr_get_len(addr));

  sw->l2_addr_remove(port, vid, mac);
}

bool nl_bridge::is_mac_in_l2_cache(rtnl_neigh *n) {
  assert(n);

  std::unique_ptr<rtnl_neigh, decltype(&rtnl_neigh_put)> n_lookup(
      NEIGH_CAST(nl_cache_search(l2_cache.get(), OBJ_CAST(n))), rtnl_neigh_put);

  if (n_lookup) {
    VLOG(2) << __FUNCTION__ << ": found existing l2_cache entry "
            << OBJ_CAST(n_lookup.get());
    return true;
  }

  return false;
}

int nl_bridge::fdb_timeout(rtnl_link *br_link, uint16_t vid,
                           const rofl::caddress_ll &mac) {
  int rv = 0;

  std::unique_ptr<rtnl_neigh, decltype(&rtnl_neigh_put)> n(rtnl_neigh_alloc(),
                                                           rtnl_neigh_put);

  std::unique_ptr<nl_addr, decltype(&nl_addr_put)> h_src(
      nl_addr_build(AF_LLC, mac.somem(), mac.memlen()), nl_addr_put);

  rtnl_neigh_set_master(n.get(), rtnl_link_get_master(br_link));
  rtnl_neigh_set_family(n.get(), AF_BRIDGE);
  rtnl_neigh_set_vlan(n.get(), vid);
  rtnl_neigh_set_lladdr(n.get(), h_src.get());
  rtnl_neigh_set_flags(n.get(), NTF_MASTER | NTF_EXT_LEARNED);

  // find entry in local l2_cache
  std::unique_ptr<rtnl_neigh, decltype(&rtnl_neigh_put)> n_lookup(
      NEIGH_CAST(nl_cache_search(l2_cache.get(), OBJ_CAST(n.get()))),
      rtnl_neigh_put);

  if (n_lookup && rtnl_neigh_get_ifindex(n_lookup.get()) ==
                      rtnl_link_get_ifindex(br_link)) {
    // * remove l2 entry from kernel
    nl_msg *msg = nullptr;
    rtnl_neigh_build_delete_request(n.get(), NLM_F_REQUEST, &msg);
    assert(msg);

    // send the message and create new fdb entry
    if (nl->send_nl_msg(msg) < 0) {
      LOG(ERROR) << __FUNCTION__ << ": failed to send netlink message";
      return -EINVAL;
    }

    // XXX TODO maybe delete after NL event and not yet here
    nl_cache_remove(OBJ_CAST(n_lookup.get()));
  }

  return rv;
}

bool nl_bridge::is_port_flooding(rtnl_link *br_link) const {
  assert(br_link);
  assert(rtnl_link_is_bridge(br_link));

  int flags = rtnl_link_bridge_get_flags(br_link);
  return !!(flags & RTNL_BRIDGE_UNICAST_FLOOD);
}

void nl_bridge::clear_tpid_entries() {
  std::deque<rtnl_link *> bridge_ports;
  nl->get_bridge_ports(rtnl_link_get_ifindex(bridge), &bridge_ports);

  for (auto iface : bridge_ports) {
    delete_vlan_proto(iface);
  }
}

int nl_bridge::mdb_entry_add(rtnl_mdb *mdb_entry) {
  int rv = 0;
#ifdef HAVE_NETLINK_ROUTE_MDB_H
  std::deque<struct rtnl_mdb_entry *> mdb;
  uint32_t bridge = rtnl_mdb_get_ifindex(mdb_entry);

  if (!is_bridge_interface(nl->get_link_by_ifindex(bridge).get())) {
    LOG(ERROR) << ": bridge not set correctly";
    return -EINVAL;
  }

  // parse MDB object to get all nested information
  rtnl_mdb_foreach_entry(
      mdb_entry,
      [](struct rtnl_mdb_entry *it, void *arg) {
        auto m_database =
            static_cast<std::deque<struct rtnl_mdb_entry *> *>(arg);
        m_database->push_back(it);
      },
      &mdb);

  for (auto i : mdb) {
    uint32_t port_ifindex = rtnl_mdb_entry_get_ifindex(i);
    uint32_t port_id = nl->get_port_id(port_ifindex);
    uint16_t vid = rtnl_mdb_entry_get_vid(i);
    unsigned char buf[ETH_ALEN];

    if (port_ifindex == get_ifindex()) {
      continue;
    }

    auto addr = rtnl_mdb_entry_get_addr(i);
    if (rtnl_mdb_entry_get_proto(i) == ETH_P_IP) {
      // RFC 4541, section 2.1.2 says:
      // 2) Packets with a destination IP (DIP) address in the 224.0.0.X range
      //    which are not IGMP must be forwarded on all ports.
      //
      // Therefore we must not create groups for them to ensure that.

      auto p = nl_addr_alloc(4);
      nl_addr_parse("224.0.0.0/24", AF_INET, &p);
      std::unique_ptr<nl_addr, decltype(&nl_addr_put)> tm_addr(p, nl_addr_put);
      if (!nl_addr_cmp_prefix(addr, tm_addr.get()))
        continue;

      rofl::caddress_in4 ipv4_dst = libnl_in4addr_2_rofl(addr, &rv);
      if (rv < 0) {
        LOG(ERROR) << __FUNCTION__ << ": could not parse addr " << addr;
        return rv;
      }

      unsigned char buf[ETH_ALEN];
      multicast_ipv4_to_ll(ipv4_dst.get_addr_hbo(), buf);
    } else if (rtnl_mdb_entry_get_proto(i) == ETH_P_IPV6) {

      // RFC 4541, section 3 says:
      // In IPv6, the data forwarding rules are more straight forward because
      // MLD is mandated for addresses with scope 2 (link-scope) or greater.
      // The only exception is the address FF02::1 which is the all hosts
      // link-scope address for which MLD messages are never sent.  Packets
      // with the all hosts link-scope address should be forwarded on all
      // ports.
      auto p = nl_addr_alloc(16);
      nl_addr_parse("ff02::1/128", AF_INET6, &p);
      std::unique_ptr<nl_addr, decltype(&nl_addr_put)> tm_addr(p, nl_addr_put);
      if (!nl_addr_cmp(addr, tm_addr.get()))
        continue;

      // MLD messages are also not sent regarding groups with addresses in the
      // range FF00::/15 (which encompasses both the reserved FF00::/16 and
      // node-local FF01::/16 IPv6 address spaces).  These addresses should
      // never appear in packets on the link.
      nl_addr_parse("ff00::/15", AF_INET6, &p);
      if (!nl_addr_cmp_prefix(addr, tm_addr.get()))
        continue;

      struct in6_addr *v6_addr =
          (struct in6_addr *)nl_addr_get_binary_addr(addr);

      multicast_ipv6_to_ll(v6_addr, buf);
    }

    rofl::caddress_ll mc_ll = rofl::caddress_ll(buf, ETH_ALEN);

    rv = sw->l2_multicast_group_join(port_id, vid, mc_ll);
    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__ << ": failed to join Layer 2 Multicast Group";
      return rv;
    }

    VLOG(2) << __FUNCTION__ << ": mdb entry added, port" << port_id
            << " grp= " << addr;
  }
#endif

  return rv;
}

int nl_bridge::mdb_entry_remove(rtnl_mdb *mdb_entry) {
  int rv = 0;
#ifdef HAVE_NETLINK_ROUTE_MDB_H
  uint32_t bridge = rtnl_mdb_get_ifindex(mdb_entry);

  if (!is_bridge_interface(nl->get_link_by_ifindex(bridge).get())) {
    LOG(ERROR) << ": bridge not set correctly";
    return -EINVAL;
  }

  std::deque<struct rtnl_mdb_entry *> mdb;

  // parse MDB object to get all nested information
  rtnl_mdb_foreach_entry(
      mdb_entry,
      [](struct rtnl_mdb_entry *it, void *arg) {
        auto m_database =
            static_cast<std::deque<struct rtnl_mdb_entry *> *>(arg);
        m_database->push_back(it);
      },
      &mdb);

  for (auto i : mdb) {
    uint32_t port_ifindex = rtnl_mdb_entry_get_ifindex(i);
    uint32_t port_id = nl->get_port_id(port_ifindex);
    uint16_t vid = rtnl_mdb_entry_get_vid(i);
    unsigned char buf[ETH_ALEN];

    if (port_ifindex == get_ifindex()) {
      continue;
    }

    auto addr = rtnl_mdb_entry_get_addr(i);
    if (rtnl_mdb_entry_get_proto(i) == ETH_P_IP) {
      auto p = nl_addr_alloc(4);
      nl_addr_parse("224.0.0.0/24", AF_INET, &p);
      std::unique_ptr<nl_addr, decltype(&nl_addr_put)> tm_addr(p, nl_addr_put);
      if (!nl_addr_cmp_prefix(addr, tm_addr.get()))
        continue;

      rofl::caddress_in4 ipv4_dst = libnl_in4addr_2_rofl(addr, &rv);
      if (rv < 0) {
        LOG(ERROR) << __FUNCTION__ << ": could not parse addr " << addr;
        return rv;
      }

      multicast_ipv4_to_ll(ipv4_dst.get_addr_hbo(), buf);
    } else if (rtnl_mdb_entry_get_proto(i) == ETH_P_IPV6) {

      auto p = nl_addr_alloc(16);
      nl_addr_parse("ff02::1/128", AF_INET6, &p);
      std::unique_ptr<nl_addr, decltype(&nl_addr_put)> tm_addr(p, nl_addr_put);
      if (!nl_addr_cmp(addr, tm_addr.get()))
        continue;
      nl_addr_parse("ff00::/15", AF_INET6, &p);
      if (!nl_addr_cmp_prefix(addr, tm_addr.get()))
        continue;

      struct in6_addr *v6_addr =
          (struct in6_addr *)nl_addr_get_binary_addr(addr);

      multicast_ipv6_to_ll(v6_addr, buf);
    }

    rofl::caddress_ll mc_ll = rofl::caddress_ll(buf, ETH_ALEN);

    rv = sw->l2_multicast_group_leave(port_id, vid, mc_ll);
    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__ << ": failed to leave Layer 2 Multicast Group";
      return rv;
    }

    VLOG(2) << __FUNCTION__ << ": mdb entry removed, port" << port_id
            << " grp= " << addr;
  }
#endif

  return rv;
}

int nl_bridge::set_port_vlan_stp_state(uint32_t port_id, uint16_t vid,
                                       uint8_t state) {
  auto err = 0;

  if (nbi::get_port_type(port_id) == nbi::port_type_lag) {
    auto members = nl->get_bond_members_by_port_id(port_id);
    auto err2 = 0;
    for (auto mem : members) {
      err2 = sw->ofdpa_stg_state_port_set(mem, vid, state);
      if (err2 < 0)
        err = err2;
    }
  } else {
    err = sw->ofdpa_stg_state_port_set(port_id, vid, state);
  }

  return err;
}

void nl_bridge::set_port_stp_state(uint32_t port_id, uint8_t stp_state) {
  bridge_stp_states.add_global_state(port_id, stp_state);

  auto pv_states = bridge_stp_states.get_min_states(port_id);
  for (auto it : pv_states)
    set_port_vlan_stp_state(port_id, it.first, it.second);

  set_port_vlan_stp_state(port_id, 0, stp_state);
}

int nl_bridge::add_port_vlan_stp_state(uint32_t port_id, uint16_t vid,
                                       uint8_t stp_state) {
  bool new_group = bridge_stp_states.add_pvlan_state(port_id, vid, stp_state);
  if (new_group) {
    auto err = sw->ofdpa_stg_create(vid);
    if (err < 0) {
      bridge_stp_states.del_pvlan_state(port_id, vid);
      return err;
    }
  }

  auto g_stp_state = bridge_stp_states.get_min_state(port_id, vid);

  LOG(INFO) << __FUNCTION__ << ": set state=" << g_stp_state
            << " port_id= " << port_id << " VLAN =" << vid;

  return set_port_vlan_stp_state(port_id, vid, g_stp_state);
}

int nl_bridge::del_port_vlan_stp_state(uint32_t port_id, uint16_t vid) {
  // default state when not part of the vlan
  auto g_stp_state = BR_STATE_FORWARDING;

  LOG(INFO) << __FUNCTION__ << ": set state=" << g_stp_state
            << " ifindex= " << port_id << " VLAN =" << vid;

  auto err = set_port_vlan_stp_state(port_id, vid, g_stp_state);

  bool last_member = bridge_stp_states.del_pvlan_state(port_id, vid);
  if (last_member) {
    auto err2 = sw->ofdpa_stg_destroy(vid);
    if (err2 < 0 && err == 0)
      err = err2;
  }

  return err;
}

// struct rtnl_bridge_vlan {
//	NLHDR_COMMON
//	uint32_t ifindex;
//	uint8_t family;
//
//	uint16_t vlan_id;
//	uint16_t flags;
//	uint16_t range;
//	uint8_t state;
int nl_bridge::set_pvlan_stp(struct rtnl_bridge_vlan *bvlan_info) {
  int err = 0;

#ifdef HAVE_NETLINK_ROUTE_BRIDGE_VLAN_H
  uint32_t ifindex = rtnl_bridge_vlan_get_ifindex(bvlan_info);
  int port_id = nl->get_port_id(ifindex);
  uint16_t vlan_id = rtnl_bridge_vlan_get_vlan_id(bvlan_info);
  uint8_t stp_state = rtnl_bridge_vlan_get_state(bvlan_info);

  if (is_bridge_interface(ifindex))
    return err;

  err = add_port_vlan_stp_state(port_id, vlan_id, stp_state);
#endif
  return err;
}

int nl_bridge::drop_pvlan_stp(struct rtnl_bridge_vlan *bvlan_info) {
  int err = 0;

#ifdef HAVE_NETLINK_ROUTE_BRIDGE_VLAN_H
  uint32_t ifindex = rtnl_bridge_vlan_get_ifindex(bvlan_info);
  int port_id = nl->get_port_id(ifindex);
  uint16_t vlan_id = rtnl_bridge_vlan_get_vlan_id(bvlan_info);

  if (is_bridge_interface(ifindex))
    return err;

  err = del_port_vlan_stp_state(port_id, vlan_id);
#endif
  return err;
}

std::map<uint16_t, uint8_t>
nl_bridge::get_port_vlan_stp_states(rtnl_link *link) {
  uint32_t port_id = nl->get_port_id(link);
  if (port_id == 0)
    return {};

  return bridge_stp_states.get_min_states(port_id);
}

} /* namespace basebox */
