#include "cipcore.hpp"

using namespace ipcore;

std::string cipcore::script_path_dpt_open 	= std::string("/var/lib/ipcore/dpath-open.sh");
std::string cipcore::script_path_dpt_close 	= std::string("/var/lib/ipcore/dpath-close.sh");
std::string cipcore::script_path_port_up 	= std::string("/var/lib/ipcore/port-up.sh");
std::string cipcore::script_path_port_down 	= std::string("/var/lib/ipcore/port-down.sh");


/*static*/cipcore* cipcore::sipcore = (cipcore*)NULL;


/*static*/
cipcore&
cipcore::get_instance(
		const rofl::cdptid& dptid, uint8_t in_ofp_table_id, uint8_t fwd_ofp_table_id, uint8_t out_ofp_table_id)
{
	if (NULL == cipcore::sipcore) {
		cipcore::sipcore = new cipcore(dptid, in_ofp_table_id, fwd_ofp_table_id, out_ofp_table_id);
	}
	return *(cipcore::sipcore);
}



void
cipcore::handle_dpt_open(rofl::crofdpt& dpt)
{
	state = STATE_ATTACHED;

	dptid = dpt.get_dptid();

	purge_dpt_entries();
	block_stp_frames();
	redirect_ipv4_multicast();
	redirect_ipv6_multicast();
	set_forwarding(true);

	for (std::map<uint32_t, clink*>::iterator
			it = links.begin(); it != links.end(); ++it) {
		it->second->handle_dpt_open(dpt);
	}
	for (std::map<unsigned int, croutetable>::iterator
			it = rtables.begin(); it != rtables.end(); ++it) {
		it->second.handle_dpt_open(dpt);
	}

	// call external scripting hook
	hook_dpt_attach();
}



void
cipcore::handle_dpt_close(rofl::crofdpt& dpt)
{
	if (dptid != dpt.get_dptid()) {
		return;
	}

	state = STATE_DETACHED;

	// call external scripting hook
	hook_dpt_detach();

	for (std::map<unsigned int, croutetable>::iterator
			it = rtables.begin(); it != rtables.end(); ++it) {
		it->second.handle_dpt_close(dpt);
	}
	for (std::map<uint32_t, clink*>::iterator
			it = links.begin(); it != links.end(); ++it) {
		it->second->handle_dpt_close(dpt);
	}
}



void
cipcore::handle_port_status(
		rofl::crofdpt& dpt,
		const rofl::cauxid& auxid,
		rofl::openflow::cofmsg_port_status& msg)
{

	if (dpt.get_dptid() != dptid) {
		rofcore::logging::debug << "[cipcore] received PortStatus from invalid data path, ignoring" << std::endl;
		return;
	}

	uint32_t port_no = msg.get_port().get_port_no();

	rofcore::logging::debug << "[cipcore] Port-Status message rcvd:" << std::endl << msg;

	try {
		switch (msg.get_reason()) {
		case rofl::openflow::OFPPR_ADD: {
			set_link(port_no).tap_open();
			hook_port_up(msg.get_port().get_name());
		} break;
		case rofl::openflow::OFPPR_MODIFY: {
			set_link(port_no).tap_open();
			hook_port_up(msg.get_port().get_name());
		} break;
		case rofl::openflow::OFPPR_DELETE: {
			set_link(port_no).tap_close();
			drop_link(port_no);
			hook_port_down(msg.get_port().get_name());
		} break;
		default: {
			rofcore::logging::debug << "[cipcore] received PortStatus with unknown reason code received, ignoring" << std::endl;
		};
		}

	} catch (rofcore::eNetDevCritical& e) {
		rofcore::logging::debug << "[cipcore] new port created: unable to create tap device: " << msg.get_port().get_name() << std::endl;
	}
}



