/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cassert>

#include <glog/logging.h>
#include <netlink/route/link.h>
#include <netlink/route/link/vlan.h>

#include "cnetlink.h"
#include "nl_output.h"
#include "nl_vlan.h"
#include "sai.h"

namespace basebox {

nl_vlan::nl_vlan(cnetlink *nl) : swi(nullptr), nl(nl) {}

int nl_vlan::add_vlan(rtnl_link *link, uint16_t vid, bool tagged,
                      uint16_t vrf_id) {
  assert(swi);

  VLOG(2) << __FUNCTION__ << ": add vid=" << vid << " tagged=" << tagged
          << " vrf=" << vrf_id;
  if (!is_vid_valid(vid)) {
    LOG(ERROR) << __FUNCTION__ << ": invalid vid " << vid;
    return -EINVAL;
  }

  uint32_t port_id = nl->get_port_id(link);

  if (port_id == 0) {
    VLOG(1) << __FUNCTION__ << ": unknown interface " << OBJ_CAST(link);
    return -EINVAL;
  }

  int rv = swi->ingress_port_vlan_add(port_id, vid, !tagged, vrf_id);
  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__
               << ": failed to setup ingress vlan " << vid
               << (tagged ? " (tagged)" : " (untagged)")
               << " on port_id=" << port_id << "; rv=" << rv;
    return rv;
  }

  if (nbi::get_port_type(port_id) == nbi::port_type_lag) {
    auto members = nl->get_bond_members_by_port_id(port_id);
    for (auto mem : members) {
      rv = swi->ingress_port_vlan_add(mem, vid, !tagged, vrf_id);
      if (rv < 0) {
        LOG(ERROR) << __FUNCTION__
                   << ": failed to setup ingress vlan " << vid
		   << (tagged ? " (tagged)" : " (untagged)")
                   << " on port_id=" << port_id << "; rv=" << rv;
	break;
      }
    }

    if (rv < 0) {
      for (auto mem : members) {
        (void)swi->ingress_port_vlan_remove(mem, vid, !tagged, vrf_id);
      }
      return rv;
    }
  }

  // setup egress interface
  rv = swi->egress_port_vlan_add(port_id, vid, !tagged);

  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__
               << ": failed to setup egress vlan " << vid
               << (tagged ? " (tagged)" : " (untagged)")
               << " on port_id=" << port_id << "; rv=" << rv;
    (void)swi->ingress_port_vlan_remove(port_id, vid, !tagged);

    if (nbi::get_port_type(port_id) == nbi::port_type_lag) {
      auto members = nl->get_bond_members_by_port_id(port_id);
      for (auto mem : members) {
        (void)swi->ingress_port_vlan_remove(mem, vid, !tagged, vrf_id);
      }
    }

    return rv;
  }

  auto key = std::make_pair(port_id, vid);
  auto refcount = port_vlan.find(key);
  if (refcount == port_vlan.end()) {
    port_vlan.emplace(key, 1);
  } else
    refcount->second++;

  if (nbi::get_port_type(port_id) == nbi::port_type_lag) {
    auto members = nl->get_bond_members_by_port_id(port_id);
    for (auto mem : members) {
      rv = swi->egress_port_vlan_add(mem, vid, !tagged);
      if (rv < 0) {
        LOG(ERROR) << __FUNCTION__
                   << ": failed to setup egress vlan " << vid
		   << (tagged ? " (tagged)" : " (untagged)")
                   << " on port_id=" << port_id << "; rv=" << rv;
	break;
      }
    }

    if (rv < 0) {
      for (auto mem : members) {
        (void)swi->egress_port_vlan_remove(mem, vid);
        (void)swi->ingress_port_vlan_remove(mem, vid, !tagged, vrf_id);
      }
      (void)swi->egress_port_vlan_remove(port_id, vid);
      (void)swi->ingress_port_vlan_remove(port_id, vid, !tagged);

      return rv;
    }
  }

  return rv;
}

// add vid at ingress
int nl_vlan::add_ingress_vlan(uint32_t port_id, uint16_t vid, bool tagged,
                                 uint16_t vrf_id) {

  return swi->ingress_port_vlan_add(port_id, vid, !tagged, vrf_id);
}

// remove vid at ingress
int nl_vlan::remove_ingress_vlan(uint32_t port_id, uint16_t vid, bool tagged,
                                 uint16_t vrf_id) {

  return swi->ingress_port_vlan_remove(port_id, vid, !tagged, vrf_id);
}

int nl_vlan::remove_vlan(rtnl_link *link, uint16_t vid, bool tagged,
                         uint16_t vrf_id) {
  assert(swi);

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
  if (refcount != port_vlan.end()) {
    refcount->second--;
    return rv;
  } else if (refcount->second == 1)
    port_vlan.erase(refcount);

  // remove vid at ingress
  if (nbi::get_port_type(port_id) == nbi::port_type_lag) {
    auto members = nl->get_bond_members_by_port_id(port_id);
    for (auto mem : members) {
      (void)swi->ingress_port_vlan_remove(mem, vid, !tagged, vrf_id);
    }
  }
  rv = swi->ingress_port_vlan_remove(port_id, vid, !tagged, vrf_id);

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
    if (rtnl_link_get_ifindex(link) != 1)
      LOG(ERROR) << __FUNCTION__ << ": unsupported link type " << lt
                 << " of link " << OBJ_CAST(link);
    break;
  }

  VLOG(2) << __FUNCTION__ << ": vid=" << vid << " interface=" << OBJ_CAST(link);

  return vid;
}

} // namespace basebox
