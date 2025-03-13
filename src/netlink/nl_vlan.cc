/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cassert>

#include <glog/logging.h>
#include <netlink/route/link.h>
#include <netlink/route/link/vlan.h>
#include <netlink/route/link/vrf.h>
#include <netlink/route/neighbour.h>

#include "cnetlink.h"
#include "nl_output.h"
#include "nl_vlan.h"
#include "sai.h"

namespace basebox {

nl_vlan::nl_vlan(cnetlink *nl) : swi(nullptr), nl(nl) {}

int nl_vlan::add_vlan(rtnl_link *link, uint16_t vid, bool tagged) {
  assert(swi);

  if (!is_vid_valid(vid)) {
    LOG(ERROR) << __FUNCTION__ << ": invalid vid " << vid;
    return -EINVAL;
  }

  uint16_t vrf_id = get_vrf_id(vid, link);

  VLOG(2) << __FUNCTION__ << ": add vid=" << vid << " tagged=" << tagged
          << " vrf=" << vrf_id;

  uint32_t port_id = nl->get_port_id(link);

  if (port_id == 0) {
    VLOG(1) << __FUNCTION__ << ": unknown interface " << OBJ_CAST(link);
    return -EINVAL;
  }

  auto key = std::make_pair(port_id, vid);
  auto refcount = port_vlan.find(key);
  if (refcount != port_vlan.end()) {
    refcount->second++;
    return 0;
  }

  int rv = enable_vlan(port_id, vid, tagged, vrf_id);
  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": failed to enable vlan " << vid
               << (tagged ? " (tagged)" : " (untagged)")
               << " on port_id=" << port_id << "; rv=" << rv;
    return rv;
  }

  port_vlan.emplace(key, 1);

  return rv;
}

// add vid at ingress
int nl_vlan::add_ingress_vlan(rtnl_link *link, uint16_t vid, bool tagged) {
  assert(swi);

  uint16_t vrf_id = get_vrf_id(vid, link);
  uint32_t port_id = nl->get_port_id(link);

  return add_ingress_vlan(port_id, vid, tagged, vrf_id);
}

// add vid at ingress
int nl_vlan::add_ingress_vlan(uint32_t port_id, uint16_t vid, bool tagged,
                              uint16_t vrf_id) {

  int rv;

  rv = swi->ingress_port_vlan_add(port_id, vid, !tagged, vrf_id);
  if (rv < 0)
    return rv;

  if (nbi::get_port_type(port_id) == nbi::port_type_lag) {
    auto members = nl->get_bond_members_by_port_id(port_id);

    for (auto mem : members) {
      rv = swi->ingress_port_vlan_add(mem, vid, !tagged, vrf_id);
      if (rv < 0)
        break;
    }
  }

  if (rv < 0) {
    (void)remove_ingress_vlan(port_id, vid, tagged, vrf_id);
  }

  return rv;
}

// add pvid at ingress
int nl_vlan::add_ingress_pvid(rtnl_link *link, uint16_t pvid) {
  assert(swi);

  int rv;
  uint32_t port_id = nl->get_port_id(link);

  auto pvid_config = port_pvid.find(port_id);
  if (pvid_config != port_pvid.end()) {
    LOG(WARNING) << __FUNCTION__ << ": vid " << pvid_config->second
                 << " was aleady configured as PVID for port_id " << port_id;
    return -EINVAL;
  } else {
    port_pvid.emplace(port_id, pvid);
  }

  if (nbi::get_port_type(port_id) == nbi::port_type_lag) {
    auto members = nl->get_bond_members_by_port_id(port_id);

    for (auto mem : members) {
      rv = swi->ingress_port_pvid_add(mem, pvid);
      if (rv < 0)
        break;
    }
  }

  return swi->ingress_port_pvid_add(port_id, pvid);
}