void
cipcore::handle_packet_in(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_packet_in& msg)
{
	try {
		uint32_t port_no = msg.get_match().get_in_port();

		if (not has_link(port_no)) {
			rofcore::logging::debug << "[cipcore] received PacketIn message for unknown port, ignoring" << std::endl;
			return;
		}

		set_link(port_no).handle_packet_in(msg.get_packet());

		// drop buffer on data path, if the packet was stored there, as it is consumed entirely by the underlying kernel
		if (rofl::openflow::base::get_ofp_no_buffer(dpt.get_version()) != msg.get_buffer_id()) {
			dpt.drop_buffer(auxid, msg.get_buffer_id());
		}

	} catch (rofcore::ePacketPoolExhausted& e) {
		rofcore::logging::warn << "[cipcore] received PacketIn message, but buffer-pool exhausted" << std::endl;
	} catch (rofl::openflow::eOxmNotFound& e) {
		rofcore::logging::debug << "[cipcore] received PacketIn message without in-port match, ignoring" << std::endl;
	}
}



void
cipcore::handle_error_message(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_error& msg)
{
	rofcore::logging::warn << "[cipcore] error message rcvd:" << std::endl << msg;
}



void
cipcore::handle_flow_removed(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_flow_removed& msg)
{
	rofcore::logging::info << "[cipcore] Flow-Removed message rcvd:" << std::endl << msg;
}



void
cipcore::set_forwarding(bool forward)
{
	try {
		rofl::openflow::cofflowmod fed = rofl::openflow::cofflowmod(rofl::crofdpt::get_dpt(dptid).get_version());

		if (forward == true) {
			fed.set_command(rofl::openflow::OFPFC_MODIFY_STRICT);
		} else {
			fed.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		}
		fed.set_table_id(0);
		fed.set_idle_timeout(0);
		fed.set_hard_timeout(0);
		fed.set_priority(0); // lowest priority
		fed.set_instructions().add_inst_goto_table().set_table_id(1);

		rofl::crofdpt::get_dpt(dptid).send_flow_mod_message(rofl::cauxid(0), fed);

	} catch (rofl::eSocketTxAgain& e) {

		rofcore::logging::debug << "[cipcore] enable forwarding: control channel congested, rescheduling (TODO)" << std::endl;

	} catch (rofl::eRofDptNotFound& e) {

		rofcore::logging::debug << "[cipcore] enable forwarding: dptid not found: " << dptid << std::endl;

	}
}


void
cipcore::link_created(unsigned int ifindex)
{
	// do nothing
}


void
cipcore::link_updated(unsigned int ifindex)
{
	// do nothing
}


void
cipcore::link_deleted(unsigned int ifindex)
{
	// do nothing
}


void
cipcore::addr_in4_created(unsigned int ifindex, uint16_t adindex)
{
	try {
		if (not has_link_by_ifindex(ifindex)) {
			return; // ignore address events for unknown interfaces
		}

		if (not get_link_by_ifindex(ifindex).has_addr_in4(adindex)) {
			set_link_by_ifindex(ifindex).set_addr_in4(adindex);
			rofcore::logging::debug << "[cipcore][addr_in4_created] state:" << std::endl << *this;
		}

	} catch (std::runtime_error& e) {
		rofcore::logging::error << "[cipcore] addr_in4 create: exception " << e.what() << std::endl;
	}
}

void
cipcore::addr_in4_updated(unsigned int ifindex, uint16_t adindex)
{
	try {

		if (not has_link_by_ifindex(ifindex)) {
			return; // ignore address events for unknown interfaces
		}

		if (not get_link_by_ifindex(ifindex).has_addr_in4(adindex)) {
			set_link_by_ifindex(ifindex).set_addr_in4(adindex);
			rofcore::logging::debug << "[cipcore][addr_in4_updated] state:" << std::endl << *this;
		}

	} catch (std::runtime_error& e) {
		rofcore::logging::error << "[cipcore] addr_in4 update: exception " << e.what() << std::endl;
	}
}

void
cipcore::addr_in4_deleted(unsigned int ifindex, uint16_t adindex)
{
	try {

		if (not has_link_by_ifindex(ifindex)) {
			return; // ignore address events for unknown interfaces
		}

		if (get_link_by_ifindex(ifindex).has_addr_in4(adindex)) {
			set_link_by_ifindex(ifindex).drop_addr_in4(adindex);
			rofcore::logging::debug << "[cipcore][addr_in4_deleted] state:" << std::endl << *this;
		}

	} catch (std::runtime_error& e) {
		rofcore::logging::error << "[cipcore] addr_in4 delete: exception " << e.what() << std::endl;
	}
}

