#include "ethcore.h"

#include <inttypes.h>

using namespace ethercore;

ethcore::ethcore(
		uint8_t port_stage_table_id,
		uint8_t fib_in_stage_table_id,
		uint8_t fib_out_stage_table_id,
		uint16_t default_vid) :
		port_stage_table_id(port_stage_table_id),
		fib_in_stage_table_id(fib_in_stage_table_id),
		fib_out_stage_table_id(fib_out_stage_table_id),
		default_vid(default_vid)
{

}



ethcore::~ethcore()
{
	// ...
}



void
ethcore::handle_timeout(int opaque)
{

}




void
ethcore::request_flow_stats()
{
#if 0
	std::map<cofdpt*, std::map<uint16_t, std::map<cmacaddr, struct fibentry_t> > >::iterator it;

	for (it = fib.begin(); it != fib.end(); ++it) {
		cofdpt *dpt = it->first;

		cofflow_stats_request req;

		switch (dpt->get_version()) {
		case OFP10_VERSION: {
			req.set_version(dpt->get_version());
			req.set_table_id(OFPTT_ALL);
			req.set_match(cofmatch(OFP10_VERSION));
			req.set_out_port(OFPP_ANY);
		} break;
		case OFP12_VERSION: {
			req.set_version(dpt->get_version());
			req.set_table_id(OFPTT_ALL);
			cofmatch match(OFP12_VERSION);
			//match.set_eth_dst(cmacaddr("01:80:c2:00:00:00"));
			req.set_match(match);
			req.set_out_port(OFPP_ANY);
			req.set_out_group(OFPG_ANY);
			req.set_cookie(0);
			req.set_cookie_mask(0);
		} break;
		default: {
			// do nothing
		} break;
		}

		fprintf(stderr, "ethercore: calling FLOW-STATS-REQUEST for dpid: 0x%"PRIu64"\n",
				dpt->get_dpid());

		send_flow_stats_request(dpt, /*flags=*/0, req);
	}

	register_timer(ETHSWITCH_TIMER_FLOW_STATS, flow_stats_timeout);
#endif
}



void
ethcore::handle_flow_stats_reply(cofdpt *dpt, cofmsg_flow_stats_reply *msg)
{
#if 0
	if (fib.find(dpt) == fib.end()) {
		delete msg; return;
	}

	std::vector<cofflow_stats_reply>& replies = msg->get_flow_stats();

	std::vector<cofflow_stats_reply>::iterator it;
	for (it = replies.begin(); it != replies.end(); ++it) {
		switch (it->get_version()) {
		case OFP10_VERSION: {
			fprintf(stderr, "FLOW-STATS-REPLY:\n  match: %s\n  actions: %s\n",
					it->get_match().c_str(), it->get_actions().c_str());
		} break;
		case OFP12_VERSION: {
			fprintf(stderr, "FLOW-STATS-REPLY:\n  match: %s\n  instructions: %s\n",
					it->get_match().c_str(), it->get_instructions().c_str());
		} break;
		default: {
			// do nothing
		} break;
		}
	}
#endif
	delete msg;
}





void
ethcore::handle_dpath_open(
		cofdpt *dpt)
{
#if 0
	rofl::cflowentry fe(dpt->get_version());

	fe.set_command(OFPFC_ADD);
	fe.set_table_id(0);

	fe.instructions.next() = cofinst_apply_actions(dpt->get_version());
	fe.instructions.back().actions.next() = cofaction_output(dpt->get_version(), OFPP12_CONTROLLER);

	fe.match.set_eth_type(farpv4frame::ARPV4_ETHER);
	send_flow_mod_message(dpt, fe);
#endif


	logging::info << "dpath attaching dpid: " << (unsigned long long)dpt->get_dpid() << std::endl;

	/* we create a single default VLAN and add all ports in an untagged mode */
	add_vlan(dpt->get_dpid(), default_vid);



	for (std::map<uint32_t, rofl::cofport*>::iterator
			it = dpt->get_ports().begin(); it != dpt->get_ports().end(); ++it) {

		rofl::cofport* port = it->second;
		try {
			sport *sp = new sport(this, dpt->get_dpid(), port->get_port_no(), port_stage_table_id);
			logging::info << "   adding port " << *sp << std::endl;

			cfib::get_fib(dpt->get_dpid(), default_vid).add_port(port->get_port_no(), /*untagged=*/false);

		} catch (eSportExists& e) {
			logging::warn << "unable to add port:" << port->get_name() << ", already exists " << std::endl;
		}
	}
}



