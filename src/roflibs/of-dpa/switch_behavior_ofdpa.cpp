#include "switch_behavior_ofdpa.hpp"
#include "roflibs/netlink/clogging.hpp"

#include <map>
#include <rofl/common/openflow/cofport.h>
#include <rofl/common/openflow/cofports.h>

namespace basebox {

switch_behavior_ofdpa::switch_behavior_ofdpa(const rofl::cdptid& dptid) :
		switch_behavior(dptid)
{
	rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);

	rofcore::logging::debug << "[switch_behavior_ofdpa][switch_behavior_ofdpa] dpt: " << std::endl << dpt;

	init_ports();
}

switch_behavior_ofdpa::~switch_behavior_ofdpa()
{
	//todo need this?
//	try {
//		if (STATE_ATTACHED == state) {
//			handle_dpt_close();
//		}
//	} catch (rofl::eRofDptNotFound& e) {}

	clear_tap_devs(dptid);
}

void
switch_behavior_ofdpa::init_ports()
{
	using rofl::openflow::cofport;
	using std::map;
	using std::set;

	/* init 1:1 port mapping */
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);

		const map<uint32_t, cofport*> &ports = dpt.get_ports().get_ports();

		for (map<uint32_t, cofport*>::const_iterator iter = ports.begin(); iter != ports.end(); ++iter) {
			const cofport* port = iter->second;
			if (not has_tap_dev(dptid, port->get_name())) {
				add_tap_dev(dptid, port->get_name(), default_pvid, port->get_hwaddr());
			}
		}

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[switch_behavior_ofdpa][" << __FUNCTION__<< "] ERROR: eRofDptNotFound" << std::endl;
	}
}


} /* namespace basebox */