void
cipcore::addr_in6_created(unsigned int ifindex, uint16_t adindex)
{
	try {

		if (not has_link_by_ifindex(ifindex)) {
			return; // ignore address events for unknown interfaces
		}

		if (not get_link_by_ifindex(ifindex).has_addr_in6(adindex)) {
			set_link_by_ifindex(ifindex).set_addr_in6(adindex);
			rofcore::logging::debug << "[cipcore][addr_in6_created] state:" << std::endl << *this;
		}

	} catch (std::runtime_error& e) {
		rofcore::logging::error << "[cipcore] addr_in6 create: exception " << e.what() << std::endl;
	}
}

void
cipcore::addr_in6_updated(unsigned int ifindex, uint16_t adindex)
{
	try {

		if (not has_link_by_ifindex(ifindex)) {
			return; // ignore address events for unknown interfaces
		}

		if (not get_link_by_ifindex(ifindex).has_addr_in6(adindex)) {
			set_link_by_ifindex(ifindex).set_addr_in6(adindex);
			rofcore::logging::debug << "[cipcore][addr_in6_updated] state:" << std::endl << *this;
		}

	} catch (std::runtime_error& e) {
		rofcore::logging::error << "[cipcore] addr_in6 update: exception " << e.what() << std::endl;
	}
}

void
cipcore::addr_in6_deleted(unsigned int ifindex, uint16_t adindex)
{
	try {

		if (not has_link_by_ifindex(ifindex)) {
			return; // ignore address events for unknown interfaces
		}

		if (get_link_by_ifindex(ifindex).has_addr_in6(adindex)) {
			set_link_by_ifindex(ifindex).drop_addr_in6(adindex);
			rofcore::logging::debug << "[cipcore][addr_in6_deleted] state:" << std::endl << *this;
		}

	} catch (std::runtime_error& e) {
		rofcore::logging::error << "[cipcore] addr_in6 delete: exception " << e.what() << std::endl;
	}
}


void
cipcore::route_in4_created(uint8_t table_id, unsigned int rtindex)
{
	try {
		// ignore local route table and unspecified table_id
		if ((RT_TABLE_LOCAL/*255*/ == table_id) || (RT_TABLE_UNSPEC/*0*/ == table_id)) {
		//if ((RT_TABLE_UNSPEC/*0*/ == table_id)) {
			rofcore::logging::debug << "[cipcore] create route => suppressing table_id=" << (unsigned int)table_id << std::endl;
			return;
		}

		rofcore::logging::info << "[cipcore] crtroute CREATE:" << std::endl << rofcore::cnetlink::get_instance().get_routes_in4(table_id).get_route(rtindex);

		if ((not has_table(table_id)) || (not get_table(table_id).has_route_in4(rtindex))) {
			set_table(table_id).add_route_in4(rtindex);
			rofcore::logging::debug << "[cipcore][route_in4_created] state:" << std::endl << *this;
		}

	} catch (rofcore::eNetLinkNotFound& e) {
		rofcore::logging::warn << "[cipcore] crtroute CREATE notification rcvd => "
				<< "unable to find crtroute for table-id:" << table_id << " rtindex:" << rtindex << std::endl;
	}
}



