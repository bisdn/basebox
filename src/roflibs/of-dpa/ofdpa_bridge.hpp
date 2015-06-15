#ifndef SRC_ROFLIBS_OF_DPA_OFDPA_BRIDGE_HPP_
#define SRC_ROFLIBS_OF_DPA_OFDPA_BRIDGE_HPP_

#include <cstdint>
#include <list>

#include "ofdpa_fm_driver.hpp"

namespace basebox {

class ofdpa_bridge {
public:
	ofdpa_bridge(ofdpa_fm_driver &fm_driver);

	ofdpa_bridge(const unsigned int ifindex, ofdpa_fm_driver &fm_driver);

	virtual
	~ofdpa_bridge();

	void
	set_bridge_interface(const unsigned int id);

	unsigned int
	get_bridge_interface() const {
		return interface_id;
	}

	bool
	has_bridge_interface() const {
		return 0 != interface_id;
	}

	void
	add_interface(const uint32_t of_port_no);

	void
	delete_interface(const uint32_t of_port_no);

	void
	add_mac_to_fdb(const rofl::cmacaddr &mac, const uint32_t of_port_no);

private:
	unsigned int interface_id;
	ofdpa_fm_driver &fm_driver; // todo use shared pointer?

	std::list<uint32_t> l2_domain;
};

} /* namespace basebox */

#endif /* SRC_ROFLIBS_OF_DPA_OFDPA_BRIDGE_HPP_ */
