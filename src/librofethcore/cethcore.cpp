/*
 * cethcore.cpp
 *
 *  Created on: 03.08.2014
 *      Author: andreas
 */

#include "cethcore.hpp"

using namespace ethcore;

/*static*/std::map<cdpid, cethcore*> cethcore::ethcores;

void
cethcore::handle_dpt_open(rofl::crofdpt& dpt)
{
	try {

		switch (state) {
		case STATE_IDLE:
		case STATE_DETACHED: {
			state = STATE_QUERYING;
			dpt.send_port_desc_stats_request(rofl::cauxid(0), 0);
		} return;
		default: {
			state = STATE_ATTACHED;

		};
		}

		for (std::map<uint16_t, cvlan>::iterator
				it = vlans.begin(); it != vlans.end(); ++it) {
			if (it->second.get_group_id() != 0) {
				release_group_id(it->second.get_group_id());
			}
			it->second.handle_dpt_open(dpt, get_next_group_id());
		}

		// install miss entry for src stage table
		rofl::openflow::cofflowmod fm(dpt.get_version());
		fm.set_command(rofl::openflow::OFPFC_ADD);
		fm.set_priority(0x0000);
		fm.set_table_id(1); // TODO: table_id
		fm.set_instructions().set_inst_apply_actions().set_actions().
				add_action_output(rofl::cindex(0)).set_port_no(rofl::openflow::OFPP_CONTROLLER);

		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::debug << "[cethcore][handle_dpt_open] control channel congested" << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::debug << "[cethcore][handle_dpt_open] control channel not connected" << std::endl;
	}
}



void
cethcore::handle_dpt_close(rofl::crofdpt& dpt)
{
	try {
		state = STATE_DETACHED;

		for (std::map<uint16_t, cvlan>::iterator
				it = vlans.begin(); it != vlans.end(); ++it) {
			it->second.handle_dpt_close(dpt);
			release_group_id(it->second.get_group_id());
		}

		// install miss entry for src stage table
		rofl::openflow::cofflowmod fm(dpt.get_version());
		fm.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		fm.set_priority(0x0000);
		fm.set_table_id(1); // TODO: table_id

		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::debug << "[cethcore][handle_dpt_close] control channel congested" << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::debug << "[cethcore][handle_dpt_close] control channel not connected" << std::endl;
	}
}



void
cethcore::handle_packet_in(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_packet_in& msg)
{
	try {
		uint16_t vid = msg.get_match().get_vlan_vid_value();

		if (has_vlan(vid)) {
			set_vlan(vid).handle_packet_in(dpt, auxid, msg);
		} else {
			drop_buffer(auxid, msg.get_buffer_id());
		}

	} catch (rofl::openflow::eOxmNotFound& e) {
		rofcore::logging::debug << "[cethcore][handle_packet_in] no IN-PORT match found" << std::endl;
	}
}



void
cethcore::handle_flow_removed(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_flow_removed& msg)
{
	try {
		uint16_t vid = msg.get_match().get_vlan_vid_value();

		if (has_vlan(vid)) {
			set_vlan(vid).handle_flow_removed(dpt, auxid, msg);
		}

	} catch (rofl::openflow::eOxmNotFound& e) {
		rofcore::logging::debug << "[cethcore][handle_flow_removed] no IN-PORT match found" << std::endl;
	}
}



void
cethcore::handle_port_status(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_port_status& msg)
{
	for (std::map<uint16_t, cvlan>::iterator
			it = vlans.begin(); it != vlans.end(); ++it) {
		it->second.handle_port_status(dpt, auxid, msg);
	}
}



void
cethcore::handle_error_message(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_error& msg)
{
	for (std::map<uint16_t, cvlan>::iterator
			it = vlans.begin(); it != vlans.end(); ++it) {
		it->second.handle_error_message(dpt, auxid, msg);
	}
}



void
cethcore::handle_port_desc_stats_reply(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_port_desc_stats_reply& msg)
{
	dpt.set_ports() = msg.get_ports();

	switch (state) {
	case STATE_QUERYING: {
		state = STATE_ATTACHED;
		/*
		 * this code block adds all unspecified ports automatically to the default port vid defined for this cethcore instance
		 */
		for (std::map<uint32_t, rofl::openflow::cofport*>::const_iterator
				it = dpt.get_ports().get_ports().begin(); it != dpt.get_ports().get_ports().end(); ++it) {
			bool found = false;
			for (std::map<uint16_t, cvlan>::iterator
					jt = vlans.begin(); jt != vlans.end(); ++jt) {
				if (jt->second.has_port(it->first)) {
					found = true;
				}
			}
			if (not found) {
				set_vlan(default_pvid).add_port(it->first, false);
			}
		}

		/*
		 *
		 */
		handle_dpt_open(dpt);
	} break;
	default: {

	};
	}
}



void
cethcore::handle_port_desc_stats_reply_timeout(rofl::crofdpt& dpt, uint32_t xid)
{
	dpt.disconnect(rofl::cauxid(0));
}



void
cethcore::drop_buffer(const rofl::cauxid& auxid, uint32_t buffer_id)
{
	try {
		if (STATE_ATTACHED != state) {
			return;
		}

		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dpid.get_dpid());
		rofl::openflow::cofactions empty(dpt.get_version());
		dpt.send_packet_out_message(auxid, buffer_id, rofl::openflow::OFPP_CONTROLLER, empty);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::debug << "[cethcore][drop_buffer] unable to drop buffer, dpt not found" << std::endl;
	}
}



uint32_t
cethcore::get_next_group_id()
{
	uint32_t group_id = 1;
	while (group_ids.find(group_id) != group_ids.end()) {
		group_id++;
	}
	group_ids.insert(group_id);
	return group_id;
}



void
cethcore::release_group_id(uint32_t group_id)
{
	group_ids.erase(group_id);
}