void
cipcore::route_in4_updated(uint8_t table_id, unsigned int rtindex)
{
	try {
		// ignore local route table and unspecified table_id
		if ((RT_TABLE_LOCAL/*255*/ == table_id) || (RT_TABLE_UNSPEC/*0*/ == table_id)) {
		//if ((RT_TABLE_UNSPEC/*0*/ == table_id)) {
			rofcore::logging::debug << "[cipcore] route update => suppressing table_id=" << (unsigned int)table_id << std::endl;
			return;
		}

		rofcore::logging::info << "[cipcore] route update:" << std::endl << rofcore::cnetlink::get_instance().get_routes_in4(table_id).get_route(rtindex);

		if ((not has_table(table_id)) || (not get_table(table_id).has_route_in4(rtindex))) {
			set_table(table_id).add_route_in4(rtindex);
			rofcore::logging::debug << "[cipcore][route_in4_updated] state:" << std::endl << *this;
		}

		// do nothing here, this event is handled directly by dptroute instance

	} catch (rofcore::eNetLinkNotFound& e) {
		rofcore::logging::warn << "[cipcore] crtroute UPDATE notification rcvd => "
				<< "unable to find crtroute for table-id:" << table_id << " rtindex:" << rtindex << std::endl;
	}
}



void
cipcore::route_in4_deleted(uint8_t table_id, unsigned int rtindex)
{
	try {
		// ignore local route table and unspecified table_id
		if ((RT_TABLE_LOCAL/*255*/ == table_id) || (RT_TABLE_UNSPEC/*0*/ == table_id)) {
		//if ((RT_TABLE_UNSPEC/*0*/ == table_id)) {
			rofcore::logging::debug << "[cipcore] route delete => suppressing table_id=" << (unsigned int)table_id << std::endl;
			return;
		}

		rofcore::logging::info << "[cipcore] crtroute DELETE:" << std::endl << rofcore::cnetlink::get_instance().get_routes_in4(table_id).get_route(rtindex);

		if (has_table(table_id) && get_table(table_id).has_route_in4(rtindex)) {
			set_table(table_id).drop_route_in4(rtindex);
			rofcore::logging::debug << "[cipcore][route_in4_deleted] state:" << std::endl << *this;
		}

	} catch (rofcore::eNetLinkNotFound& e) {
		rofcore::logging::warn << "[cipcore] crtroute DELETE notification rcvd => "
				<< "unable to find crtroute for table-id:" << table_id << " rtindex:" << rtindex << std::endl;
	}
}



void
cipcore::route_in6_created(uint8_t table_id, unsigned int rtindex)
{
	try {
		// ignore local route table and unspecified table_id
		if ((RT_TABLE_LOCAL/*255*/ == table_id) || (RT_TABLE_UNSPEC/*0*/ == table_id)) {
		//if ((RT_TABLE_UNSPEC/*0*/ == table_id)) {
			std::cerr << "ipcore::route_created() => suppressing table_id=" << (unsigned int)table_id << std::endl;
			return;
		}

		rofcore::logging::info << "[cipcore] crtroute CREATE:" << std::endl << rofcore::cnetlink::get_instance().get_routes_in6(table_id).get_route(rtindex);

		if ((not has_table(table_id)) || (not get_table(table_id).has_route_in6(rtindex))) {
			set_table(table_id).add_route_in6(rtindex);
			rofcore::logging::debug << "[cipcore][route_in6_created] state:" << std::endl << *this;
		}

	} catch (rofcore::eNetLinkNotFound& e) {
		rofcore::logging::warn << "[cipcore] crtroute CREATE notification rcvd => "
				<< "unable to find crtroute for table-id:" << table_id << " rtindex:" << rtindex << std::endl;
	}
}



void
cipcore::route_in6_updated(uint8_t table_id, unsigned int rtindex)
{
	try {
		// ignore local route table and unspecified table_id
		if ((RT_TABLE_LOCAL/*255*/ == table_id) || (RT_TABLE_UNSPEC/*0*/ == table_id)) {
		//if ((RT_TABLE_UNSPEC/*0*/ == table_id)) {
			rofcore::logging::debug << "[cipcore] route update => suppressing table_id=" << (unsigned int)table_id << std::endl;
			return;
		}

		rofcore::logging::info << "[cipcore] route update:" << std::endl << rofcore::cnetlink::get_instance().get_routes_in6(table_id).get_route(rtindex);

		if ((not has_table(table_id)) || (not get_table(table_id).has_route_in6(rtindex))) {
			set_table(table_id).add_route_in6(rtindex);
			rofcore::logging::debug << "[cipcore][route_in6_updated] state:" << std::endl << *this;
		}

		// do nothing here, this event is handled directly by dptroute instance

	} catch (rofcore::eNetLinkNotFound& e) {
		rofcore::logging::warn << "[cipcore] crtroute UPDATE notification rcvd => "
				<< "unable to find crtroute for table-id:" << table_id << " rtindex:" << rtindex << std::endl;
	}
}



