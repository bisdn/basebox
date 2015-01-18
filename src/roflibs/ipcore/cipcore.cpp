#include "cipcore.hpp"
#include "roflibs/ethcore/cethcore.hpp"

using namespace roflibs::ip;

/*static*/std::map<rofl::cdpid, cipcore*> cipcore::ipcores;
/*static*/const std::string cipcore::DEFAULT_CONFIG_FILE("/usr/local/etc/roflibs.conf");

void
cipcore::handle_dpt_open(rofl::crofdpt& dpt)
{
	state = STATE_ATTACHED;

	dpid = dpt.get_dpid();

	//purge_dpt_entries();
	//block_stp_frames();
	redirect_ipv4_multicast();
	redirect_ipv6_multicast();
	set_forwarding(true);

	for (std::map<int, clink*>::iterator
			it = links.begin(); it != links.end(); ++it) {
		it->second->handle_dpt_open(dpt);
	}
	for (std::map<unsigned int, croutetable>::iterator
			it = rtables.begin(); it != rtables.end(); ++it) {
		it->second.handle_dpt_open(dpt);
	}
}



void
cipcore::handle_dpt_close(rofl::crofdpt& dpt)
{
	if (dpid != dpt.get_dpid()) {
		return;
	}

	state = STATE_DETACHED;

	for (std::map<unsigned int, croutetable>::iterator
			it = rtables.begin(); it != rtables.end(); ++it) {
		it->second.handle_dpt_close(dpt);
	}
	for (std::map<int, clink*>::iterator
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

	if (dpt.get_dpid() != dpid) {
		rofcore::logging::debug << "[roflibs][ipcore] received PortStatus from invalid data path, ignoring" << std::endl;
		return;
	}

	rofcore::logging::debug << "[roflibs][ipcore] Port-Status message rcvd:" << std::endl << msg;

	// nothing to do, link creation/destruction is seen via netlink interface
}



void
cipcore::handle_packet_in(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_packet_in& msg)
{
	rofcore::logging::debug << "[roflibs][ipcore][handle_packet_in] pkt received: " << std::endl << msg;
	// store packet in ethcore and thus, tap devices
	roflibs::eth::cethcore::set_eth_core(dpt.get_dptid()).handle_packet_in(dpt, auxid, msg);
}



void
cipcore::handle_error_message(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_error& msg)
{
	rofcore::logging::warn << "[roflibs][ipcore] error message rcvd:" << std::endl << msg;
}



void
cipcore::handle_flow_removed(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_flow_removed& msg)
{
	rofcore::logging::info << "[roflibs][ipcore] Flow-Removed message rcvd:" << std::endl << msg;
}



void
cipcore::set_forwarding(bool forward)
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dpid);

		rofl::openflow::cofflowmod fed = rofl::openflow::cofflowmod(dpt.get_version_negotiated());
		if (forward == true) {
			fed.set_command(rofl::openflow::OFPFC_MODIFY_STRICT);
		} else {
			fed.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		}
		// local ip stage
		fed.set_table_id(local_ofp_table_id);
		fed.set_idle_timeout(0);
		fed.set_hard_timeout(0);
		fed.set_priority(0); // lowest priority
		fed.set_cookie(cookie_fwd_local);
		fed.set_instructions().clear();
		fed.set_instructions().add_inst_goto_table().set_table_id(out_ofp_table_id);
		dpt.send_flow_mod_message(rofl::cauxid(0), fed);

		// local ip stage+1
		fed.set_cookie(cookie_app);
		fed.set_table_id(local_ofp_table_id+1);
		fed.set_instructions().clear();
		fed.set_instructions().set_inst_apply_actions().set_actions().
				add_action_output(rofl::cindex(0)).set_port_no(rofl::openflow::OFPP_CONTROLLER);
		fed.set_instructions().set_inst_apply_actions().set_actions().
				set_action_output(rofl::cindex(0)).set_max_len(1526);
		dpt.send_flow_mod_message(rofl::cauxid(0), fed);

		// fwd ip stage
		fed.set_cookie(cookie_no_route);
		fed.set_table_id(out_ofp_table_id);
		fed.set_instructions().clear();
		fed.set_instructions().set_inst_apply_actions().set_actions().
				add_action_output(rofl::cindex(0)).set_port_no(rofl::openflow::OFPP_CONTROLLER);
		fed.set_instructions().set_inst_apply_actions().set_actions().
				set_action_output(rofl::cindex(0)).set_max_len(1526);
		dpt.send_flow_mod_message(rofl::cauxid(0), fed);

	} catch (rofl::eSocketTxAgain& e) {

		rofcore::logging::debug << "[roflibs][ipcore] enable forwarding: control channel congested, rescheduling (TODO)" << std::endl;

	} catch (rofl::eRofDptNotFound& e) {

		rofcore::logging::debug << "[roflibs][ipcore] enable forwarding: dpid not found: " << dpid << std::endl;

	}
}