// remove pvid at ingress
int nl_vlan::remove_ingress_pvid(rtnl_link *link, uint16_t pvid) {
  assert(swi);

  int rv;
  uint32_t port_id = nl->get_port_id(link);

  auto pvid_config = port_pvid.find(port_id);
  if (pvid_config != port_pvid.end()) {
    if (pvid_config->second != pvid) {
      LOG(WARNING) << __FUNCTION__ << ": vid " << pvid_config->second
                   << " was aleady configured as PVID for port_id " << port_id;
      return -EINVAL;
    }

    port_pvid.erase(pvid_config);
  } else {
    LOG(WARNING) << __FUNCTION__ << ": vid " << pvid
                 << " was not configured as PVID for port_id " << port_id;
    return -EINVAL;
  }

  if (nbi::get_port_type(port_id) == nbi::port_type_lag) {
    auto members = nl->get_bond_members_by_port_id(port_id);

    for (auto mem : members) {
      rv = swi->ingress_port_pvid_remove(mem, pvid);
      if (rv < 0)
        break;
    }
  }

  return swi->ingress_port_pvid_remove(port_id, pvid);
}

// remove vid at ingress
int nl_vlan::remove_ingress_vlan(rtnl_link *link, uint16_t vid, bool pvid) {
  assert(swi);

  uint32_t port_id = nl->get_port_id(link);

  return remove_ingress_vlan(port_id, vid, pvid);
}

// remove vid at ingress
int nl_vlan::remove_ingress_vlan(uint32_t port_id, uint16_t vid, bool pvid,
                                 uint16_t vrf_id) {

  if (nbi::get_port_type(port_id) == nbi::port_type_lag) {
    auto members = nl->get_bond_members_by_port_id(port_id);

    for (auto mem : members) {
      swi->ingress_port_vlan_remove(mem, vid, !pvid, vrf_id);
    }
  }

  return swi->ingress_port_vlan_remove(port_id, vid, !pvid, vrf_id);
}

int nl_vlan::remove_vlan(rtnl_link *link, uint16_t vid, bool tagged) {
  assert(swi);

  uint16_t vrf_id = get_vrf_id(vid, link);

  if (!is_vid_valid(vid)) {
    LOG(ERROR) << __FUNCTION__ << ": invalid vid " << vid;
    return -EINVAL;
  }

  int rv = 0;

  uint32_t port_id = nl->get_port_id(link);

  if (port_id == 0) {
    VLOG(1) << __FUNCTION__ << ": unknown link " << OBJ_CAST(link);
    return -EINVAL;
  }

  // check for refcount
  auto key = std::make_pair(port_id, vid);
  auto refcount = port_vlan.find(key);
  if (refcount == port_vlan.end())
    return -EINVAL;

  if (--refcount->second > 0)
    return rv;

  port_vlan.erase(refcount);

  // remove vid at ingress
  rv = disable_vlan(port_id, vid, tagged, vrf_id);
  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": failed with rv=" << rv
               << " to remove vid=" << vid << "(tagged=" << tagged
               << ") of link " << OBJ_CAST(link);
  }

  return rv;
}

int nl_vlan::enable_vlan(uint32_t port_id, uint16_t vid, bool tagged,
                         uint16_t vrf_id) {
  assert(swi);

  int rv = add_ingress_vlan(port_id, vid, tagged, vrf_id);
  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": failed to setup ingress vlan " << vid
               << (tagged ? " (tagged)" : " (untagged)")
               << " on port_id=" << port_id << "; rv=" << rv;
    return rv;
  }

  // setup egress interface
  rv = swi->egress_port_vlan_add(port_id, vid, !tagged);

  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": failed to setup egress vlan " << vid
               << (tagged ? " (tagged)" : " (untagged)")
               << " on port_id=" << port_id << "; rv=" << rv;
    (void)remove_ingress_vlan(port_id, vid, tagged, vrf_id);

    return rv;
  }

  if (nbi::get_port_type(port_id) == nbi::port_type_lag) {
    auto members = nl->get_bond_members_by_port_id(port_id);
    for (auto mem : members) {
      rv = swi->egress_port_vlan_add(mem, vid, !tagged);
      if (rv < 0) {
        LOG(ERROR) << __FUNCTION__ << ": failed to setup egress vlan " << vid
                   << (tagged ? " (tagged)" : " (untagged)")
                   << " on port_id=" << port_id << "; rv=" << rv;
        break;
      }
    }

    if (rv < 0) {
      for (auto mem : members) {
        (void)swi->egress_port_vlan_remove(mem, vid);
      }
      (void)swi->egress_port_vlan_remove(port_id, vid);
      (void)remove_ingress_vlan(port_id, vid, tagged, vrf_id);

      return rv;
    }
  }

  return rv;
}

