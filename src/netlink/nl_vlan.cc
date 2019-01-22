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

int nl_vlan::add_vlan(rtnl_link *link, uint16_t vid, bool tagged) const {
  assert(swi);

  VLOG(2) << __FUNCTION__ << ": add vid=" << vid << " tagged=" << tagged;
  if (!is_vid_valid(vid)) {
    LOG(ERROR) << __FUNCTION__ << ": invalid vid " << vid;
    return -EINVAL;
  }

  uint32_t port_id = nl->get_port_id(link);

  if (port_id == 0) {
    VLOG(1) << __FUNCTION__ << ": unknown interface " << OBJ_CAST(link);
    return -EINVAL;
  }

  int rv =
      swi->ingress_port_vlan_add(port_id, vid, !tagged /* pvid == !tagged */);
  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__
               << ": failed to setup ingress vlan 1 (untagged) on port_id="
               << port_id << "; rv=" << rv;
    return rv;
  }

  // setup egress interface
  rv = swi->egress_port_vlan_add(port_id, vid, !tagged);

  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__
               << ": failed to setup egress vlan 1 (untagged) on port_id="
               << port_id << "; rv=" << rv;
    (void)swi->ingress_port_vlan_remove(port_id, 1, true);

    return rv;
  }

  // XXX TODO store the vlan interface usage?

  return rv;
}

int nl_vlan::remove_vlan(rtnl_link *link, uint16_t vid, bool tagged) const {
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

  // remove vid at ingress
  rv = swi->ingress_port_vlan_remove(port_id, vid, !tagged);

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
  rv = swi->egress_bridge_port_vlan_remove(port_id, vid);

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

  if (pport_id == 0)
    return 0;

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
    vid = rtnl_link_vlan_get_id(link);
    break;
  default:
    if (rtnl_link_get_ifindex(link) != 1)
      LOG(ERROR) << __FUNCTION__ << ": unsupported link type " << lt
                 << " of link " << OBJ_CAST(link);
    break;
  }

  return vid;
}

} // namespace basebox
