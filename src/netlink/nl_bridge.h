// SPDX-FileCopyrightText: © 2015 BISDN GmbH
// SPDX-License-Identifier: MPL-2.0-no-copyleft-exception

#pragma once

#include <cstdint>
#include <deque>
#include <list>
#include <map>
#include <memory>
#include <string>

#include <netlink/route/link/bridge.h>

#include "netlink-utils.h"

#define BR_STATE_DISABLED 0
#define BR_STATE_LISTENING 1
#define BR_STATE_LEARNING 2
#define BR_STATE_FORWARDING 3
#define BR_STATE_BLOCKING 4

extern "C" {
struct rtnl_bridge_vlan;
struct rtnl_link;
struct rtnl_neigh;
struct rtnl_mdb;
struct nl_addr;
}

namespace rofl {
class caddress_ll;
}

namespace basebox {

class cnetlink;
class nl_vlan;
class nl_vxlan;
class switch_interface;
class port_manager;
struct packet;

struct key {
  int port_id;
  uint16_t vid;

  key(int port_id, uint16_t vid) : port_id(port_id), vid(vid) {}
  bool operator<(const struct key &rhs) const {
    return std::tie(port_id, vid) < std::tie(rhs.port_id, rhs.vid);
  }
};

struct bridge_stp_states {
  // Global STP state: KEY : port_id + vid, state
  std::map<int, uint8_t> gl_states;
  std::map<uint16_t, std::map<uint16_t, uint8_t>> pv_states;

  bridge_stp_states() = default;
  bool add_pvlan_state(int port_id, uint16_t vlan_id, uint8_t state) {
    auto it = pv_states.find(vlan_id);
    if (it == pv_states.end()) {
      std::map<uint16_t, uint8_t> pv_map;
      pv_map.emplace(port_id, state);
      pv_states.emplace(vlan_id, pv_map);
      return true;
    }

    it->second.insert_or_assign(port_id, state);
    return false;
  }

  bool del_pvlan_state(int port_id, uint16_t vlan_id) {
    auto it = pv_states.find(vlan_id);
    if (it == pv_states.end()) {
      return false;
    }

    it->second.erase(port_id);

    if (it->second.empty()) {
      pv_states.erase(it);
      return true;
    }
    return false;
  }

  void add_global_state(int port_id, uint8_t state) {
    gl_states.insert_or_assign(port_id, state);
  }

  int get_global_state(int port_id) {
    auto it = gl_states.find(port_id);

    return (it == gl_states.end()) ? -EINVAL : it->second;
  }

  int get_pvlan_state(int port_id, uint16_t vid) {
    auto it = pv_states.find(vid);

    // no vlan found
    if (it == pv_states.end())
      return -EINVAL;

    // no port found
    auto pv_state = it->second.find(port_id);
    if (pv_state == it->second.end())
      return -EINVAL;

    return pv_state->second;
  }

  uint8_t get_effective_state(uint8_t g_state, uint8_t pv_state) {
    if (g_state == BR_STATE_BLOCKING || pv_state == BR_STATE_BLOCKING)
      return BR_STATE_BLOCKING;

    return std::min(g_state, pv_state);
  }

  std::map<uint16_t, uint8_t> get_min_states(int port_id) {
    int g_state = get_global_state(port_id);

    std::map<uint16_t, uint8_t> ret;
    for (auto it : pv_states) {
      auto pv_it = it.second.find(port_id);
      if (pv_it == it.second.end())
        continue;

      ret.emplace(it.first, get_effective_state(g_state, pv_it->second));
    }
    return ret;
  }

  int get_min_state(int port_id, uint16_t vid) {
    int g_state = get_global_state(port_id);
    int pv_state = get_pvlan_state(port_id, vid);

    if (pv_state < 0)
      return g_state;

    return get_effective_state(g_state, pv_state);
  }
};

class nl_bridge {
public:
  nl_bridge(switch_interface *sw, std::shared_ptr<port_manager> port_man,
            cnetlink *nl, std::shared_ptr<nl_vlan> vlan,
            std::shared_ptr<nl_vxlan> vxlan);