int nl_vlan::disable_vlan(uint32_t port_id, uint16_t vid, bool tagged,
                          uint16_t vrf_id) {
  assert(swi);

  // remove vid at ingress
  int rv = remove_ingress_vlan(port_id, vid, tagged, vrf_id);
  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": failed with rv=" << rv
               << " to remove vid=" << vid << "(tagged=" << tagged
               << ") of link " << OBJ_CAST(link);
    return rv;
  }

  // delete all FM pointing to this group first
  rv = swi->l2_addr_remove_all_in_vlan(port_id, vid);

  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": failed with rv= " << rv
               << " to remove all bridge entries in vid=" << vid << " link "
               << OBJ_CAST(link);
    return rv;
  }

  // remove vid from egress
  if (nbi::get_port_type(port_id) == nbi::port_type_lag) {
    auto members = nl->get_bond_members_by_port_id(port_id);
    for (auto mem : members) {
      (void)swi->egress_port_vlan_remove(mem, vid);
    }
  }
  rv = swi->egress_port_vlan_remove(port_id, vid);

  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": failed with rv= " << rv
               << " to remove vid=" << vid << " of link " << OBJ_CAST(link);
    return rv;
  }

  return rv;
}

int nl_vlan::enable_vlans(rtnl_link *link) {
  assert(swi);

  uint32_t port_id = nl->get_port_id(link);

  if (port_id == 0) {
    VLOG(1) << __FUNCTION__ << ": unknown interface " << OBJ_CAST(link);
    return -EINVAL;
  }

  for (auto const &it : port_vlan) {
    if (it.first.first != port_id)
      continue;

    uint16_t vid = it.first.second;
    uint16_t vrf_id = get_vrf_id(vid, link);

    // assume the default vlan is untagged
    (void)enable_vlan(port_id, it.first.second, vid != default_vid, vrf_id);
  }

  return 0;
}

int nl_vlan::disable_vlans(rtnl_link *link) {
  assert(swi);

  uint32_t port_id = nl->get_port_id(link);

  if (port_id == 0) {
    VLOG(1) << __FUNCTION__ << ": unknown interface " << OBJ_CAST(link);
    return -EINVAL;
  }

  for (auto const &it : port_vlan) {
    if (it.first.first != port_id)
      continue;

    uint16_t vid = it.first.second;
    uint16_t vrf_id = get_vrf_id(vid, link);

    // assume the default vlan is untagged
    (void)disable_vlan(port_id, it.first.second, vid != default_vid, vrf_id);
  }

  return 0;
}

int nl_vlan::add_bridge_vlan(rtnl_link *link, uint16_t vid, bool pvid,
                             bool tagged) {
  int rv;
  assert(swi);

  uint16_t vrf_id = get_vrf_id(vid, link);

  VLOG(2) << __FUNCTION__ << ": add vid=" << vid << " pvid=" << pvid
          << " tagged=" << tagged << " vrf=" << vrf_id;

  if (!is_vid_valid(vid)) {
    LOG(ERROR) << __FUNCTION__ << ": invalid vid " << vid;
    return -EINVAL;
  }

  uint32_t port_id = nl->get_port_id(link);

  if (port_id == 0) {
    VLOG(1) << __FUNCTION__ << ": unknown interface " << OBJ_CAST(link);
    return -EINVAL;
  }

  auto vlans = bridge_vlan.find(port_id);
  if (vlans == bridge_vlan.end()) {
    std::map<uint16_t, bool> _vlans;
    _vlans.emplace(vid, tagged);
    bridge_vlan.emplace(port_id, _vlans);
  } else {
    if (vlans->second.find(vid) == vlans->second.end()) {
      vlans->second.emplace(vid, tagged);
    } else {
      LOG(WARNING) << __FUNCTION__ << ": vid " << vid
                   << " is already configured for port_id " << port_id;
    }
  }

  auto pvid_config = port_pvid.find(port_id);
  if (pvid_config != port_pvid.end()) {
    if (!pvid && pvid_config->second == vid)
      LOG(WARNING) << __FUNCTION__ << ": vid " << pvid_config->second
                   << " was aleady configured as PVID for port_id " << port_id;
    if (pvid && pvid_config->second != vid)
      LOG(WARNING) << __FUNCTION__ << ": vid " << pvid_config->second
                   << " was aleady configured as PVID for port_id " << port_id;
  } else {
    if (pvid)
      port_pvid.emplace(port_id, vid);
  }

  rv = swi->egress_bridge_port_vlan_add(port_id, vid, !tagged);
  if (rv < 0)
    return rv;

  rv = swi->ingress_port_vlan_add(port_id, vid, pvid, vrf_id);
  if (rv < 0) {
    swi->egress_bridge_port_vlan_remove(port_id, vid);
    return rv;
  }

  if (nbi::get_port_type(port_id) == nbi::port_type_lag) {
    auto members = nl->get_bond_members_by_port_id(port_id);

    for (auto mem : members) {
      swi->egress_bridge_port_vlan_add(mem, vid, !tagged);
      swi->ingress_port_vlan_add(mem, vid, pvid, vrf_id);
    }

    if (rv < 0) {
      remove_bridge_vlan(link, vid, pvid, tagged);
    }
  }

  return rv;
}

