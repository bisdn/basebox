/*
 * cvlan.cpp
 *
 *  Created on: 07.08.2014
 *      Author: andreas
 */

#include "cvlan.hpp"

using namespace ethcore;

void
cvlan::handle_dpt_open(
		rofl::crofdpt& dpt, uint32_t group_id)
{
	try {
		this->group_id = group_id;

		state = STATE_ATTACHED;

		// set flooding group entry itself
		update_group_entry_buckets(rofl::openflow::OFPGC_ADD);

		// set redirecting entry for this vlan's flooding group entry
		rofl::cindex index(0);
		rofl::openflow::cofflowmod fm(dpt.get_version());

		fm.set_command(rofl::openflow::OFPFC_ADD);
		fm.set_table_id(2); // TODO: table_id
		fm.set_buffer_id(rofl::openflow::OFP_NO_BUFFER);
		fm.set_idle_timeout(0);
		fm.set_hard_timeout(0);
		fm.set_priority(0x8000);
		fm.set_match().set_vlan_vid(vid | rofl::openflow::OFPVID_PRESENT);
		fm.set_instructions().set_inst_apply_actions().
				set_actions().add_action_group(index++).set_group_id(group_id);

		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		// send notification to all member ports
		for (std::map<uint32_t, cmemberport>::iterator
				it = ports.begin(); it != ports.end(); ++it) {
			it->second.handle_dpt_open(dpt);
		}

		// send notification to all fib entries
		for (std::map<rofl::caddress_ll, cfibentry>::iterator
				it = fib.begin(); it != fib.end(); ++it) {
			it->second.handle_dpt_open(dpt);
		}

	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::debug << "[cvlan][handle_dpt_open] control channel congested" << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::debug << "[cvlan][handle_dpt_open] control channel not connected" << std::endl;
	}
}



void
cvlan::handle_dpt_close(
		rofl::crofdpt& dpt)
{
	try {
		state = STATE_DETACHED;

		// send notification to all fib entries
		for (std::map<rofl::caddress_ll, cfibentry>::iterator
				it = fib.begin(); it != fib.end(); ++it) {
			it->second.handle_dpt_close(dpt);
		}

		// send notification to all member ports
		for (std::map<uint32_t, cmemberport>::iterator
				it = ports.begin(); it != ports.end(); ++it) {
			it->second.handle_dpt_close(dpt);
		}

		// remove redirecting entry for this vlan's flooding group entry
		rofl::openflow::cofflowmod fm(dpt.get_version());

		fm.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		fm.set_table_id(2); // TODO: table_id
		fm.set_buffer_id(rofl::openflow::OFP_NO_BUFFER);
		fm.set_idle_timeout(0);
		fm.set_hard_timeout(0);
		fm.set_priority(0x8000);
		fm.set_match().set_vlan_vid(vid | rofl::openflow::OFPVID_PRESENT);

		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		// remove flooding group entry itself
		rofl::openflow::cofgroupmod gm(dpt.get_version());
		gm.set_command(rofl::openflow::OFPGC_DELETE);
		gm.set_group_id(group_id);
		gm.set_type(rofl::openflow::OFPGT_ALL);

	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::debug << "[cvlan][handle_dpt_close] control channel congested" << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::debug << "[cvlan][handle_dpt_close] control channel not connected" << std::endl;
	}
}



void
cvlan::handle_packet_in(
		rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_packet_in& msg)
{
	try {
		assert(dpid.get_dpid() == dpt.get_dpid());
		assert(vid == (msg.get_match().get_vlan_vid_value() & ((uint16_t)~rofl::openflow::OFPVID_PRESENT)));

		uint32_t in_port = msg.get_match().get_in_port();
		const rofl::caddress_ll& lladdr = msg.get_match().get_eth_src_addr();
		if (lladdr.is_multicast() || lladdr.is_null()) {
			rofcore::logging::debug << "[cvlan][handle_packet_in] invalid source lladdr found" << std::endl << msg;
			drop_buffer(auxid, msg.get_buffer_id());
			return;
		}

		set_fib_entry(lladdr, in_port).handle_packet_in(dpt, auxid, msg);

	} catch (rofl::openflow::eOxmNotFound& e) {
		rofcore::logging::debug << "[cvlan][handle_packet_in] match(es) not found" << std::endl << msg;
		drop_buffer(auxid, msg.get_buffer_id());
	} catch (eFibEntryPortNotMember& e) {
		rofcore::logging::debug << "[cvlan][handle_packet_in] packet-in on invalid port" << std::endl << msg;
		drop_buffer(auxid, msg.get_buffer_id());
	}
}



