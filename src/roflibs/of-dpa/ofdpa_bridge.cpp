#include "ofdpa_bridge.hpp"

#include <cassert>
#include <map>

#include <rofl/common/openflow/cofport.h>

namespace basebox {

ofdpa_bridge::ofdpa_bridge(ofdpa_fm_driver &fm_driver) :
		interface_id(0),
		fm_driver(fm_driver)
{
}

ofdpa_bridge::ofdpa_bridge(const unsigned int ifindex, ofdpa_fm_driver &fm_driver) :
		interface_id(ifindex),
		fm_driver(fm_driver)
{
	assert(interface_id);
}

ofdpa_bridge::~ofdpa_bridge()
{
}

void
ofdpa_bridge::set_bridge_interface(const unsigned int id)
{
	assert(id);
	assert(0 == interface_id);
	this->interface_id = id;

	// todo vid should not be static, and maybe moved to a better location
	// XXX enable again if decided what the default fwding should be
	// fm_driver.enable_bridging_dlf_vlan(1, 0, true); // enable pkt-in for vid 1
}

void
ofdpa_bridge::add_interface(const uint32_t of_port_no)
{
	assert(interface_id);

	fm_driver.enable_port_pvid_ingress(1, of_port_no);
	uint32_t group = fm_driver.enable_port_pvid_egress(1, of_port_no);
	assert(group);
	l2_domain.push_back(group);
	fm_driver.enable_group_l2_multicast(1, 1, l2_domain, 1 != l2_domain.size());
}

void
ofdpa_bridge::delete_interface(const uint32_t of_port_no)
{
	assert(interface_id);
	// fixme update L2 Multicast Group
}

} /* namespace basebox */
