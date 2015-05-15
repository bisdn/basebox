#ifndef SRC_ROFLIBS_OF_DPA_SWITCH_BEHAVIOR_OFDPA_HPP_
#define SRC_ROFLIBS_OF_DPA_SWITCH_BEHAVIOR_OFDPA_HPP_

#include <baseboxd/switch_behavior.hpp>
#include "roflibs/netlink/cnetdev.hpp"
#include "roflibs/netlink/cnetlink.hpp"
#include "roflibs/netlink/ctapdev.hpp"
#include "roflibs/netlink/ctundev.hpp"

namespace basebox {

class switch_behavior_ofdpa :
		public switch_behavior,
		public rofcore::cnetdev_owner,
		public rofcore::cnetlink_common_observer {
public:
	switch_behavior_ofdpa(rofl::cdptid const& dptid);

	virtual
	~switch_behavior_ofdpa();

	virtual int
	get_switch_type()
	{
		return 1;
	}

private:

	std::map<rofl::cdptid, std::map<std::string, rofcore::ctapdev*> > devs;
	static const uint16_t							default_pvid = 0;

	void
	init_ports();


	// todo code duplication: align with cethcore
	/**
	 *
	 */
	void
	clear_tap_devs(const rofl::cdptid& dpid) {
		while (not devs[dpid].empty()) {
			std::map<std::string, rofcore::ctapdev*>::iterator it = devs[dpid].begin();
			try {
				// todo temp. disabled:
				// hook_port_down(rofl::crofdpt::get_dpt(it->second->get_dptid()), it->second->get_devname());
			} catch (rofl::eRofDptNotFound& e) {}
			drop_tap_dev(dpid, it->first);
		}
	};

	/**
	 *
	 */
	rofcore::ctapdev&
	add_tap_dev(const rofl::cdptid& dpid, const std::string& devname, uint16_t pvid, const rofl::caddress_ll& hwaddr) {
		if (devs[dpid].find(devname) != devs[dpid].end()) {
			delete devs[dpid][devname];
		}
		devs[dpid][devname] = new rofcore::ctapdev(this, dpid, devname, pvid, hwaddr);
		return *(devs[dpid][devname]);
	};

	/**
	 *
	 */
	rofcore::ctapdev&
	set_tap_dev(const rofl::cdptid& dpid, const std::string& devname, uint16_t pvid, const rofl::caddress_ll& hwaddr) {
		if (devs[dpid].find(devname) == devs[dpid].end()) {
			devs[dpid][devname] = new rofcore::ctapdev(this, dpid, devname, pvid, hwaddr);
		}
		return *(devs[dpid][devname]);
	};

	/**
	 *
	 */
	rofcore::ctapdev&
	set_tap_dev(const rofl::cdptid& dpid, const std::string& devname) {
		if (devs[dpid].find(devname) == devs[dpid].end()) {
			throw rofcore::eTunDevNotFound("cbasebox::set_tap_dev() devname not found");
		}
		return *(devs[dpid][devname]);
	};

	/**
	 *
	 */
	rofcore::ctapdev&
	set_tap_dev(const rofl::cdptid& dpid, const rofl::caddress_ll& hwaddr) {
		std::map<std::string, rofcore::ctapdev*>::iterator it;
		if ((it = find_if(devs[dpid].begin(), devs[dpid].end(),
				rofcore::ctapdev::ctapdev_find_by_hwaddr(dpid, hwaddr))) == devs[dpid].end()) {
			throw rofcore::eTunDevNotFound("cbasebox::set_tap_dev() hwaddr not found");
		}
		return *(it->second);
	};

	/**
	 *
	 */
	const rofcore::ctapdev&
	get_tap_dev(const rofl::cdptid& dpid, const std::string& devname) const {
		if (devs.find(dpid) == devs.end()) {
			throw rofcore::eTunDevNotFound("cbasebox::get_tap_dev() dpid not found");
		}
		if (devs.at(dpid).find(devname) == devs.at(dpid).end()) {
			throw rofcore::eTunDevNotFound("cbasebox::get_tap_dev() devname not found");
		}
		return *(devs.at(dpid).at(devname));
	};

	/**
	 *
	 */
	const rofcore::ctapdev&
	get_tap_dev(const rofl::cdptid& dpid, const rofl::caddress_ll& hwaddr) const {
		if (devs.find(dpid) == devs.end()) {
			throw rofcore::eTunDevNotFound("cbasebox::get_tap_dev() dpid not found");
		}
		std::map<std::string, rofcore::ctapdev*>::const_iterator it;
		if ((it = find_if(devs.at(dpid).begin(), devs.at(dpid).end(),
				rofcore::ctapdev::ctapdev_find_by_hwaddr(dpid, hwaddr))) == devs.at(dpid).end()) {
			throw rofcore::eTunDevNotFound("cbasebox::get_tap_dev() hwaddr not found");
		}
		return *(it->second);
	};

	/**
	 *
	 */
	void
	drop_tap_dev(const rofl::cdptid& dpid, const std::string& devname) {
		if (devs[dpid].find(devname) == devs[dpid].end()) {
			return;
		}
		delete devs[dpid][devname];
		devs[dpid].erase(devname);
	};

	/**
	 *
	 */
	void
	drop_tap_dev(const rofl::cdptid& dpid, const rofl::caddress_ll& hwaddr) {
		std::map<std::string, rofcore::ctapdev*>::const_iterator it;
		if ((it = find_if(devs[dpid].begin(), devs[dpid].end(),
				rofcore::ctapdev::ctapdev_find_by_hwaddr(dpid, hwaddr))) == devs[dpid].end()) {
			return;
		}
		delete it->second;
		devs[dpid].erase(it->first);
	};

	/**
	 *
	 */
	bool
	has_tap_dev(const rofl::cdptid& dpid, const std::string& devname) const {
		if (devs.find(dpid) == devs.end()) {
			return false;
		}
		return (not (devs.at(dpid).find(devname) == devs.at(dpid).end()));
	};

	/**
	 *
	 */
	bool
	has_tap_dev(const rofl::cdptid& dpid, const rofl::caddress_ll& hwaddr) const {
		if (devs.find(dpid) == devs.end()) {
			return false;
		}
		std::map<std::string, rofcore::ctapdev*>::const_iterator it;
		if ((it = find_if(devs.at(dpid).begin(), devs.at(dpid).end(),
				rofcore::ctapdev::ctapdev_find_by_hwaddr(dpid, hwaddr))) == devs.at(dpid).end()) {
			return false;
		}
		return true;
	};

};

}
/* namespace basebox */

#endif /* SRC_ROFLIBS_OF_DPA_SWITCH_BEHAVIOR_OFDPA_HPP_ */
