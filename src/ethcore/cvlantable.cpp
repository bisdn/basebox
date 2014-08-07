/*
 * cvlantable.cpp
 *
 *  Created on: 03.08.2014
 *      Author: andreas
 */

#include "cvlantable.hpp"

using namespace ethcore;

/*static*/std::map<cdpid, cvlantable*> cvlantable::vtables;

void
cvlantable::handle_dpt_open(rofl::crofdpt& dpt)
{
	state = STATE_ATTACHED;
	for (std::map<uint16_t, cvlan>::iterator
			it = vlans.begin(); it != vlans.end(); ++it) {
		it->second.handle_dpt_open(dpt);
	}
}



void
cvlantable::handle_dpt_close(rofl::crofdpt& dpt)
{
	state = STATE_DETACHED;
	for (std::map<uint16_t, cvlan>::iterator
			it = vlans.begin(); it != vlans.end(); ++it) {
		it->second.handle_dpt_close(dpt);
	}
}



void
cvlantable::handle_packet_in(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_packet_in& msg)
{
	try {
		uint16_t vid = msg.get_match().get_vlan_vid_value();

		if (has_vlan(vid)) {
			set_vlan(vid).handle_packet_in(dpt, auxid, msg);
		} else {
			drop_buffer(auxid, msg.get_buffer_id());
		}

	} catch (rofl::openflow::eOxmNotFound& e) {
		rofcore::logging::debug << "[cvlantable][handle_packet_in] no IN-PORT match found" << std::endl;
	}
}



void
cvlantable::handle_flow_removed(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_flow_removed& msg)
{
	try {
		uint16_t vid = msg.get_match().get_vlan_vid_value();

		if (has_vlan(vid)) {
			set_vlan(vid).handle_flow_removed(dpt, auxid, msg);
		}

	} catch (rofl::openflow::eOxmNotFound& e) {
		rofcore::logging::debug << "[cvlantable][handle_flow_removed] no IN-PORT match found" << std::endl;
	}
}



void
cvlantable::handle_port_status(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_port_status& msg)
{
	for (std::map<uint16_t, cvlan>::iterator
			it = vlans.begin(); it != vlans.end(); ++it) {
		it->second.handle_port_status(dpt, auxid, msg);
	}
}



void
cvlantable::handle_error_message(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_error& msg)
{
	for (std::map<uint16_t, cvlan>::iterator
			it = vlans.begin(); it != vlans.end(); ++it) {
		it->second.handle_error_message(dpt, auxid, msg);
	}
}



void
cvlantable::drop_buffer(const rofl::cauxid& auxid, uint32_t buffer_id)
{
	try {
		if (STATE_ATTACHED != state) {
			return;
		}

		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dpid.get_dpid());
		rofl::openflow::cofactions empty(dpt.get_version());
		dpt.send_packet_out_message(auxid, buffer_id, rofl::openflow::OFPP_CONTROLLER, empty);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::debug << "[cvlantable][drop_buffer] unable to drop buffer, dpt not found" << std::endl;
	}
}


