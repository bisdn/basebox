/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cassert>
#include <cstring>
#include <map>

#include <glog/logging.h>
#include <rofl/common/openflow/cofport.h>

#include "roflibs/netlink/sai.hpp"
#include "ofdpa_bridge.hpp"

namespace rofcore {

ofdpa_bridge::ofdpa_bridge(switch_interface *sw)
    : sw(sw), ingress_vlan_filtered(true), egress_vlan_filtered(false) {}

ofdpa_bridge::~ofdpa_bridge() {}

void ofdpa_bridge::set_bridge_interface(const crtlink &rtl) {
  assert(sw);
  if (AF_BRIDGE != rtl.get_family() || 0 != rtl.get_master()) {
    LOG(ERROR) << __FUNCTION__ << " not a bridge master: " << rtl << std::endl;
    return;
  }

  this->bridge = rtl;

  sw->subscribe_to(switch_interface::SWIF_ARP);
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

void ofdpa_bridge::add_interface(uint32_t port, const crtlink &rtl) {
  assert(sw);

  // sanity checks
  if (0 == bridge.get_ifindex()) {
    LOG(ERROR) << __FUNCTION__
               << " cannot attach interface without bridge: " << rtl
               << std::endl;
    return;
  }
  if (AF_BRIDGE != rtl.get_family()) {
    LOG(ERROR) << __FUNCTION__ << rtl << " is not a bridge interface "
               << std::endl;
    return;
  }
  if (bridge.get_ifindex() != rtl.get_master()) {
    LOG(ERROR) << __FUNCTION__ << rtl
               << " is not a slave of this bridge interface " << std::endl;
    return;
  }

  if (not ingress_vlan_filtered) {
    // ingress
    sw->ingress_port_vlan_accept_all(port);
  }

  if (not egress_vlan_filtered) {
    // egress
    sw->egress_port_vlan_accept_all(port);
  }

  if (not ingress_vlan_filtered && not egress_vlan_filtered) {
    return;
  }

  const struct rtnl_link_bridge_vlan *br_vlan = rtl.get_br_vlan();

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

        if (egress_vlan_filtered) {
          sw->egress_port_vlan_add(port, vid, egress_untagged);
        }

        if (ingress_vlan_filtered) {
          sw->ingress_port_vlan_add(port, vid, br_vlan->pvid == vid);
        }

        i = j;
      } else {
        done = 1;
      }
    }
  }
}

void ofdpa_bridge::update_vlans(uint32_t port, const std::string &devname,
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
            sw->egress_port_vlan_add(port, vid, egress_untagged);
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
            sw->egress_port_vlan_remove(port, vid, egress_untagged);
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

void ofdpa_bridge::update_interface(uint32_t port, const crtlink &oldlink,
                                    const crtlink &newlink) {
  // sanity checks
  if (0 == bridge.get_ifindex()) {
    LOG(ERROR) << __FUNCTION__ << " cannot update interface without bridge"
               << std::endl;
    return;
  }
  if (AF_BRIDGE != newlink.get_family()) {
    LOG(ERROR) << __FUNCTION__ << newlink << " is not a bridge interface"
               << std::endl;
    return;
  }
  // if (AF_BRIDGE != oldlink.get_family()) {
  //   LOG(ERROR) << __FUNCTION__ << oldlink << " is
  //                     not a bridge interface" << std::endl;
  //   return;
  // }
  if (bridge.get_ifindex() != newlink.get_master()) {
    LOG(ERROR) << __FUNCTION__ << newlink
               << " is not a slave of this bridge interface" << std::endl;
    return;
  }
  if (bridge.get_ifindex() != oldlink.get_master()) {
    LOG(ERROR) << __FUNCTION__ << newlink
               << " is not a slave of this bridge interface" << std::endl;
    return;
  }

  if (newlink.get_devname().compare(oldlink.get_devname())) {
    LOG(INFO) << __FUNCTION__ << " interface rename currently ignored "
              << std::endl;
    // FIXME this has to be handled differently
    return;
  }

  // handle VLANs
  if (not ingress_vlan_filtered && not egress_vlan_filtered) {
    return;
  }

  if (not crtlink::are_br_vlan_equal(newlink.get_br_vlan(),
                                     oldlink.get_br_vlan())) {
    // vlan updated
    update_vlans(port, oldlink.get_devname(), oldlink.get_br_vlan(),
                 newlink.get_br_vlan());
  }
}

void ofdpa_bridge::delete_interface(uint32_t port, const crtlink &rtl) {
  assert(sw);
  // sanity checks
  if (0 == bridge.get_ifindex()) {
    LOG(ERROR) << __FUNCTION__
               << " cannot attach interface without bridge: " << rtl
               << std::endl;
    return;
  }
  if (AF_BRIDGE != rtl.get_family()) {
    LOG(ERROR) << __FUNCTION__ << rtl << " is not a bridge interface "
               << std::endl;
    return;
  }
  if (bridge.get_ifindex() != rtl.get_master()) {
    LOG(ERROR) << __FUNCTION__ << rtl
               << " is not a slave of this bridge interface " << std::endl;
    return;
  }

  if (not ingress_vlan_filtered) {
    // ingress
    sw->ingress_port_vlan_drop_accept_all(port);
  }

  if (not egress_vlan_filtered) {
    // egress
    // delete all FM pointing to this group first
    sw->l2_addr_remove_all_in_vlan(port, -1);
    sw->egress_port_vlan_drop_accept_all(port);
  }

  if (not ingress_vlan_filtered && not egress_vlan_filtered) {
    return;
  }

  const struct rtnl_link_bridge_vlan *br_vlan = rtl.get_br_vlan();
  struct rtnl_link_bridge_vlan br_vlan_empty;
  memset(&br_vlan_empty, 0, sizeof(struct rtnl_link_bridge_vlan));

  if (not crtlink::are_br_vlan_equal(br_vlan, &br_vlan_empty)) {
    // vlan updated
    update_vlans(port, rtl.get_devname(), br_vlan, &br_vlan_empty);
  }
}

void ofdpa_bridge::add_mac_to_fdb(const uint32_t port, const uint16_t vid,
                                  const rofl::cmacaddr &mac) {
  assert(sw);
  sw->l2_addr_add(port, vid, mac);
}

void ofdpa_bridge::remove_mac_from_fdb(const uint32_t port, uint16_t vid,
                                       const rofl::cmacaddr &mac) {
  assert(sw);
  sw->l2_addr_remove(port, vid, mac);
}

} /* namespace basebox */