void
cipcore::route_in6_deleted(uint8_t table_id, unsigned int rtindex)
{
	try {
		// ignore local route table and unspecified table_id
		if ((RT_TABLE_LOCAL/*255*/ == table_id) || (RT_TABLE_UNSPEC/*0*/ == table_id)) {
		//if ((RT_TABLE_UNSPEC/*0*/ == table_id)) {
			std::cerr << "ipcore::route_deleted() => suppressing table_id=" << (unsigned int)table_id << std::endl;
			return;
		}

		rofcore::logging::info << "[cipcore] crtroute DELETE:" << std::endl << rofcore::cnetlink::get_instance().get_routes_in6(table_id).get_route(rtindex);

		if (has_table(table_id) && get_table(table_id).has_route_in6(rtindex)) {
			set_table(table_id).drop_route_in6(rtindex);
			rofcore::logging::debug << "[cipcore][route_in6_deleted] state:" << std::endl << *this;
		}

	} catch (rofcore::eNetLinkNotFound& e) {
		rofcore::logging::warn << "[cipcore] crtroute DELETE notification rcvd => "
				<< "unable to find crtroute for table-id:" << table_id << " rtindex:" << rtindex << std::endl;
	}
}


void
cipcore::neigh_in4_created(unsigned int ifindex, uint16_t nbindex)
{
	try {

		if (not has_link_by_ifindex(ifindex)) {
			rofcore::logging::error << "[cipcore] neigh_in4 create: link ifindex not found: " << ifindex << std::endl;
			return;
		}

		if (not get_link_by_ifindex(ifindex).has_neigh_in4(nbindex)) {
			set_link_by_ifindex(ifindex).set_neigh_in4(nbindex);
			rofcore::logging::debug << "[cipcore][neigh_in4_created] state:" << std::endl << *this;
		}

	} catch (std::runtime_error& e) {
		rofcore::logging::error << "[cipcore] neigh_in4 create: exception " << e.what() << std::endl;
	}
}


void
cipcore::neigh_in4_updated(unsigned int ifindex, uint16_t nbindex)
{
	try {

		if (not has_link_by_ifindex(ifindex)) {
			rofcore::logging::error << "[cipcore] neigh_in4 update: link ifindex not found: " << ifindex << std::endl;
			return;
		}

		if (not get_link_by_ifindex(ifindex).has_neigh_in4(nbindex)) {
			set_link_by_ifindex(ifindex).set_neigh_in4(nbindex);
			rofcore::logging::debug << "[cipcore][neigh_in4_updated] state:" << std::endl << *this;
		}

	} catch (std::runtime_error& e) {
		rofcore::logging::error << "[cipcore] neigh_in4 update: exception " << e.what() << std::endl;
	}
}


void
cipcore::neigh_in4_deleted(unsigned int ifindex, uint16_t nbindex)
{
	try {

		if (not has_link_by_ifindex(ifindex)) {
			rofcore::logging::error << "[cipcore] neigh_in4 delete: link ifindex not found: " << ifindex << std::endl;
			return;
		}

		if (get_link_by_ifindex(ifindex).has_neigh_in6(nbindex)) {
			set_link_by_ifindex(ifindex).drop_neigh_in6(nbindex);
			rofcore::logging::debug << "[cipcore][neigh_in4_deleted] state:" << std::endl << *this;
		}

	} catch (std::runtime_error& e) {
		rofcore::logging::error << "[cipcore] neigh_in4 delete: exception " << e.what() << std::endl;
	}
}


