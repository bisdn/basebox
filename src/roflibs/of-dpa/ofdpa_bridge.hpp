#ifndef SRC_ROFLIBS_OF_DPA_OFDPA_BRIDGE_HPP_
#define SRC_ROFLIBS_OF_DPA_OFDPA_BRIDGE_HPP_

#include <cstdint>
#include <list>

#include "ofdpa_fm_driver.hpp"
#include "roflibs/netlink/crtlink.hpp"

namespace basebox {

class ofdpa_bridge {
public:
	ofdpa_bridge(ofdpa_fm_driver &fm_driver);

	virtual
	~ofdpa_bridge();

	void
	set_bridge_interface(const rofcore::crtlink& rtl);

	const rofcore::crtlink&
	get_bridge_interface() const {
		return bridge;
	}

	bool
	has_bridge_interface() const {
		return 0 != bridge.get_ifindex();
	}

	void
	add_interface(const rofcore::crtlink& rtl);

	void
	update_interface(const rofcore::crtlink& oldlink, const rofcore::crtlink& newlink);

	void
	delete_interface(const rofcore::crtlink& rtl);

	void
	add_mac_to_fdb(const uint32_t of_port_no, const uint16_t vlan, const rofl::cmacaddr &mac, bool permanent = false);

	void
	remove_mac_from_fdb(const uint32_t of_port_no, uint16_t vid,
			const rofl::cmacaddr& mac);

private:
	void
	update_vlans(const std::string &devname,
			const rtnl_link_bridge_vlan *old_br_vlan,
			const rtnl_link_bridge_vlan *new_br_vlan);

	rofcore::crtlink bridge;
	ofdpa_fm_driver &fm_driver; // todo use shared pointer?

	std::map<uint16_t, std::list<uint32_t> > l2_domain;
};

} /* namespace basebox */

#endif /* SRC_ROFLIBS_OF_DPA_OFDPA_BRIDGE_HPP_ */
