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

	fm_driver.enable_port_pvid_ingress(rtl.get_devname(), rtl.get_pvid());
	uint32_t group = fm_driver.enable_port_pvid_egress(rtl.get_devname(), rtl.get_pvid());
	assert(group);
	if (rofl::openflow::OFPG_MAX == group) {
		// fixme disable pvid ingress
		rofcore::logging::error << __PRETTY_FUNCTION__ << " failed to set pvid egress " << std::endl;
		return;
	}
	l2_domain.push_back(group);

	// todo check if vid is okay as an id as well
	group = fm_driver.enable_group_l2_multicast(rtl.get_pvid(), rtl.get_pvid(), l2_domain, 1 != l2_domain.size());
	// enable arp flooding as well

	if (1 == l2_domain.size()) {
		fm_driver.enable_policy_arp(rtl.get_pvid(), group);
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