void
cipcore::link_created(unsigned int ifindex)
{
	try {
		const rofcore::crtlink& rtl = rofcore::cnetlink::get_instance().get_links().get_link(ifindex);

		std::string devname = rtl.get_devname();
		uint16_t vid = 1;
		bool tagged = false;

		const std::string dbname("file");
		roflibs::eth::cportdb& portdb = roflibs::eth::cportdb::get_portdb(dbname);

		if (not portdb.has_eth_entry(dpid, devname)) {
			return;
		}

		vid = portdb.get_eth_entry(dpid, devname).get_port_vid();

		set_link(ifindex, rtl.get_devname(), rtl.get_hwaddr(), tagged, vid);
		rofcore::logging::debug << "[roflibs][ipcore][link_created] state:" << std::endl << *this;

	} catch (rofcore::crtlink::eRtLinkNotFound& e) {
		// should (:) never happen
	}
}


void
cipcore::link_updated(unsigned int ifindex)
{
	// do nothing for now
	rofcore::logging::debug << "[roflibs][ipcore][link_updated] state:" << std::endl << *this;
}


void
cipcore::link_deleted(unsigned int ifindex)
{
	drop_link(ifindex);
	rofcore::logging::debug << "[roflibs][ipcore][link_deleted] state:" << std::endl << *this;
}


void
cipcore::addr_in4_created(unsigned int ifindex, uint16_t adindex)
{
	try {
		if (not has_link(ifindex)) {
			rofcore::logging::debug << "[roflibs][ipcore][addr_in4_created] ignoring ifindex:" << ifindex << std::endl << *this;
			return; // ignore address events for unknown interfaces
		}

		if (not get_link(ifindex).has_addr_in4(adindex)) {
			set_link(ifindex).set_addr_in4(adindex);
			rofcore::logging::debug << "[roflibs][ipcore][addr_in4_created] state:" << std::endl << *this;
		}

	} catch (std::runtime_error& e) {
		rofcore::logging::error << "[roflibs][ipcore] addr_in4 create: exception " << e.what() << std::endl;
	}
}

void
cipcore::addr_in4_updated(unsigned int ifindex, uint16_t adindex)
{
	try {

		if (not has_link(ifindex)) {
			rofcore::logging::debug << "[roflibs][ipcore][addr_in4_updated] ignoring ifindex:" << ifindex << std::endl << *this;
			return; // ignore address events for unknown interfaces
		}

		if (not get_link(ifindex).has_addr_in4(adindex)) {
			set_link(ifindex).set_addr_in4(adindex);
			rofcore::logging::debug << "[roflibs][ipcore][addr_in4_updated] state:" << std::endl << *this;
		}

	} catch (std::runtime_error& e) {
		rofcore::logging::error << "[roflibs][ipcore] addr_in4 update: exception " << e.what() << std::endl;
	}
}

