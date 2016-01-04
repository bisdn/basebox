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
	delete_interface(const rofcore::crtlink& rtl);

	void
	add_mac_to_fdb(const rofl::cmacaddr &mac, const uint32_t of_port_no, bool permanent = false);

	void
	remove_mac_from_fdb(const rofl::cmacaddr &mac, const uint32_t of_port_no);

private:
	rofcore::crtlink bridge;
	ofdpa_fm_driver &fm_driver; // todo use shared pointer?

	std::list<uint32_t> l2_domain;
};

} /* namespace basebox */

#endif /* SRC_ROFLIBS_OF_DPA_OFDPA_BRIDGE_HPP_ */
