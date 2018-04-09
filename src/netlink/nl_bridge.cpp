/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cassert>
#include <cstring>
#include <map>

#include <glog/logging.h>
#include <netlink/route/link.h>
#include <netlink/route/link/bridge.h>
#include <netlink/route/neighbour.h>
#include <rofl/common/openflow/cofport.h>

#include "cnetlink.hpp"
#include "nl_bridge.hpp"
#include "nl_output.hpp"
#include "sai.hpp"
#include "tap_manager.hpp"

namespace basebox {

nl_bridge::nl_bridge(switch_interface *sw, std::shared_ptr<tap_manager> tap_man,
                     cnetlink *nl)
    : bridge(nullptr), sw(sw), tap_man(tap_man), nl(nl) {
  memset(&empty_br_vlan, 0, sizeof(rtnl_link_bridge_vlan));
}

nl_bridge::~nl_bridge() { rtnl_link_put(bridge); }

void nl_bridge::set_bridge_interface(rtnl_link *bridge) {
  assert(bridge);

  this->bridge = bridge;
  nl_object_get(OBJ_CAST(bridge));

  sw->subscribe_to(switch_interface::SWIF_ARP);
}

static bool br_vlan_equal(const rtnl_link_bridge_vlan *lhs,
                          const rtnl_link_bridge_vlan *rhs) {
  assert(lhs);
  assert(rhs);
  return (memcmp(lhs, rhs, sizeof(struct rtnl_link_bridge_vlan)) == 0);
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

  update_vlans(nullptr, link);
}

void nl_bridge::update_interface(rtnl_link *old_link, rtnl_link *new_link) {
  LOG(INFO) << __FUNCTION__ << ": fam_old=" << rtnl_link_get_family(old_link)
            << ", fam_new=" << rtnl_link_get_family(new_link);
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
}

void nl_bridge::update_vlans(rtnl_link *old_link, rtnl_link *new_link) {
  assert(sw);

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
    if (old_br_vlan == nullptr) {
      old_br_vlan = &empty_br_vlan;
    }
    new_br_vlan = rtnl_link_bridge_get_port_vlan(new_link);
    //  TODO check if it can happen that new_br_vlan == null
    _link = nl->get_link(rtnl_link_get_ifindex(new_link), AF_UNSPEC);
  }

  if (_link == nullptr) {
    // XXX FIXME in case a vxlan has been deleted the vxlan_domain and
    // vxlan_dom_bitmap need an update, maybe this can be handled already from
    // the link_delete of the vxlan itself?
    LOG(WARNING) << __FUNCTION__
                 << ": could not get parent link of bridge interface. This "
                    "case needs further checks if everything got already "
                    "deleted.";
    return;
  }

  if (old_br_vlan == nullptr || new_br_vlan == nullptr) {
    VLOG(2) << __FUNCTION__ << ": interface attached without VLAN";
    return;
  }

  if (br_vlan_equal(old_br_vlan, new_br_vlan)) {
    // no vlan changed
    VLOG(2) << __FUNCTION__ << ": vlans did not change";
    return;
  }

  // sanity checks
  if (bridge == nullptr) { // XXX TODO solve this differently
    LOG(WARNING) << __FUNCTION__ << " cannot update interface without bridge";
    return;
  }
  char *type = rtnl_link_get_type(_link);
  if (type == nullptr) {
    VLOG(1) << __FUNCTION__ << ": no link type";
  }

  uint32_t pport_no = 0;
  std::deque<rtnl_link *> bridge_ports;

  pport_no = tap_man->get_port_id(rtnl_link_get_ifindex(_link));
  if (pport_no == 0) {
    LOG(ERROR) << __FUNCTION__ << ": invalid port "
               << static_cast<void *>(_link);
    return;
  }

  for (int k = 0; k < RTNL_LINK_BRIDGE_VLAN_BITMAP_LEN; k++) {
    int base_bit;
    uint32_t a = old_br_vlan->vlan_bitmap[k];
    uint32_t b = new_br_vlan->vlan_bitmap[k];
    uint32_t vlan_diff = a ^ b;

#if 0  // untagged change not yet implemented
    uint32_t c = old_br_vlan->untagged_bitmap[k];
    uint32_t d = new_br_vlan->untagged_bitmap[k];
    uint32_t untagged_diff = c ^ d;
#endif // 0

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

#if 0  // untagged change not yet implemented
       // clear untagged_diff bit
          untagged_diff &= ~((uint32_t)1 << (j - 1));
#endif // 0
        }

        if (new_br_vlan->vlan_bitmap[k] & 1 << (j - 1)) {
          // vlan added
          assert(pport_no);
          // normal vlan port
          sw->egress_bridge_port_vlan_add(pport_no, vid, egress_untagged);
          sw->ingress_port_vlan_add(pport_no, vid, new_br_vlan->pvid == vid);

        } else {
          // vlan removed

          sw->ingress_port_vlan_remove(pport_no, vid, old_br_vlan->pvid == vid);

          // delete all FM pointing to this group first
          sw->l2_addr_remove_all_in_vlan(pport_no, vid);
          sw->egress_bridge_port_vlan_remove(pport_no, vid);
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

void nl_bridge::add_neigh_to_fdb(rtnl_neigh *neigh) {
  assert(sw);
  assert(neigh);

  uint32_t port = tap_man->get_port_id(rtnl_neigh_get_ifindex(neigh));
  if (port == 0) {
    VLOG(1) << "ignoring neigh" << neigh << ": not a tap port";
    return;
  }

  nl_addr *mac = rtnl_neigh_get_lladdr(neigh);
  int vlan = rtnl_neigh_get_vlan(neigh);

  rofl::caddress_ll _mac((uint8_t *)nl_addr_get_binary_addr(mac),
                         nl_addr_get_len(mac));

  LOG(INFO) << __FUNCTION__ << ": add mac=" << _mac << " to bridge "
            << rtnl_link_get_name(bridge) << " on port=" << port
            << " vlan=" << (unsigned)vlan;
  sw->l2_addr_add(port, vlan, _mac, true);
}

void nl_bridge::remove_mac_from_fdb(rtnl_neigh *neigh) {
  assert(sw);
  nl_addr *addr = rtnl_neigh_get_lladdr(neigh);
  if (nl_addr_cmp(rtnl_link_get_addr(bridge), addr) == 0) {
    // ignore ll addr of bridge on slave
    return;
  }
  const uint32_t port = tap_man->get_port_id(rtnl_neigh_get_ifindex(neigh));

  rofl::caddress_ll mac((uint8_t *)nl_addr_get_binary_addr(addr),
                        nl_addr_get_len(addr));
  sw->l2_addr_remove(port, rtnl_neigh_get_vlan(neigh), mac);
}

} /* namespace basebox */
