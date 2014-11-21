/*
 * cethendpnt.cpp
 *
 *  Created on: 07.08.2014
 *      Author: andreas
 */

#include "cethendpnt.hpp"
#include "cethcore.hpp"

using namespace roflibs::eth;


void
cethendpnt::handle_dpt_open(
		rofl::crofdpt& dpt)
{
	try {
		if (rofl::openflow::OFP_VERSION_UNKNOWN == dpt.get_version()) {
			return;
		}

		dpt_state = STATE_ATTACHED;

		rofl::cindex index(0);

		/*
		 * local unicast address
		 */
		rofl::openflow::cofflowmod fm(dpt.get_version());
		fm.set_command(rofl::openflow::OFPFC_ADD);
		fm.set_idle_timeout(0);
		fm.set_hard_timeout(0);
		fm.set_priority(0x8200);
		fm.set_cookie(cookie_endpoint);
		fm.set_table_id(table_id_eth_local); // local address stage
		fm.set_match().clear();
		//fm.set_match().set_eth_dst(dpt.get_ports().get_port(portno).get_hwaddr());
		fm.set_match().set_eth_dst(hwaddr);
		fm.set_match().set_vlan_vid(vid | rofl::openflow::OFPVID_PRESENT);
		fm.set_instructions().clear();
		fm.set_instructions().set_inst_goto_table().set_table_id(table_id_eth_local+1);
#if 0
		if (not tagged) {
			fm.set_instructions().set_inst_apply_actions().set_actions().
					add_action_pop_vlan(rofl::cindex(0));
		}
#endif
		dpt.send_flow_mod_message(rofl::cauxid(0), fm);


	} catch (rofl::eRofBaseCongested& e) {
		// TODO: handle congested control channel
	}
}

void
cethendpnt::handle_dpt_close(
		rofl::crofdpt& dpt)
{
	try {
		if (rofl::openflow::OFP_VERSION_UNKNOWN == dpt.get_version()) {
			return;
		}

		dpt_state = STATE_DETACHED;

		rofl::cindex index(0);

		/*
		 * local unicast address
		 */
		rofl::openflow::cofflowmod fm(dpt.get_version());
		fm.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		fm.set_idle_timeout(0);
		fm.set_hard_timeout(0);
		fm.set_priority(0x8200);
		fm.set_cookie(cookie_endpoint);
		fm.set_table_id(table_id_eth_local); // local address stage
		fm.set_match().clear();
		fm.set_match().set_eth_dst(hwaddr);
		//fm.set_match().set_eth_dst(dpt.get_ports().get_port(portno).get_hwaddr());
		fm.set_match().set_vlan_vid(vid | rofl::openflow::OFPVID_PRESENT);
		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

	} catch (rofl::eRofBaseCongested& e) {
		// TODO: handle congested control channel
	}
}



void
cethendpnt::handle_packet_in(
		rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_packet_in& msg)
{
	rofcore::logging::debug << "[cethendpnt][handle_packet_in] pkt received: " << std::endl << msg;
	// store packet in ethcore and thus, tap devices
	cethcore::set_eth_core(dpt.get_dpid()).handle_packet_in(dpt, auxid, msg);
}



void
cethendpnt::handle_flow_removed(
		rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_flow_removed& msg)
{
	// nothing to do
}



void
cethendpnt::handle_port_status(
		rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_port_status& msg)
{
	if (STATE_ATTACHED != dpt_state) {
		return;
	}
}



void
cethendpnt::handle_error_message(
		rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_error& msg)
{
	// TODO
}



