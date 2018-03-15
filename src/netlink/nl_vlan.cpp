#include <cassert>
#include <glog/logging.h>

#include "sai.hpp"
#include "tap_manager.hpp"
#include "nl_vlan.hpp"

namespace basebox {

nl_vlan::nl_vlan(std::shared_ptr<tap_manager> tap_man, cnetlink *nl)
    : swi(nullptr), tap_man(tap_man) {}

int nl_vlan::add_vlan(int ifindex, uint16_t vid, bool tagged) {
  assert(swi);

  VLOG(2) << __FUNCTION__ << ": add vid=" << vid << " tagged=" << tagged;
  // XXX FIXME add vlan range check

  uint32_t port_id = ifindex; // tap_man->get_port_id(ifindex);

  if (port_id == 0) {
    VLOG(1) << __FUNCTION__ << ": unknown port with ifindex=" << ifindex;
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

  // XXX TODO store the vlan interface usage

  return rv;
}

int nl_vlan::remove_vlan(int ifindex, uint16_t vid, bool tagged) {
  assert(swi);

  int rv = 0;

  uint32_t port_id = tap_man->get_port_id(ifindex);

  if (port_id == 0) {
    VLOG(1) << __FUNCTION__ << ": unknown port with ifindex=" << ifindex;
    return -EINVAL;
  }

  // remove vid at ingress
  rv = swi->ingress_port_vlan_remove(port_id, vid, !tagged);

  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": failed to remove vid=" << vid
               << "(tagged=" << tagged << ") at ifindex= " << ifindex
               << " rv=" << rv;
    return rv;
  }

  // delete all FM pointing to this group first
  rv = swi->l2_addr_remove_all_in_vlan(port_id, vid);

  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__
               << ": failed to remove all bridge entries in vid=" << vid
               << " at ifindex= " << ifindex << " rv=" << rv;
    return rv;
  }

  // remove vid from egress
  rv = swi->egress_bridge_port_vlan_remove(port_id, vid);

  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": failed to remove vid=" << vid
               << " at ifindex= " << ifindex << " rv=" << rv;
    return rv;
  }

  return rv;
}

} // namespace basebox