void
cipcore::neigh_in6_created(unsigned int ifindex, uint16_t nbindex)
{
	try {

		if (not has_link_by_ifindex(ifindex)) {
			rofcore::logging::error << "[cipcore] neigh_in6 create: link ifindex not found: " << ifindex << std::endl;
			return;
		}

		if (not get_link_by_ifindex(ifindex).has_neigh_in6(nbindex)) {
			set_link_by_ifindex(ifindex).set_neigh_in6(nbindex);
			rofcore::logging::debug << "[cipcore][neigh_in6_created] state:" << std::endl << *this;
		}

	} catch (std::runtime_error& e) {
		rofcore::logging::error << "[cipcore] neigh_in6 create: exception " << e.what() << std::endl;
	}
}


void
cipcore::neigh_in6_updated(unsigned int ifindex, uint16_t nbindex)
{
	try {

		if (not has_link_by_ifindex(ifindex)) {
			rofcore::logging::error << "[cipcore] neigh_in6 create: link ifindex not found: " << ifindex << std::endl;
			return;
		}

		if (not get_link_by_ifindex(ifindex).has_neigh_in6(nbindex)) {
			set_link_by_ifindex(ifindex).set_neigh_in6(nbindex);
			rofcore::logging::debug << "[cipcore][neigh_in6_updated] state:" << std::endl << *this;
		}

	} catch (std::runtime_error& e) {
		rofcore::logging::error << "[cipcore] neigh_in6 create: exception " << e.what() << std::endl;
	}
}


void
cipcore::neigh_in6_deleted(unsigned int ifindex, uint16_t nbindex)
{
	try {

		if (not has_link_by_ifindex(ifindex)) {
			rofcore::logging::error << "[cipcore] neigh_in6 delete: link ifindex not found: " << ifindex << std::endl;
			return;
		}

		if (get_link_by_ifindex(ifindex).has_neigh_in6(nbindex)) {
			set_link_by_ifindex(ifindex).drop_neigh_in6(nbindex);
			rofcore::logging::debug << "[cipcore][neigh_in6_deleted] state:" << std::endl << *this;
		}

	} catch (std::runtime_error& e) {
		rofcore::logging::error << "[cipcore] neigh_in6 delete: exception " << e.what() << std::endl;
	}
}


void
cipcore::purge_dpt_entries()
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);

		// all wildcard matches with non-strict deletion => removes all state from data path
		rofl::openflow::cofflowmod fe(dpt.get_version());
		fe.set_command(rofl::openflow::OFPFC_DELETE);
		fe.set_table_id(rofl::openflow::base::get_ofptt_all(dpt.get_version()));
		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

	} catch (rofl::eRofDptNotFound& e) {

	}
}



void
cipcore::block_stp_frames()
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);

		rofl::openflow::cofflowmod fe(dpt.get_version());

		fe.set_command(rofl::openflow::OFPFC_ADD);
		fe.set_table_id(0);
		fe.set_match().set_eth_dst(rofl::cmacaddr("01:80:c2:00:00:00"), rofl::cmacaddr("ff:ff:ff:00:00:00"));

		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

	} catch (rofl::eRofDptNotFound& e) {

	}
}



void
cipcore::unblock_stp_frames()
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);

		rofl::openflow::cofflowmod fe(dpt.get_version());

		fe.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		fe.set_table_id(0);
		fe.set_match().set_eth_dst(rofl::cmacaddr("01:80:c2:00:00:00"), rofl::cmacaddr("ff:ff:ff:00:00:00"));

		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

	} catch (rofl::eRofDptNotFound& e) {

	}
}



void
cipcore::redirect_ipv4_multicast()
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);

		rofl::openflow::cofflowmod fe(dpt.get_version());

		fe.set_command(rofl::openflow::OFPFC_ADD);
		fe.set_table_id(0);

		fe.set_match().set_eth_type(rofl::fipv4frame::IPV4_ETHER);
		fe.set_match().set_ipv4_dst(rofl::caddress_in4("224.0.0.0"), rofl::caddress_in4("240.0.0.0"));

		rofl::cindex index(0);

		fe.set_instructions().add_inst_apply_actions().set_actions().add_action_output(index).
				set_port_no(rofl::openflow::base::get_ofpp_controller_port(dpt.get_version()));
		fe.set_instructions().set_inst_apply_actions().set_actions().set_action_output(index).
				set_max_len(ETH_FRAME_LEN);

		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

	} catch (rofl::eRofDptNotFound& e) {

	}
}



