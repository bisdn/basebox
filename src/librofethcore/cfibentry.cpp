/*
 * cfibentry.cpp
 *
 *  Created on: 15.07.2013
 *      Author: andreas
 */

#include "cfibentry.hpp"

using namespace ethcore;

void
cfibentry::handle_dpt_open(
		rofl::crofdpt& dpt)
{
	try {
		assert(dpt.get_dpid() == dpid.get_dpid());

		state = STATE_ATTACHED;

		if (rofl::openflow::OFP_VERSION_UNKNOWN == dpt.get_version()) {
			return;
		}

		// check for existence of our port on dpt
		if (not dpt.get_ports().has_port(portno)) {
			return;
		}

		// src table
		rofl::openflow::cofflowmod fm_src_table(dpt.get_version());
		fm_src_table.set_command(rofl::openflow::OFPFC_ADD);
		fm_src_table.set_table_id(src_stage_table_id);
		fm_src_table.set_priority(0x8000);
		fm_src_table.set_idle_timeout(entry_timeout);
		fm_src_table.set_flags(rofl::openflow13::OFPFF_SEND_FLOW_REM);
		fm_src_table.set_match().set_in_port(portno);
		fm_src_table.set_match().set_vlan_vid(vid | rofl::openflow::OFPVID_PRESENT);
		fm_src_table.set_match().set_eth_src(lladdr); // yes, indeed: set_eth_src for dst
		fm_src_table.set_instructions().add_inst_goto_table().set_table_id(dst_stage_table_id);
		dpt.send_flow_mod_message(rofl::cauxid(0), fm_src_table);

		// dst table
		rofl::openflow::cofflowmod fm_dst_table(dpt.get_version());
		fm_dst_table.set_command(rofl::openflow::OFPFC_ADD);
		fm_dst_table.set_table_id(dst_stage_table_id);
		fm_dst_table.set_priority(0x8000);
		fm_dst_table.set_idle_timeout(entry_timeout);
		fm_dst_table.set_flags(rofl::openflow13::OFPFF_SEND_FLOW_REM);
		fm_dst_table.set_match().set_vlan_vid(vid | rofl::openflow::OFPVID_PRESENT);
		fm_dst_table.set_match().set_eth_dst(lladdr);
		rofl::cindex index(0);
		if (not tagged) {
			fm_dst_table.set_instructions().set_inst_apply_actions().set_actions().add_action_pop_vlan(index++);
		}
		fm_dst_table.set_instructions().set_inst_apply_actions().set_actions().add_action_output(index++).set_port_no(portno);
		dpt.send_flow_mod_message(rofl::cauxid(0), fm_dst_table);

	} catch (rofl::eRofSockTxAgain& e) {
		// TODO: handle congested control channel
	}
}



void
cfibentry::handle_dpt_close(
		rofl::crofdpt& dpt)
{
	try {
		assert(dpt.get_dpid() == dpid.get_dpid());

		state = STATE_DETACHED;

		if (rofl::openflow::OFP_VERSION_UNKNOWN == dpt.get_version()) {
			return;
		}

		// check for existence of our port on dpt
		if (not dpt.get_ports().has_port(portno)) {
			return;
		}

		// src table
		rofl::openflow::cofflowmod fm_src_table(dpt.get_version());
		fm_src_table.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		fm_src_table.set_table_id(src_stage_table_id);
		fm_src_table.set_priority(0x8000);
		fm_src_table.set_idle_timeout(entry_timeout);
		fm_src_table.set_flags(rofl::openflow13::OFPFF_SEND_FLOW_REM);
		fm_src_table.set_match().set_in_port(portno);
		fm_src_table.set_match().set_vlan_vid(vid | rofl::openflow::OFPVID_PRESENT);
		fm_src_table.set_match().set_eth_src(lladdr); // yes, indeed: set_eth_src for dst
		dpt.send_flow_mod_message(rofl::cauxid(0), fm_src_table);

		// dst table
		rofl::openflow::cofflowmod fm_dst_table(dpt.get_version());
		fm_dst_table.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		fm_dst_table.set_table_id(dst_stage_table_id);
		fm_dst_table.set_priority(0x8000);
		fm_dst_table.set_flags(rofl::openflow13::OFPFF_SEND_FLOW_REM);
		fm_dst_table.set_idle_timeout(entry_timeout);
		fm_dst_table.set_match().set_vlan_vid(vid | rofl::openflow::OFPVID_PRESENT);
		fm_dst_table.set_match().set_eth_dst(lladdr);
		dpt.send_flow_mod_message(rofl::cauxid(0), fm_dst_table);

	} catch (rofl::eRofSockTxAgain& e) {
		// TODO: handle congested control channel
	}
}