  virtual ~nl_bridge();

  int set_pvlan_stp(struct rtnl_bridge_vlan *bvlan_info);
  int drop_pvlan_stp(struct rtnl_bridge_vlan *bvlan_info);

  void set_bridge_interface(rtnl_link *);
  bool is_bridge_interface(int ifindex);
  bool is_bridge_interface(rtnl_link *);

  void add_interface(rtnl_link *);
  void update_interface(rtnl_link *old_link, rtnl_link *new_link);
  void delete_interface(rtnl_link *);

  void add_neigh_to_fdb(rtnl_neigh *, bool update = false);
  void remove_neigh_from_fdb(rtnl_neigh *);

  int mdb_update(rtnl_mdb *old_mdb, rtnl_mdb *new_mdb);

  bool is_mac_in_l2_cache(rtnl_neigh *n);
  int fdb_timeout(rtnl_link *br_link, uint16_t vid,
                  const rofl::caddress_ll &mac);
  int get_ifindex() { return bridge ? rtnl_link_get_ifindex(bridge) : 0; }

  uint32_t get_vlan_proto();
  uint32_t get_vlan_filtering();

  void clear_tpid_entries();

  // XXX Improve cache search mechanism
  std::deque<rtnl_neigh *> get_fdb_entries_of_port(rtnl_link *br_port,
                                                   uint16_t vid = 0,
                                                   nl_addr *lladdr = nullptr);

  void get_bridge_ports(
      std::tuple<std::shared_ptr<port_manager>, std::deque<rtnl_link *> *>
          &params, // XXX TODO make a struct here
      rtnl_link *br_port) noexcept;

  bool is_port_flooding(rtnl_link *br_link) const;

  int update_access_ports(rtnl_link *vxlan_link, rtnl_link *br_link,
                          const uint32_t tunnel_id, bool add);

  void set_port_locked(rtnl_link *link, bool locked);

  int bond_member_attached(rtnl_link *bond, rtnl_link *member);
  int bond_member_detached(rtnl_link *bond, rtnl_link *member);

private:
  struct bridge_stp_states bridge_stp_states;

  void update_vlans(rtnl_link *, rtnl_link *);

  void update_access_ports(rtnl_link *vxlan_link, rtnl_link *br_link,
                           const uint16_t vid, const uint32_t tunnel_id,
                           const std::deque<rtnl_link *> &bridge_ports,
                           bool add);

  void set_port_stp_state(uint32_t port_id, uint8_t stp_state);
  int add_port_vlan_stp_state(uint32_t port_id, uint16_t vid,
                              uint8_t stp_state);
  int del_port_vlan_stp_state(uint32_t port_id, uint16_t vid);
  int set_port_vlan_stp_state(uint32_t port_id, uint16_t vid, uint8_t state);

  int set_vlan_proto(rtnl_link *link, uint32_t port_id);
  int delete_vlan_proto(rtnl_link *link, uint32_t port_id);

  int mdb_to_set(rtnl_mdb *,
                 std::set<std::tuple<uint32_t, uint16_t, rofl::caddress_ll>> *);

  rtnl_link *bridge;
  switch_interface *sw;
  std::shared_ptr<port_manager> port_man;
  cnetlink *nl;
  std::shared_ptr<nl_vlan> vlan;
  std::shared_ptr<nl_vxlan> vxlan;
  std::unique_ptr<nl_cache, decltype(&nl_cache_free)> l2_cache;

  rtnl_link_bridge_vlan empty_br_vlan;
  uint32_t vxlan_dom_bitmap[RTNL_LINK_BRIDGE_VLAN_BITMAP_LEN];
  std::map<uint16_t, uint32_t> vxlan_domain;

  nl_addr *ipv4_igmp;
  nl_addr *ipv6_all_hosts;
  nl_addr *ipv6_mld;
};

} /* namespace basebox */