void
ethcore::handle_dpath_close(
		cofdpt *dpt)
{
	logging::info << "dpath detaching dpid: " << (unsigned long long)dpt->get_dpid() << std::endl;

	cfib::destroy_fibs(dpt->get_dpid());

	// destroy all sports associated with dpt
	sport::destroy_sports(dpt->get_dpid());
}



void
ethcore::handle_packet_in(
		cofdpt *dpt,
		cofmsg_packet_in *msg)
{
	uint16_t vid = 0xffff;
	try {

		try {
			/* check for VLAN VID OXM TLV in Packet-In message */
			vid = msg->get_match().get_vlan_vid();
		} catch (eOFmatchNotFound& e) {
			/* no VLAN VID OXM TLV => frame was most likely untagged */
			vid = sport::get_sport(dpt->get_dpid(), msg->get_match().get_in_port()).get_pvid();
		}

		cfib::get_fib(dpt->get_dpid(), vid).handle_packet_in(*msg);

	} catch (eFibNotFound& e) {
		// no FIB for specified VLAN found
		logging::warn << "[ethcore] no VLAN vid:" << (int)vid << " configured on dpid:" << (unsigned long long)dpt->get_dpid() << std::endl;
	} catch (eOFmatchNotFound& e) {
		// OXM-TLV in-port not found
		logging::warn << "[ethcore] no in-port found in Packet-In message" << *msg << std::endl;
	} catch (eSportNotFound& e) {
		// sport instance not found? critical error!
		logging::crit << "[ethcore] no sport instance for in-port found in Packet-In message" << *msg << std::endl;
	} catch (eSportNoPvid& e) {
		// drop frame on data path
		rofl::cofaclist actions(dpt->get_version());
		send_packet_out_message(dpt, msg->get_buffer_id(), msg->get_match().get_in_port(), actions);
	}

	delete msg;
}


void
ethcore::handle_port_status(cofdpt *dpt, cofmsg_port_status *msg)
{
	// TODO

	switch (msg->get_reason()) {
	case OFPPR_ADD: {

	} break;
	case OFPPR_MODIFY: {

	} break;
	case OFPPR_DELETE: {

	} break;
	}
	delete msg;
}


uint32_t
ethcore::get_idle_group_id()
{
	uint32_t group_id = 0;
	while (group_ids.find(group_id) != group_ids.end()) {
		group_id++;
	}
	group_ids.insert(group_id);
	return group_id;
}


void
ethcore::release_group_id(uint32_t group_id)
{
	if (group_ids.find(group_id) != group_ids.end()) {
		group_ids.erase(group_id);
	}
}


void
ethcore::add_vlan(
		uint64_t dpid,
		uint16_t vid)
{
	try {
		logging::info << "[ethcore] adding vid:" << (int)vid << " to dpid:" << (unsigned long long)dpid << std::endl;
		new cfib(this, dpid, vid, fib_in_stage_table_id, fib_out_stage_table_id);
	} catch (eFibExists& e) {
		logging::warn << "[ethcore] adding vid:" << (int)vid << " to dpid:" << (unsigned long long)dpid << " failed" << std::endl;
	}
}


void
ethcore::drop_vlan(
		uint64_t dpid,
		uint16_t vid)
{
	try {
		logging::info << "[ethcore] dropping vid:" << (int)vid << " from dpid:" << (unsigned long long)dpid << std::endl;
		delete &(cfib::get_fib(dpid, vid));
	} catch (eFibNotFound& e) {
		logging::warn << "[ethcore] dropping vid:" << (int)vid << " from dpid:" << (unsigned long long)dpid << " failed" << std::endl;
	}
}


void
ethcore::add_port_to_vlan(
		uint64_t dpid,
		uint32_t portno,
		uint16_t vid,
		bool tagged)
{
	try {
		logging::info << "[ethcore] adding port:" << (int)portno << " to vid:" << (int)vid << " on dpid:" << (unsigned long long)dpid << std::endl;
		cfib::get_fib(dpid, vid).add_port(portno, tagged);
	} catch (eFibNotFound& e) {
		logging::warn << "[ethcore] adding port:" << (int)portno << " to vid:" << (int)vid << " on dpid:" << (unsigned long long)dpid << " failed" << std::endl;
	}
}


void
ethcore::drop_port_from_vlan(
		uint64_t dpid,
		uint32_t portno,
		uint16_t vid)
{
	try {
		logging::info << "[ethcore] dropping port:" << (int)portno << " to vid:" << (int)vid << " on dpid:" << (unsigned long long)dpid << std::endl;
		cfib::get_fib(dpid, vid).drop_port(portno);
	} catch (eFibNotFound& e) {
		logging::warn << "[ethcore] dropping port:" << (int)portno << " to vid:" << (int)vid << " on dpid:" << (unsigned long long)dpid << " failed" << std::endl;
	}
}