void
cvlan::handle_flow_removed(
		rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_flow_removed& msg)
{
	try {
		assert(dpid.get_dpid() == dpt.get_dpid());
		assert(vid == (msg.get_match().get_vlan_vid_value() & ((uint16_t)~rofl::openflow::OFPVID_PRESENT)));

		// TODO: check port here?

		rofl::caddress_ll lladdr;

		if (msg.get_table_id() == 1) {
			lladdr = msg.get_match().get_eth_src_addr();
		} else
		if (msg.get_table_id() == 2) {
			lladdr = msg.get_match().get_eth_dst_addr();
		} else {
			return;
		}

		if (lladdr.is_multicast() || lladdr.is_null()) {
			rofcore::logging::debug << "[cvlan][handle_flow_removed] invalid source lladdr found" << std::endl << msg;
			return;
		}

		if (has_fib_entry(lladdr)) {
			set_fib_entry(lladdr).handle_flow_removed(dpt, auxid, msg); // notify fib entry
		}

	} catch (rofl::openflow::eOxmNotFound& e) {
		rofcore::logging::debug << "[cvlan][handle_flow_removed] match(es) not found" << std::endl << msg;
	}
}



void
cvlan::handle_port_status(
		rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_port_status& msg)
{
	try {
		uint32_t portno = msg.get_port().get_port_no();

		if (not has_port(portno)) {
			return;
		}

		set_port(portno).handle_port_status(dpt, auxid, msg);

		for (std::map<rofl::caddress_ll, cfibentry>::iterator
				it = fib.begin(); it != fib.end(); ++it) {
			it->second.handle_port_status(dpt, auxid, msg);
		}

	} catch (rofl::openflow::eOxmNotFound& e) {
		rofcore::logging::debug << "[cvlan][handle_port_status] port not found" << std::endl << msg;
	}
}



void
cvlan::handle_error_message(
		rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_error& msg)
{
	// TODO
}



void
cvlan::drop_buffer(const rofl::cauxid& auxid, uint32_t buffer_id)
{
	try {
		if (STATE_ATTACHED != state) {
			return;
		}

		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dpid.get_dpid());
		rofl::openflow::cofactions empty(dpt.get_version());
		dpt.send_packet_out_message(auxid, buffer_id, rofl::openflow::OFPP_CONTROLLER, empty);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::debug << "[cvlan][drop_buffer] unable to drop buffer, dpt not found" << std::endl;
	}
}



void
cvlan::update_group_entry_buckets(uint16_t command)
{
	try {
		if (STATE_ATTACHED != state) {
			return;
		}

		// update flooding group entry
		rofl::openflow::cofgroupmod gm(rofl::crofdpt::get_dpt(dpid.get_dpid()).get_version());
		gm.set_command(command);
		gm.set_group_id(group_id);
		gm.set_type(rofl::openflow::OFPGT_ALL);
		unsigned int bucket_id = 0;
		for (std::map<uint32_t, cmemberport>::iterator
				it = ports.begin(); it != ports.end(); ++it) {
			rofl::cindex index(0);
			if (not it->second.get_tagged()) {
				gm.set_buckets().add_bucket(bucket_id).set_actions().
						add_action_pop_vlan(index++);
			}
			gm.set_buckets().set_bucket(bucket_id).set_actions().
						add_action_output(index++).set_port_no(it->second.get_port_no());
			bucket_id++;
		}

		rofl::crofdpt::get_dpt(dpid.get_dpid()).send_group_mod_message(rofl::cauxid(0), gm);

	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::debug << "[cvlan][update_group_entry_buckets] control channel congested" << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::debug << "[cvlan][update_group_entry_buckets] control channel not connected" << std::endl;
	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::debug << "[cvlan][update_group_entry_buckets] dpt not found" << std::endl;
	}
}



