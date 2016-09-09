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

#include "roflibs/netlink/sai.hpp"
#include "ofdpa_bridge.hpp"

namespace rofcore {

ofdpa_bridge::ofdpa_bridge(switch_interface *sw)
    : bridge(nullptr), sw(sw), ingress_vlan_filtered(true),
      egress_vlan_filtered(true) {}

ofdpa_bridge::~ofdpa_bridge() {
  if (bridge)
    rtnl_link_put(bridge);
}

void ofdpa_bridge::set_bridge_interface(rtnl_link *bridge) {
  assert(bridge);

  this->bridge = bridge;
  nl_object_get(OBJ_CAST(bridge));

  sw->subscribe_to(switch_interface::SWIF_ARP);
}

static bool br_vlan_cmp(const rtnl_link_bridge_vlan *lhs,
                        const rtnl_link_bridge_vlan *rhs) {
  assert(lhs);
  assert(rhs);
  if (lhs->pvid != rhs->pvid) {
    return false;
  }

  if (memcmp(lhs->vlan_bitmap, rhs->vlan_bitmap, sizeof(rhs->vlan_bitmap))) {
    return false;
  }

  if (memcmp(lhs->untagged_bitmap, rhs->untagged_bitmap,
             sizeof(rhs->untagged_bitmap))) {
    return false;
  }

  return true;
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

void ofdpa_bridge::add_interface(uint32_t port, rtnl_link *link) {
  assert(sw);
  assert(rtnl_link_get_family(link) == AF_BRIDGE);

  // sanity checks
  if (bridge == nullptr) {
    LOG(WARNING) << __FUNCTION__ << " cannot update interface without bridge";
    return;
  }

  if (rtnl_link_get_ifindex(bridge) != rtnl_link_get_master(link)) {
    LOG(INFO) << __FUNCTION__ << ": link " << rtnl_link_get_name(link)
              << " is no slave of " << rtnl_link_get_name(bridge);
    return;
  }

  const struct rtnl_link_bridge_vlan *br_vlan =
      rtnl_link_bridge_get_port_vlan(link);

  if (not ingress_vlan_filtered) {
    sw->ingress_port_vlan_accept_all(port);
    if (br_vlan->pvid) {
      VLOG(2) << __FUNCTION__ << " port=" << port << " pvid=" << br_vlan->pvid;
      sw->ingress_port_vlan_add(port, br_vlan->pvid, true);
    }
  }

  if (not egress_vlan_filtered) {
    // egress
    // TODO: as soon as pop_vlan on egress works update this
    sw->egress_port_vlan_accept_all(port);
  }

  if (not ingress_vlan_filtered && not egress_vlan_filtered) {
    return;
  }

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

void ofdpa_bridge::update_interface(uint32_t port, rtnl_link *old_link,
                                    rtnl_link *new_link) {
  assert(rtnl_link_get_family(new_link) == AF_BRIDGE);
  assert(rtnl_link_get_family(old_link) == AF_BRIDGE);

  // sanity checks
  if (bridge == nullptr) {
    LOG(WARNING) << __FUNCTION__ << " cannot update interface without bridge";
    return;
  }

  if (rtnl_link_get_ifindex(bridge) != rtnl_link_get_master(new_link)) {
    LOG(INFO) << __FUNCTION__ << ": link " << rtnl_link_get_name(new_link)
              << " is no slave of " << rtnl_link_get_name(bridge);
    return;
  }

  if (0 != strcmp(rtnl_link_get_name(old_link), rtnl_link_get_name(new_link))) {
    LOG(INFO) << __FUNCTION__ << ": ignore rename of link "
              << rtnl_link_get_name(old_link) << " to "
              << rtnl_link_get_name(new_link);
    // TODO this has to be handled differently
    return;
  }

  struct rtnl_link_bridge_vlan *old_br_vlan =
      rtnl_link_bridge_get_port_vlan(old_link);
  assert(old_br_vlan);
  struct rtnl_link_bridge_vlan *new_br_vlan =
      rtnl_link_bridge_get_port_vlan(new_link);
  assert(new_br_vlan);

  if (not ingress_vlan_filtered) {
    if (old_br_vlan->pvid != new_br_vlan->pvid) {
      if (new_br_vlan->pvid) {
        VLOG(2) << __FUNCTION__ << ": pvid changed of link "
                << rtnl_link_get_name(new_link) << " port=" << port
                << " old pvid=" << old_br_vlan->pvid
                << " new pvid=" << new_br_vlan->pvid;
        sw->ingress_port_vlan_add(port, new_br_vlan->pvid, true);
        if (old_br_vlan->pvid) {
          // just remove the vid and not the rewrite for untagged packets, hence
          // pvid=false is fine here
          // TODO maybe update the interface, because this might be missleading
          sw->ingress_port_vlan_remove(port, old_br_vlan->pvid, false);
        }
      } else {
        // vlan removed
        sw->ingress_port_vlan_remove(port, old_br_vlan->pvid, true);
      }
    }
  }

  // handle VLANs
  if (not ingress_vlan_filtered && not egress_vlan_filtered) {
    return;
  }

  if (!br_vlan_cmp(old_br_vlan, new_br_vlan)) {
    update_vlans(port, rtnl_link_get_name(old_link), old_br_vlan, new_br_vlan);
  }
}

void ofdpa_bridge::delete_interface(uint32_t port, rtnl_link *link) {
  assert(sw);
  assert(rtnl_link_get_family(link) == AF_BRIDGE);

  // sanity checks
  if (bridge == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": cannot attach interface without bridge";
    return;
  }
  if (rtnl_link_get_ifindex(bridge) != rtnl_link_get_master(link)) {
    LOG(INFO) << __FUNCTION__ << ": link " << rtnl_link_get_name(link)
              << " is no slave of " << rtnl_link_get_name(bridge);
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

  const struct rtnl_link_bridge_vlan *br_vlan =
      rtnl_link_bridge_get_port_vlan(link);
  struct rtnl_link_bridge_vlan br_vlan_empty;
  memset(&br_vlan_empty, 0, sizeof(struct rtnl_link_bridge_vlan));

  if (!br_vlan_cmp(br_vlan, &br_vlan_empty)) {
    // vlan updated
    update_vlans(port, rtnl_link_get_name(link), br_vlan, &br_vlan_empty);
  }
}

void ofdpa_bridge::add_mac_to_fdb(const uint32_t port, uint16_t vlan,
                                  nl_addr *mac) {
  assert(sw);
  assert(mac);

  rofl::caddress_ll _mac((uint8_t *)nl_addr_get_binary_addr(mac),
                         nl_addr_get_len(mac));

  LOG(INFO) << __FUNCTION__ << ": add mac=" << _mac << " to bridge "
            << rtnl_link_get_name(bridge) << " on port=" << port
            << " vlan=" << (unsigned)vlan;
  sw->l2_addr_add(port, vlan, _mac, egress_vlan_filtered);
}

void ofdpa_bridge::remove_mac_from_fdb(const uint32_t port, rtnl_neigh *neigh) {
  assert(sw);
  nl_addr *addr = rtnl_neigh_get_lladdr(neigh);
  if (nl_addr_cmp(rtnl_link_get_addr(bridge), addr) == 0) {
    // ignore ll addr of bridge on slave
    return;
  }
  rofl::caddress_ll mac((uint8_t *)nl_addr_get_binary_addr(addr),
                        nl_addr_get_len(addr));
  sw->l2_addr_remove(port, rtnl_neigh_get_vlan(neigh), mac);
}

} /* namespace basebox */