void
cfibentry::handle_packet_in(
		rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_packet_in& msg)
{
	try {
		// oops ...
		if (STATE_ATTACHED != state) {
			return;
		}

		// sanity check
		if (rofl::openflow::OFP_VERSION_UNKNOWN == dpt.get_version()) {
			return;
		}

		// has station moved to another port? src-stage flowmod entry has not matched causing this packet-in event
		uint32_t in_port = msg.get_match().get_in_port();
		if (in_port != portno) {
			handle_dpt_close(rofl::crofdpt::get_dpt(dpid.get_dpid()));
			portno = in_port;
			handle_dpt_open(rofl::crofdpt::get_dpt(dpid.get_dpid()));
		}

#if 0
		// handle buffered packet
		rofl::openflow::cofactions actions(dpt.get_version());
		rofl::cindex index(0);
		if (not tagged) {
			actions.add_action_pop_vlan(index++);
		}
		actions.add_action_output(index++).set_port_no(portno);

		if (msg.get_buffer_id() == rofl::openflow::OFP_NO_BUFFER) {
			dpt.send_packet_out_message(auxid, msg.get_buffer_id(),
					msg.get_match().get_in_port(), actions,
					msg.get_packet().soframe(), msg.get_packet().framelen());
		} else {
			dpt.send_packet_out_message(auxid, msg.get_buffer_id(),
					msg.get_match().get_in_port(), actions);
		}
#endif

	} catch (rofl::eRofSockTxAgain& e) {
		// TODO: handle control channel congestion

	} catch (rofl::openflow::eOxmNotFound& e) {

	}
}



void
cfibentry::handle_flow_removed(
		rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_flow_removed& msg)
{
	fib->fib_expired(*this);
}



void
cfibentry::handle_port_status(
		rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_port_status& msg)
{
	if (STATE_ATTACHED != state) {
		return;
	}

	if (msg.get_port().get_port_no() != portno) {
		return;
	}

	switch (msg.get_reason()) {
	case rofl::openflow::OFPPR_ADD: {
		handle_dpt_open(dpt);
	} break;
	case rofl::openflow::OFPPR_MODIFY: {
		// TODO: port up/down?
	} break;
	case rofl::openflow::OFPPR_DELETE: {
		handle_dpt_close(dpt);
	} break;
	}
}



void
cfibentry::handle_error_message(
		rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_error& msg)
{

}



void
cfibentry::drop_buffer(const rofl::cauxid& auxid, uint32_t buffer_id)
{
	try {
		if (STATE_ATTACHED != state) {
			return;
		}

		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dpid.get_dpid());
		rofl::openflow::cofactions empty(dpt.get_version());
		dpt.send_packet_out_message(auxid, buffer_id, rofl::openflow::OFPP_CONTROLLER, empty);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::debug << "[cfibentry][drop_buffer] unable to drop buffer, dpt not found" << std::endl;
	}
}



#if 0
void
cfibentry::flow_mod_configure(enum flow_mod_cmd_t flow_mod_cmd)
{
	try {
		rofl::crofbase *rofbase = fib->get_rofbase();
		rofl::crofdpt *dpt = rofbase->dpt_find(dpid);

		if ((NULL == dpt) || (rofl::openflow::OFP_VERSION_UNKNOWN == dpt->get_version())) {
			return;
		}

		rofl::openflow::cofflowmod fe(dpt->get_version());

		/* table 'src_stage_table_id':
		 *
		 * if src mac address is already known, move on to next table, otherwise, send packet to controller
		 *
		 * this allows the controller to learn yet unknown src macs when being received on a port
		 */
		if (true) {
			fe.reset();
			switch (flow_mod_cmd) {
			case FLOW_MOD_ADD:		fe.set_command(rofl::openflow::OFPFC_ADD);				break;
			case FLOW_MOD_MODIFY:	fe.set_command(rofl::openflow::OFPFC_MODIFY_STRICT); 	break;
			case FLOW_MOD_DELETE:	fe.set_command(rofl::openflow::OFPFC_DELETE_STRICT);	break;
			}
			fe.set_table_id(src_stage_table_id);
			fe.set_priority(0x8000);
			fe.set_idle_timeout(entry_timeout);
			fe.set_flags(rofl::openflow12::OFPFF_SEND_FLOW_REM);
			fe.match.set_in_port(portno);
			fe.match.set_vlan_vid(vid);
			fe.match.set_eth_src(lladdr); // yes, indeed: set_eth_src for dst
			fe.instructions.add_inst_goto_table().set_table_id(dst_stage_table_id);

			dpt->send_flow_mod_message(fe);
		}

		//dpt->send_barrier_request();

		/* table 'dst_stage_table_id':
		 *
		 * add a second rule to dst-stage: checks for destination
		 */
		if (true) {
			fe.reset();
			switch (flow_mod_cmd) {
			case FLOW_MOD_ADD:		fe.set_command(rofl::openflow::OFPFC_ADD);				break;
			case FLOW_MOD_MODIFY:	fe.set_command(rofl::openflow::OFPFC_MODIFY_STRICT); 	break;
			case FLOW_MOD_DELETE:	fe.set_command(rofl::openflow::OFPFC_DELETE_STRICT);	break;
			}
			fe.set_table_id(dst_stage_table_id);
			fe.set_priority(0x8000);
			fe.set_idle_timeout(entry_timeout);
			fe.match.set_vlan_vid(vid);
			fe.match.set_eth_dst(lladdr);
			fe.instructions.add_inst_apply_actions();
			if (not tagged)
				fe.instructions.set_inst_apply_actions().get_actions().append_action_pop_vlan();
			fe.instructions.set_inst_apply_actions().get_actions().append_action_output(portno);

			dpt->send_flow_mod_message(fe);
		}

		//dpt->send_barrier_request();

	} catch (rofl::eRofBaseNotFound& e) {

	} catch (rofl::eBadVersion& e) {
		logging::error << "[ethcore][cfibentry] data path already disconnected, unable to remove our entries" << std::endl;
	}
}
#endif