void
cipcore::addr_in4_deleted(unsigned int ifindex, uint16_t adindex)
{
	try {

		if (not has_link(ifindex)) {
			rofcore::logging::debug << "[roflibs][ipcore][addr_in4_deleted] ignoring ifindex:" << ifindex << std::endl << *this;
			return; // ignore address events for unknown interfaces
		}

		if (get_link(ifindex).has_addr_in4(adindex)) {
			set_link(ifindex).drop_addr_in4(adindex);
			rofcore::logging::debug << "[roflibs][ipcore][addr_in4_deleted] state:" << std::endl << *this;
		}

	} catch (std::runtime_error& e) {
		rofcore::logging::error << "[roflibs][ipcore] addr_in4 delete: exception " << e.what() << std::endl;
	}
}

void
cipcore::addr_in6_created(unsigned int ifindex, uint16_t adindex)
{
	try {

		if (not has_link(ifindex)) {
			rofcore::logging::debug << "[roflibs][ipcore][addr_in6_created] ignoring ifindex:" << ifindex << std::endl << *this;
			return; // ignore address events for unknown interfaces
		}

		if (not get_link(ifindex).has_addr_in6(adindex)) {
			set_link(ifindex).set_addr_in6(adindex);
			rofcore::logging::debug << "[roflibs][ipcore][addr_in6_created] state:" << std::endl << *this;
		}

	} catch (std::runtime_error& e) {
		rofcore::logging::error << "[roflibs][ipcore] addr_in6 create: exception " << e.what() << std::endl;
	}
}

void
cipcore::addr_in6_updated(unsigned int ifindex, uint16_t adindex)
{
	try {

		if (not has_link(ifindex)) {
			rofcore::logging::debug << "[roflibs][ipcore][addr_in6_updated] ignoring ifindex:" << ifindex << std::endl << *this;
			return; // ignore address events for unknown interfaces
		}

		if (not get_link(ifindex).has_addr_in6(adindex)) {
			set_link(ifindex).set_addr_in6(adindex);
			rofcore::logging::debug << "[roflibs][ipcore][addr_in6_updated] state:" << std::endl << *this;
		}

	} catch (std::runtime_error& e) {
		rofcore::logging::error << "[roflibs][ipcore] addr_in6 update: exception " << e.what() << std::endl;
	}
}

void
cipcore::addr_in6_deleted(unsigned int ifindex, uint16_t adindex)
{
	try {

		if (not has_link(ifindex)) {
			rofcore::logging::debug << "[roflibs][ipcore][addr_in6_deleted] ignoring ifindex:" << ifindex << std::endl << *this;
			return; // ignore address events for unknown interfaces
		}

		if (get_link(ifindex).has_addr_in6(adindex)) {
			set_link(ifindex).drop_addr_in6(adindex);
			rofcore::logging::debug << "[roflibs][ipcore][addr_in6_deleted] state:" << std::endl << *this;
		}

	} catch (std::runtime_error& e) {
		rofcore::logging::error << "[roflibs][ipcore] addr_in6 delete: exception " << e.what() << std::endl;
	}
}