void
cipcore::redirect_ipv6_multicast()
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);

		rofl::openflow::cofflowmod fe(dpt.get_version());

		fe.set_command(rofl::openflow::OFPFC_ADD);
		fe.set_table_id(0);

		fe.set_match().set_eth_type(rofl::fipv6frame::IPV6_ETHER);
		fe.set_match().set_ipv6_dst(rofl::caddress_in6("ff00::"), rofl::caddress_in6("ff00::"));

		rofl::cindex index(0);

		fe.set_instructions().add_inst_apply_actions().set_actions().add_action_output(index).
				set_port_no(rofl::openflow::base::get_ofpp_controller_port(dpt.get_version()));
		fe.set_instructions().set_inst_apply_actions().set_actions().set_action_output(index).
				set_max_len(ETH_FRAME_LEN);

		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

	} catch (rofl::eRofDptNotFound& e) {

	}
}



void
cipcore::hook_dpt_attach()
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);

		std::vector<std::string> argv;
		std::vector<std::string> envp;

		std::stringstream s_dpid;
		s_dpid << "DPID=" << dpt.get_dpid();
		envp.push_back(s_dpid.str());

		cipcore::execute(cipcore::script_path_dpt_open, argv, envp);

	} catch (rofl::eRofDptNotFound& e) {

	}
}



void
cipcore::hook_dpt_detach()
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);

		std::vector<std::string> argv;
		std::vector<std::string> envp;

		std::stringstream s_dpid;
		s_dpid << "DPID=" << dpt.get_dpid();
		envp.push_back(s_dpid.str());

		cipcore::execute(cipcore::script_path_dpt_close, argv, envp);

	} catch (rofl::eRofDptNotFound& e) {

	}
}



void
cipcore::hook_port_up(std::string const& devname)
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);

		std::vector<std::string> argv;
		std::vector<std::string> envp;

		std::stringstream s_dpid;
		s_dpid << "DPID=" << dpt.get_dpid();
		envp.push_back(s_dpid.str());

		std::stringstream s_devname;
		s_devname << "DEVNAME=" << devname;
		envp.push_back(s_devname.str());

		cipcore::execute(cipcore::script_path_port_up, argv, envp);

	} catch (rofl::eRofDptNotFound& e) {

	}
}



void
cipcore::hook_port_down(std::string const& devname)
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);

		std::vector<std::string> argv;
		std::vector<std::string> envp;

		std::stringstream s_dpid;
		s_dpid << "DPID=" << dpt.get_dpid();
		envp.push_back(s_dpid.str());

		std::stringstream s_devname;
		s_devname << "DEVNAME=" << devname;
		envp.push_back(s_devname.str());

		cipcore::execute(cipcore::script_path_port_down, argv, envp);

	} catch (rofl::eRofDptNotFound& e) {

	}
}



void
cipcore::execute(
		std::string const& executable,
		std::vector<std::string> argv,
		std::vector<std::string> envp)
{
	pid_t pid = 0;

	if ((pid = fork()) < 0) {
		fprintf(stderr, "syscall error fork(): %d (%s)\n", errno, strerror(errno));
		return;
	}

	if (pid > 0) { // father process
		return;
	}


	// child process

	std::vector<const char*> vctargv;
	for (std::vector<std::string>::iterator
			it = argv.begin(); it != argv.end(); ++it) {
		vctargv.push_back((*it).c_str());
	}
	vctargv.push_back(NULL);


	std::vector<const char*> vctenvp;
	for (std::vector<std::string>::iterator
			it = envp.begin(); it != envp.end(); ++it) {
		vctenvp.push_back((*it).c_str());
	}
	vctenvp.push_back(NULL);


	execvpe(executable.c_str(), (char*const*)&vctargv[0], (char*const*)&vctenvp[0]);

	exit(1); // just in case execvpe fails
}