int nl_vlan::update_egress_bridge_vlan(rtnl_link *link, uint16_t vid,
                                       bool untagged) {
  int rv = 0;
  assert(swi);

  if (!is_vid_valid(vid)) {
    LOG(ERROR) << __FUNCTION__ << ": invalid vid " << vid;
    return -EINVAL;
  }

  uint32_t port_id = nl->get_port_id(link);

  if (port_id == 0) {
    VLOG(1) << __FUNCTION__ << ": unknown interface " << OBJ_CAST(link);
    return -EINVAL;
  }

  auto vlans = bridge_vlan.find(port_id);
  if (vlans == bridge_vlan.end()) {
    LOG(WARNING) << __FUNCTION__ << ": vid " << vid
                 << " was not configured for port_id " << port_id;
    return -EINVAL;
  } else {
    auto vlan_config = vlans->second.find(vid);
    if (vlan_config == vlans->second.end()) {
      LOG(WARNING) << __FUNCTION__ << ": vid " << vid
                   << " was not configured for port_id " << port_id;
      return -EINVAL;
    }

    if (vlan_config->second == !untagged)
      LOG(WARNING) << __FUNCTION__ << ": vid " << vid
                   << " was already configured as " << (untagged ? "un" : "")
                   << "tagged for port_id " << port_id;
    vlan_config->second = !untagged;
  }

  VLOG(2) << __FUNCTION__ << ": update egress vid=" << vid
          << " tagged=" << untagged;

  std::set<uint32_t> ports;
  if (nbi::get_port_type(port_id) == nbi::port_type_lag) {
    ports = nl->get_bond_members_by_port_id(port_id);
  }
  ports.insert(port_id);

  for (auto port : ports) {
    rv = swi->egress_port_vlan_add(port, vid, untagged, true);
    if (rv < 0)
      break;
  }

  return rv;
}

void nl_vlan::remove_bridge_vlan(rtnl_link *link, uint16_t vid, bool pvid,
                                 bool tagged) {
  assert(swi);

  uint16_t vrf_id = get_vrf_id(vid, link);

  VLOG(2) << __FUNCTION__ << ": remove vid=" << vid << " pvid=" << pvid
          << " tagged=" << tagged << " vrf=" << vrf_id;

  if (!is_vid_valid(vid)) {
    LOG(ERROR) << __FUNCTION__ << ": invalid vid " << vid;
    return;
  }

  uint32_t port_id = nl->get_port_id(link);

  if (port_id == 0) {
    VLOG(1) << __FUNCTION__ << ": unknown interface " << OBJ_CAST(link);
    return;
  }

  auto vlans = bridge_vlan.find(port_id);
  if (vlans != bridge_vlan.end()) {
    if (vlans->second.find(vid) != vlans->second.end())
      vlans->second.erase(vid);
    else
      LOG(WARNING) << __FUNCTION__ << ": vid " << vid
                   << " was not configured for port_id " << port_id;
    if (vlans->second.empty())
      bridge_vlan.erase(vlans);
  } else {
    LOG(WARNING) << __FUNCTION__ << ": vid " << vid
                 << " was not configured for port_id " << port_id;
  }

  auto pvid_config = port_pvid.find(port_id);
  if (pvid_config == port_pvid.end()) {
    if (pvid)
      LOG(WARNING) << __FUNCTION__ << ": vid " << vid
                   << " was not configured as PVID for port_id " << port_id;
  } else {
    if (!pvid || pvid_config->second != vid)
      LOG(WARNING) << __FUNCTION__ << ": vid " << vid
                   << " was not configured as PVID for port_id " << port_id;
    if (pvid)
      port_pvid.erase(pvid_config);
  }

  if (nbi::get_port_type(port_id) == nbi::port_type_lag) {
    auto members = nl->get_bond_members_by_port_id(port_id);

    for (auto mem : members) {
      swi->egress_bridge_port_vlan_remove(mem, vid);
      swi->ingress_port_vlan_remove(mem, vid, pvid, vrf_id);
    }
  }

  swi->egress_bridge_port_vlan_remove(port_id, vid);
  swi->ingress_port_vlan_remove(port_id, vid, pvid, vrf_id);
}