void
cipcore::route_in4_created(uint8_t table_id, unsigned int rtindex)
{
	try {
		// ignore local route table and unspecified table_id
		if ((RT_TABLE_LOCAL/*255*/ == table_id) || (RT_TABLE_UNSPEC/*0*/ == table_id)) {
		//if ((RT_TABLE_UNSPEC/*0*/ == table_id)) {
			rofcore::logging::debug << "[roflibs][ipcore] create route => suppressing table_id=" << (unsigned int)table_id << std::endl;
			return;
		}

		rofcore::logging::info << "[roflibs][ipcore] crtroute CREATE:" << std::endl << rofcore::cnetlink::get_instance().get_routes_in4(table_id).get_route(rtindex);

		if ((not has_table(table_id)) || (not get_table(table_id).has_route_in4(rtindex))) {
			set_table(table_id).add_route_in4(rtindex);
			rofcore::logging::debug << "[roflibs][ipcore][route_in4_created] state:" << std::endl << *this;
		}

	} catch (rofcore::eNetLinkNotFound& e) {
		rofcore::logging::warn << "[roflibs][ipcore] crtroute CREATE notification rcvd => "
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
			rofcore::logging::debug << "[roflibs][ipcore] route update => suppressing table_id=" << (unsigned int)table_id << std::endl;
			return;
		}

		rofcore::logging::info << "[roflibs][ipcore] route update:" << std::endl << rofcore::cnetlink::get_instance().get_routes_in4(table_id).get_route(rtindex);

		if ((not has_table(table_id)) || (not get_table(table_id).has_route_in4(rtindex))) {
			set_table(table_id).add_route_in4(rtindex);
			rofcore::logging::debug << "[roflibs][ipcore][route_in4_updated] state:" << std::endl << *this;
		}

		// do nothing here, this event is handled directly by dptroute instance

	} catch (rofcore::eNetLinkNotFound& e) {
		rofcore::logging::warn << "[roflibs][ipcore] crtroute UPDATE notification rcvd => "
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
			rofcore::logging::debug << "[roflibs][ipcore] route delete => suppressing table_id=" << (unsigned int)table_id << std::endl;
			return;
		}

		rofcore::logging::info << "[roflibs][ipcore] crtroute DELETE:" << std::endl << rofcore::cnetlink::get_instance().get_routes_in4(table_id).get_route(rtindex);

		if (has_table(table_id) && get_table(table_id).has_route_in4(rtindex)) {
			set_table(table_id).drop_route_in4(rtindex);
			rofcore::logging::debug << "[roflibs][ipcore][route_in4_deleted] state:" << std::endl << *this;
		}

	} catch (rofcore::eNetLinkNotFound& e) {
		rofcore::logging::warn << "[roflibs][ipcore] crtroute DELETE notification rcvd => "
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
			rofcore::logging::debug << "ipcore::route_created() => suppressing table_id=" << (unsigned int)table_id << std::endl;
			return;
		}

		rofcore::logging::info << "[roflibs][ipcore] crtroute CREATE:" << std::endl << rofcore::cnetlink::get_instance().get_routes_in6(table_id).get_route(rtindex);

		if ((not has_table(table_id)) || (not get_table(table_id).has_route_in6(rtindex))) {
			set_table(table_id).add_route_in6(rtindex);
			rofcore::logging::debug << "[roflibs][ipcore][route_in6_created] state:" << std::endl << *this;
		}

	} catch (rofcore::eNetLinkNotFound& e) {
		rofcore::logging::warn << "[roflibs][ipcore] crtroute CREATE notification rcvd => "
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
			rofcore::logging::debug << "[roflibs][ipcore] route update => suppressing table_id=" << (unsigned int)table_id << std::endl;
			return;
		}

		rofcore::logging::info << "[roflibs][ipcore] route update:" << std::endl << rofcore::cnetlink::get_instance().get_routes_in6(table_id).get_route(rtindex);

		if ((not has_table(table_id)) || (not get_table(table_id).has_route_in6(rtindex))) {
			set_table(table_id).add_route_in6(rtindex);
			rofcore::logging::debug << "[roflibs][ipcore][route_in6_updated] state:" << std::endl << *this;
		}

		// do nothing here, this event is handled directly by dptroute instance

	} catch (rofcore::eNetLinkNotFound& e) {
		rofcore::logging::warn << "[roflibs][ipcore] crtroute UPDATE notification rcvd => "
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
			rofcore::logging::debug << "ipcore::route_deleted() => suppressing table_id=" << (unsigned int)table_id << std::endl;
			return;
		}

		rofcore::logging::info << "[roflibs][ipcore] crtroute DELETE:" << std::endl << rofcore::cnetlink::get_instance().get_routes_in6(table_id).get_route(rtindex);

		if (has_table(table_id) && get_table(table_id).has_route_in6(rtindex)) {
			set_table(table_id).drop_route_in6(rtindex);
			rofcore::logging::debug << "[roflibs][ipcore][route_in6_deleted] state:" << std::endl << *this;
		}

	} catch (rofcore::eNetLinkNotFound& e) {
		rofcore::logging::warn << "[roflibs][ipcore] crtroute DELETE notification rcvd => "
				<< "unable to find crtroute for table-id:" << table_id << " rtindex:" << rtindex << std::endl;
	}
}


