#ifndef SRC_ROFLIBS_OF_DPA_OFDPA_FM_DRIVER_HPP_
#define SRC_ROFLIBS_OF_DPA_OFDPA_FM_DRIVER_HPP_

#include <rofl/common/cdptid.h>
#include <rofl/common/caddress.h>
#include <list>

namespace basebox
{

class ofdpa_fm_driver
{
public:
	ofdpa_fm_driver(const rofl::cdptid& dptid);
	virtual ~ofdpa_fm_driver();

	void
	enable_port_pvid_ingress(uint16_t vid, uint32_t port_no);

	void
	enable_port_vid_ingress(uint16_t vid, uint32_t port_no);

	// equals l2 interaface group, so maybe rename this
	uint32_t
	enable_port_pvid_egress(uint16_t vid, uint32_t port_no);

	uint32_t
	enable_group_l2_multicast(uint16_t vid, uint16_t id,
			const std::list<uint32_t> &l2_interfaces, bool update = false);

	void
	enable_bridging_dlf_vlan(uint16_t vid, uint32_t group_id, bool do_pkt_in);

	void
	enable_policy_arp(uint16_t vid, uint32_t group_id, bool update = false);

	void
	add_bridging_unicast_vlan(const rofl::cmacaddr& mac, uint16_t vid,
			uint32_t port_no);

private:
	const uint16_t default_idle_timeout;

	rofl::cdptid dptid;
};

} /* namespace basebox */

#endif /* SRC_ROFLIBS_OF_DPA_OFDPA_FM_DRIVER_HPP_ */