uint16_t nl_vlan::get_vid(rtnl_link *link) {
  uint16_t vid = 0;
  uint32_t pport_id = nl->get_port_id(link);
  auto lt = get_link_type(link);

  if (!nl->is_bridge_interface(link) && pport_id == 0) {
    VLOG(2) << __FUNCTION__ << ": invalid interface";
    return vid;
  }

  switch (lt) {
  case LT_BRIDGE:
    VLOG(2) << __FUNCTION__ << ": bridge default vid " << default_vid;
    vid = default_vid;
    break;
  case LT_TUN:
  case LT_BOND:
    vid = default_vid;
    break;
  case LT_VLAN:
  case LT_VRF_SLAVE:
    vid = rtnl_link_vlan_get_id(link);
    break;
  default:
    // port or bond interface
    if (nl->get_port_id(link) > 0)
      vid = default_vid;
    else if (rtnl_link_get_ifindex(link) != 1)
      LOG(ERROR) << __FUNCTION__ << ": unsupported link type " << lt
                 << " of link " << OBJ_CAST(link);
    break;
  }

  VLOG(2) << __FUNCTION__ << ": vid=" << vid << " interface=" << OBJ_CAST(link);

  return vid;
}

uint16_t nl_vlan::get_vrf_id(uint16_t vid, rtnl_link *link) {
  auto vlanmap = vlan_vrf.find(vid);
  if (vlanmap == vlan_vrf.end()) {
    return 0;
  }

  return vlanmap->second;
}

void nl_vlan::vrf_attach(rtnl_link *old_link, rtnl_link *new_link) {
  uint16_t vid = get_vid(new_link);
  if (vid == 0)
    return;

  uint16_t vrf = get_vrf_id(vid, new_link);
  if (vrf == 0) {
    vrf = nl->get_vrf_table_id(new_link);
    if (vrf <= 0) // not found, doesnt exist
      VLOG(1) << __FUNCTION__ << ": did not find correct VRF";

    vlan_vrf.insert(std::make_pair(vid, vrf));
  }

  LOG(INFO) << __FUNCTION__ << ": map vlan=" << vid << " vrf=" << vrf;

  // delete old entries
  auto fdb_entries = nl->search_fdb(vid, nullptr);
  for (auto entry : fdb_entries) {
    auto link = nl->get_link_by_ifindex(rtnl_neigh_get_ifindex(entry));

    remove_ingress_vlan(nl->get_port_id(link.get()), vid, true);
  }

  // add updated entries
  for (auto entry : fdb_entries) {
    auto link = nl->get_link_by_ifindex(rtnl_neigh_get_ifindex(entry));

    add_ingress_vlan(nl->get_port_id(link.get()), vid, true, vrf);
  }

  VLOG(1) << __FUNCTION__ << ": attached=" << OBJ_CAST(new_link)
          << " to VRF id=" << vrf;
}