void
cipcore::neigh_in4_created(unsigned int ifindex, uint16_t nbindex)
{
	try {

		if (not has_link(ifindex)) {
			rofcore::logging::error << "[roflibs][ipcore] neigh_in4 create: link ifindex not found: " << ifindex << std::endl;
			return;
		}

		if (not get_link(ifindex).has_neigh_in4(nbindex)) {
			set_link(ifindex).set_neigh_in4(nbindex);
			rofcore::logging::debug << "[roflibs][ipcore][neigh_in4_created] state:" << std::endl << *this;
		} else {
			set_link(ifindex).set_neigh_in4(nbindex).update();
			rofcore::logging::debug << "[roflibs][ipcore][neigh_in4_created] state:" << std::endl << *this;
		}

	} catch (std::runtime_error& e) {
		rofcore::logging::error << "[roflibs][ipcore] neigh_in4 create: exception " << e.what() << std::endl;
	}
}


void
cipcore::neigh_in4_updated(unsigned int ifindex, uint16_t nbindex)
{
	try {

		if (not has_link(ifindex)) {
			rofcore::logging::error << "[roflibs][ipcore] neigh_in4 update: link ifindex not found: " << ifindex << std::endl;
			return;
		}

		if (not get_link(ifindex).has_neigh_in4(nbindex)) {
			set_link(ifindex).set_neigh_in4(nbindex);
			rofcore::logging::debug << "[roflibs][ipcore][neigh_in4_updated] state:" << std::endl << *this;
		} else {
			set_link(ifindex).set_neigh_in4(nbindex).update();
			rofcore::logging::debug << "[roflibs][ipcore][neigh_in4_updated] state:" << std::endl << *this;
		}

	} catch (std::runtime_error& e) {
		rofcore::logging::error << "[roflibs][ipcore] neigh_in4 update: exception " << e.what() << std::endl;
	}
}


void
cipcore::neigh_in4_deleted(unsigned int ifindex, uint16_t nbindex)
{
	try {

		if (not has_link(ifindex)) {
			rofcore::logging::error << "[roflibs][ipcore] neigh_in4 delete: link ifindex not found: " << ifindex << std::endl;
			return;
		}

		if (get_link(ifindex).has_neigh_in4(nbindex)) {
			set_link(ifindex).drop_neigh_in4(nbindex);
			rofcore::logging::debug << "[roflibs][ipcore][neigh_in4_deleted] state:" << std::endl << *this;
		}

	} catch (std::runtime_error& e) {
		rofcore::logging::error << "[roflibs][ipcore] neigh_in4 delete: exception " << e.what() << std::endl;
	}
}


void
cipcore::neigh_in6_created(unsigned int ifindex, uint16_t nbindex)
{
	try {

		if (not has_link(ifindex)) {
			rofcore::logging::error << "[roflibs][ipcore] neigh_in6 create: link ifindex not found: " << ifindex << std::endl;
			return;
		}

		if (not get_link(ifindex).has_neigh_in6(nbindex)) {
			set_link(ifindex).set_neigh_in6(nbindex);
			rofcore::logging::debug << "[roflibs][ipcore][neigh_in6_created] state:" << std::endl << *this;
		} else {
			set_link(ifindex).set_neigh_in6(nbindex).update();
			rofcore::logging::debug << "[roflibs][ipcore][neigh_in6_created] state:" << std::endl << *this;
		}

	} catch (std::runtime_error& e) {
		rofcore::logging::error << "[roflibs][ipcore] neigh_in6 create: exception " << e.what() << std::endl;
	}
}


