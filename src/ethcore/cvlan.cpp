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
		rofl::crofdpt& dpt)
{
	state = STATE_ATTACHED;
	for (std::map<uint32_t, cmemberport>::iterator
			it = ports.begin(); it != ports.end(); ++it) {
		it->second.handle_dpt_open(dpt);
	}
	for (std::map<rofl::caddress_ll, cfibentry>::iterator
			it = fib.begin(); it != fib.end(); ++it) {
		it->second.handle_dpt_open(dpt);
	}
}



void
cvlan::handle_dpt_close(
		rofl::crofdpt& dpt)
{
	state = STATE_DETACHED;
	for (std::map<rofl::caddress_ll, cfibentry>::iterator
			it = fib.begin(); it != fib.end(); ++it) {
		it->second.handle_dpt_close(dpt);
	}
	for (std::map<uint32_t, cmemberport>::iterator
			it = ports.begin(); it != ports.end(); ++it) {
		it->second.handle_dpt_close(dpt);
	}
}



void
cvlan::handle_packet_in(
		rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_packet_in& msg)
{
	try {
		assert(dpid.get_dpid() == dpt.get_dpid());
		assert(vid == msg.get_match().get_vlan_vid_value());


		uint32_t in_port = msg.get_match().get_in_port();
		if (not has_port(in_port)) {
			rofcore::logging::debug << "[cvlan][handle_packet_in] packet-in on invalid port" << std::endl << msg;
			drop_buffer(auxid, msg.get_buffer_id());
			return;
		}


		const rofl::caddress_ll& lladdr = msg.get_match().get_eth_src_addr();
		if (lladdr.is_multicast() || lladdr.is_null()) {
			rofcore::logging::debug << "[cvlan][handle_packet_in] invalid source lladdr found" << std::endl << msg;
			drop_buffer(auxid, msg.get_buffer_id());
			return;
		}


		set_fib_entry(lladdr, in_port, get_port(in_port).get_tagged()).handle_packet_in(dpt, auxid, msg);

	} catch (rofl::openflow::eOxmNotFound& e) {
		rofcore::logging::debug << "[cvlan][handle_packet_in] match(es) not found" << std::endl << msg;
		drop_buffer(auxid, msg.get_buffer_id());
	}
}



void
cvlan::handle_flow_removed(
		rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_flow_removed& msg)
{

}



void
cvlan::handle_port_status(
		rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_port_status& msg)
{

}



void
cvlan::handle_error_message(
		rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_error& msg)
{

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


