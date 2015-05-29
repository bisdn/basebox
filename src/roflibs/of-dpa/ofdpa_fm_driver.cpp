#include "ofdpa_fm_driver.hpp"
#include "ofdpa_datatypes.h"

#include <rofl/common/crofdpt.h>
#include <rofl/common/openflow/cofflowmod.h>
#include <rofl/common/protocols/fvlanframe.h>

namespace basebox {

ofdpa_fm_driver::ofdpa_fm_driver(const rofl::cdptid& dptid) :
		dptid(dptid)
{
}

ofdpa_fm_driver::~ofdpa_fm_driver()
{
}

void
ofdpa_fm_driver::enable_port_pvid_ingress(uint16_t vid, uint32_t port_no)
{
	// XXX This contradicts the OF-DPA ASS (p. 64) "The VLAN_VID value cannot be used in a VLAN Filtering rule."
	// XXX Send ticket to BCM to clarify
	enable_port_vid_ingress(vid, port_no);

	// todo check vid

	rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);

	rofl::openflow::cofflowmod fm(dpt.get_version_negotiated());

	fm.set_command(rofl::openflow::OFPFC_ADD);
	fm.set_table_id(OFDPA_FLOW_TABLE_ID_VLAN);

	// todo make configurable?
	fm.set_idle_timeout(0);
	fm.set_hard_timeout(0);
	fm.set_priority(2);
	fm.set_cookie(0);

	fm.set_match().set_in_port(port_no);
	fm.set_match().set_vlan_vid(0, 0x1fff);

	fm.set_instructions().set_inst_apply_actions().set_actions().
			add_action_set_field(rofl::cindex(0)).set_oxm(rofl::openflow::coxmatch_ofb_vlan_vid(vid | rofl::openflow::OFPVID_PRESENT));

	fm.set_instructions().set_inst_goto_table().set_table_id(OFDPA_FLOW_TABLE_ID_TERMINATION_MAC);

	// todo set vrf?

	std::cout << "write flow-mod:" << std::endl << fm;

	dpt.send_flow_mod_message(rofl::cauxid(0), fm);
}

void
ofdpa_fm_driver::enable_port_vid_ingress(uint16_t vid, uint32_t port_no)
{
	rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);

	rofl::openflow::cofflowmod fm(dpt.get_version_negotiated());

	fm.set_command(rofl::openflow::OFPFC_ADD); // fixme what happens if this is added two times?
	fm.set_table_id(OFDPA_FLOW_TABLE_ID_VLAN);

	// todo make configurable?
	fm.set_idle_timeout(0);
	fm.set_hard_timeout(0);
	fm.set_priority(3);
	fm.set_cookie(0);

	fm.set_match().set_in_port(port_no);
	fm.set_match().set_vlan_vid(vid | rofl::openflow::OFPVID_PRESENT, 0x1fff);

	fm.set_instructions().set_inst_goto_table().set_table_id(OFDPA_FLOW_TABLE_ID_TERMINATION_MAC);

	// todo set vrf?

	std::cout << "write flow-mod:" << std::endl << fm;

	dpt.send_flow_mod_message(rofl::cauxid(0), fm);
}

uint32_t
ofdpa_fm_driver::enable_port_pvid_egress(uint16_t vid, uint32_t port_no)
{
	// todo check params
	uint32_t group_id = (0x0fff & vid) << 16 | (0xffff & port_no);

	rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);
	rofl::openflow::cofgroupmod gm(dpt.get_version_negotiated());

	gm.set_command(rofl::openflow::OFPGC_ADD);
	gm.set_type(rofl::openflow::OFPGT_INDIRECT);
	gm.set_group_id(group_id);

	gm.set_buckets().add_bucket(0).set_actions().
			add_action_pop_vlan(rofl::cindex(0));
	gm.set_buckets().set_bucket(0).set_actions().
			add_action_output(rofl::cindex(1)).set_port_no(port_no);

	std::cout << "send group mod:" << std::endl << gm;

	dpt.send_group_mod_message(rofl::cauxid(0), gm);

	return group_id;
}

void
ofdpa_fm_driver::enable_group_l2_multicast(uint16_t vid, uint16_t id, const std::list<uint32_t>& l2_interfaces, bool update)
{
	rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);
	rofl::openflow::cofgroupmod gm(dpt.get_version_negotiated());

	gm.set_command(update ? rofl::openflow::OFPGC_MODIFY : rofl::openflow::OFPGC_ADD);
	gm.set_type(rofl::openflow::OFPGT_ALL);
	gm.set_group_id(3 << 28 | (0x0fff & vid) << 16 | (0xffff & id));

	rofl::cindex indx(0);
	gm.set_buckets().add_bucket(0).set_actions().
			add_action_pop_vlan(indx);

	for (const uint32_t &i : l2_interfaces) {
		std::cout << i << ' ';
		gm.set_buckets().add_bucket(0).set_actions().
					add_action_group(++indx).set_group_id(i);
	}
	std::cout << std::endl;

	std::cout << "send group mod:" << std::endl << gm;

	dpt.send_group_mod_message(rofl::cauxid(0), gm);
}

void
ofdpa_fm_driver::enable_bridging_dlf_vlan(uint16_t vid, uint32_t group_id, bool do_pkt_in)
{
	rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);

	rofl::openflow::cofflowmod fm(dpt.get_version_negotiated());
	fm.set_table_id(OFDPA_FLOW_TABLE_ID_BRIDGING);

	// todo make configurable?
	fm.set_idle_timeout(0);
	fm.set_hard_timeout(0);
	fm.set_priority(2);
	fm.set_cookie(0);

	//fm.set_match().set_eth_dst(rofl::cmacaddr("00:00:00:00:00:01"), rofl::cmacaddr("00:00:00:00:00:00"));
	// fm.set_match().set_vlan_vid(vid | rofl::openflow::OFPVID_PRESENT);
	fm.set_match().set_vlan_vid(vid | rofl::openflow::OFPVID_PRESENT, 0x0fff);

	if (do_pkt_in) {
		fm.set_out_port(rofl::openflow::OFPP_CONTROLLER);

		fm.set_instructions().set_inst_apply_actions().set_actions().
				add_action_output(rofl::cindex(0)).set_port_no(rofl::openflow::OFPP_CONTROLLER);
	}

	// fixme not supported in this table... check with BCM
	//fm.set_instructions().set_inst_clear_actions();

	fm.set_instructions().set_inst_goto_table().set_table_id(OFDPA_FLOW_TABLE_ID_ACL_POLICY);


	std::cout << "write flow-mod:" << std::endl << fm;
	// fixme add cookie
	dpt.send_flow_mod_message(rofl::cauxid(0), fm);
}

} /* namespace basebox */