void
cipcore::neigh_in6_updated(unsigned int ifindex, uint16_t nbindex)
{
	try {

		if (not has_link(ifindex)) {
			rofcore::logging::error << "[roflibs][ipcore] neigh_in6 create: link ifindex not found: " << ifindex << std::endl;
			return;
		}

		if (not get_link(ifindex).has_neigh_in6(nbindex)) {
			set_link(ifindex).set_neigh_in6(nbindex);
			rofcore::logging::debug << "[roflibs][ipcore][neigh_in6_updated] state:" << std::endl << *this;
		} else {
			set_link(ifindex).set_neigh_in6(nbindex).update();
			rofcore::logging::debug << "[roflibs][ipcore][neigh_in6_created] state:" << std::endl << *this;
		}

	} catch (std::runtime_error& e) {
		rofcore::logging::error << "[roflibs][ipcore] neigh_in6 create: exception " << e.what() << std::endl;
	}
}


void
cipcore::neigh_in6_deleted(unsigned int ifindex, uint16_t nbindex)
{
	try {

		if (not has_link(ifindex)) {
			rofcore::logging::error << "[roflibs][ipcore] neigh_in6 delete: link ifindex not found: " << ifindex << std::endl;
			return;
		}

		if (get_link(ifindex).has_neigh_in6(nbindex)) {
			set_link(ifindex).drop_neigh_in6(nbindex);
			rofcore::logging::debug << "[roflibs][ipcore][neigh_in6_deleted] state:" << std::endl << *this;
		}

	} catch (std::runtime_error& e) {
		rofcore::logging::error << "[roflibs][ipcore] neigh_in6 delete: exception " << e.what() << std::endl;
	}
}


void
cipcore::purge_dpt_entries()
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dpid);

		// all wildcard matches with non-strict deletion => removes all state from data path
		rofl::openflow::cofflowmod fe(dpt.get_version_negotiated());
		fe.set_command(rofl::openflow::OFPFC_DELETE);
		fe.set_table_id(rofl::openflow::base::get_ofptt_all(dpt.get_version_negotiated()));
		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

	} catch (rofl::eRofDptNotFound& e) {

	}
}



void
cipcore::redirect_ipv4_multicast()
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dpid);

		rofl::openflow::cofflowmod fe(dpt.get_version_negotiated());

		fe.set_command(rofl::openflow::OFPFC_ADD);
		fe.set_cookie(cookie_multicast_ipv4);
		fe.set_table_id(local_ofp_table_id);

		fe.set_match().set_eth_type(rofl::fipv4frame::IPV4_ETHER);
		fe.set_match().set_ipv4_dst(rofl::caddress_in4("224.0.0.0"), rofl::caddress_in4("240.0.0.0"));

		rofl::cindex index(0);

		fe.set_instructions().add_inst_apply_actions().set_actions().add_action_output(index).
				set_port_no(rofl::openflow::base::get_ofpp_controller_port(dpt.get_version_negotiated()));
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
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dpid);

		rofl::openflow::cofflowmod fe(dpt.get_version_negotiated());

		fe.set_command(rofl::openflow::OFPFC_ADD);
		fe.set_cookie(cookie_multicast_ipv6);
		fe.set_table_id(local_ofp_table_id);

		fe.set_match().set_eth_type(rofl::fipv6frame::IPV6_ETHER);
		fe.set_match().set_ipv6_dst(rofl::caddress_in6("ff00::"), rofl::caddress_in6("ff00::"));

		rofl::cindex index(0);

		fe.set_instructions().add_inst_apply_actions().set_actions().add_action_output(index).
				set_port_no(rofl::openflow::base::get_ofpp_controller_port(dpt.get_version_negotiated()));
		fe.set_instructions().set_inst_apply_actions().set_actions().set_action_output(index).
				set_max_len(ETH_FRAME_LEN);

		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

	} catch (rofl::eRofDptNotFound& e) {

	}
}