void nl_vlan::vrf_detach(rtnl_link *old_link, rtnl_link *new_link) {
  uint16_t vid = get_vid(new_link);
  if (vid == 0)
    return;

  auto vrf = vlan_vrf.find(vid);
  if (vrf == vlan_vrf.end()) {
    return;
  }

  // delete old entries
  auto fdb_entries = nl->search_fdb(vid, nullptr);
  for (auto entry : fdb_entries) {
    auto link = nl->get_link_by_ifindex(rtnl_neigh_get_ifindex(entry));

    remove_ingress_vlan(nl->get_port_id(link.get()), vid, true, vrf->second);
  }

  // add updated entries
  for (auto entry : fdb_entries) {
    auto link = nl->get_link_by_ifindex(rtnl_neigh_get_ifindex(entry));

    add_ingress_vlan(nl->get_port_id(link.get()), vid, true);
  }

  VLOG(1) << __FUNCTION__ << ": detached=" << OBJ_CAST(new_link)
          << " to VRF id=" << vrf->second;
  vlan_vrf.erase(vrf);
}

int nl_vlan::bond_member_attached(rtnl_link *bond, rtnl_link *member) {
  uint32_t bond_id = nl->get_port_id(bond);
  uint32_t port_id = nl->get_port_id(member);

  if (bond_id == 0) {
    VLOG(1) << __FUNCTION__ << ": unknown interface " << OBJ_CAST(bond);
    return -EINVAL;
  }

  if (port_id == 0) {
    VLOG(1) << __FUNCTION__ << ": unknown interface " << OBJ_CAST(member);
    return -EINVAL;
  }

  if (rtnl_link_get_master(bond)) {
    // check bridge attachement
    auto br_link = nl->get_link(rtnl_link_get_ifindex(bond), AF_BRIDGE);

    // bridge VLANs are handled by bridge
    if (br_link) {
      auto vlans = bridge_vlan.find(bond_id);
      if (vlans == bridge_vlan.end())
        return 0;

      uint16_t pvid = 0;

      auto pv_it = port_pvid.find(bond_id);
      if (pv_it != port_pvid.end())
        pvid = pv_it->second;

      for (auto const &it : vlans->second) {
        uint16_t vid = it.first;
        bool tagged = it.second;
        uint16_t vrf_id = get_vrf_id(vid, bond);

        swi->egress_bridge_port_vlan_add(port_id, vid, !tagged);
        swi->ingress_port_vlan_add(port_id, vid, vid == pvid, vrf_id);
      }

      return 0;
    }
  }

  for (auto const &it : port_vlan) {
    if (it.first.first != bond_id)
      continue;

    uint16_t vid = it.first.second;
    uint16_t vrf_id = get_vrf_id(vid, bond);

    // assume the default vlan is untagged
    (void)enable_vlan(port_id, it.first.second, vid != default_vid, vrf_id);
  }

  return 0;
}

int nl_vlan::bond_member_detached(rtnl_link *bond, rtnl_link *member) {
  uint32_t bond_id = nl->get_port_id(bond);
  uint32_t port_id = nl->get_port_id(member);

  if (bond_id == 0) {
    VLOG(1) << __FUNCTION__ << ": unknown interface " << OBJ_CAST(bond);
    return -EINVAL;
  }

  if (port_id == 0) {
    VLOG(1) << __FUNCTION__ << ": unknown interface " << OBJ_CAST(member);
    return -EINVAL;
  }

  if (rtnl_link_get_master(bond)) {
    // check bridge attachement
    auto br_link = nl->get_link(rtnl_link_get_ifindex(bond), AF_BRIDGE);

    if (br_link) {
      auto vlans = bridge_vlan.find(bond_id);
      if (vlans == bridge_vlan.end())
        return 0;

      uint16_t pvid = 0;

      auto pv_it = port_pvid.find(bond_id);
      if (pv_it != port_pvid.end())
        pvid = pv_it->second;

      for (auto const &it : vlans->second) {
        uint16_t vid = it.first;
        uint16_t vrf_id = get_vrf_id(vid, bond);

        swi->egress_bridge_port_vlan_remove(port_id, vid);
        swi->ingress_port_vlan_remove(port_id, vid, vid == pvid, vrf_id);
      }

      return 0;
    }
  }

  for (auto const &it : port_vlan) {
    if (it.first.first != bond_id)
      continue;

    uint16_t vid = it.first.second;
    uint16_t vrf_id = get_vrf_id(vid, bond);

    // assume the default vlan is untagged
    (void)disable_vlan(port_id, it.first.second, vid != default_vid, vrf_id);
  }

  return 0;
}

} // namespace basebox
