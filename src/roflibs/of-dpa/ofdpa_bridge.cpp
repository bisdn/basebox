#include "ofdpa_bridge.hpp"

#include "roflibs/netlink/clogging.hpp"

#include <cassert>
#include <map>

#include <rofl/common/openflow/cofport.h>


namespace basebox {

ofdpa_bridge::ofdpa_bridge(ofdpa_fm_driver &fm_driver) :
		fm_driver(fm_driver)
{
}

ofdpa_bridge::~ofdpa_bridge()
{
}

void
ofdpa_bridge::set_bridge_interface(const rofcore::crtlink& rtl)
{
	if (AF_BRIDGE != rtl.get_family() || 0 != rtl.get_master()) {
		rofcore::logging::error << __PRETTY_FUNCTION__ << " not a bridge master: " << rtl << std::endl;
		return;
	}

	this->bridge = rtl;
}

static int find_next_bit(int i, uint32_t x)
{
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

void
ofdpa_bridge::add_interface(const rofcore::crtlink& rtl)
{
	// sanity checks
	if (0 == bridge.get_ifindex()) {
		rofcore::logging::error << __PRETTY_FUNCTION__ << " cannot attach interface without bridge: " << rtl << std::endl;
		return;
	}
	if (AF_BRIDGE != rtl.get_family()) {
		rofcore::logging::error << __PRETTY_FUNCTION__ << rtl << " is not a bridge interface " << std::endl;
		return;
	}
	if (bridge.get_ifindex() != rtl.get_master()) {
		rofcore::logging::error << __PRETTY_FUNCTION__ << rtl << " is not a slave of this bridge interface " << std::endl;
		return;
	}

	const struct rtnl_link_bridge_vlan* br_vlan = rtl.get_br_vlan();

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
				if (br_vlan->untagged_bitmap[k] & 1 << (j-1)) {
					egress_untagged = true;
				}

				uint32_t group = fm_driver.enable_port_vid_egress(rtl.get_devname(), vid, egress_untagged);
				assert(group && "invalid group identifier");
				if (rofl::openflow::OFPG_MAX == group) {
					rofcore::logging::error << __PRETTY_FUNCTION__ << " failed to set vid on egress " << std::endl;
					i = j;
					continue;
				}
				l2_domain[vid].push_back(group);

				if (br_vlan->pvid == vid) {
					fm_driver.enable_port_pvid_ingress(rtl.get_devname(), vid);
				} else {
					fm_driver.enable_port_vid_ingress(rtl.get_devname(), vid);
				}

				// todo check if vid is okay as an id as well
				group = fm_driver.enable_group_l2_multicast(vid, vid, l2_domain[vid], 1 != l2_domain[vid].size());
				// enable arp flooding as well

				if (1 == l2_domain[vid].size()) { // todo maybe unnecessary
					fm_driver.enable_policy_arp(vid, group);
				}

				i = j;
			} else {
				done = 1;
			}
		}
	}
}

void
ofdpa_bridge::delete_interface(const rofcore::crtlink& rtl)
{
	// fixme update L2 Multicast Group
}


void
ofdpa_bridge::add_mac_to_fdb(const rofl::cmacaddr& mac, const uint32_t of_port_no, bool permanent)
{
	fm_driver.add_bridging_unicast_vlan(mac, 1, of_port_no, permanent);
}

void ofdpa_bridge::remove_mac_from_fdb(const rofl::cmacaddr& mac,
		const uint32_t of_port_no)
{
	fm_driver.remove_bridging_unicast_vlan(mac, 1, of_port_no);
}

} /* namespace basebox */
