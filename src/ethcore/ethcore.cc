#include "ethcore.h"

#include <inttypes.h>

using namespace ethercore;

ethcore* ethcore::sethcore = (ethcore*)0;


ethcore&
ethcore::get_instance()
{
	if (0 == ethcore::sethcore) {
		ethcore::sethcore =
				new ethcore();
	}
	return *(ethcore::sethcore);
}


ethcore::ethcore() :
		port_stage_table_id(0),
		fib_in_stage_table_id(1),
		fib_out_stage_table_id(2),
		default_vid(1),
		timer_dump_interval(DEFAULT_TIMER_DUMP_INTERVAL)
{
	register_timer(ETHSWITCH_TIMER_DUMP, timer_dump_interval);
}


void
ethcore::init(
		uint8_t port_stage_table_id,
		uint8_t fib_in_stage_table_id,
		uint8_t fib_out_stage_table_id,
		uint16_t default_vid)
{
	this->port_stage_table_id 		= port_stage_table_id;
	this->fib_in_stage_table_id 	= fib_in_stage_table_id;
	this->fib_out_stage_table_id 	= fib_out_stage_table_id;
	this->default_vid 				= default_vid;
}



ethcore::~ethcore()
{
	// ...
}



void
ethcore::handle_timeout(int opaque)
{
	switch (opaque) {
	case ETHSWITCH_TIMER_DUMP: {
		logging::debug << *this << std::endl;
		register_timer(ETHSWITCH_TIMER_DUMP, timer_dump_interval);
	} break;
	}
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
	logging::info << "[ethcore] dpath attaching dpid: " << (unsigned long long)dpt->get_dpid() << std::endl;

	/* we create a single default VLAN and add all ports in an untagged mode */
	add_vlan(dpt->get_dpid(), default_vid);

	// for testing
#if 0
	add_vlan(dpt->get_dpid(), 20);
#endif


	for (std::map<uint32_t, rofl::cofport*>::iterator
			it = dpt->get_ports().begin(); it != dpt->get_ports().end(); ++it) {

		rofl::cofport* port = it->second;
		try {
			sport *sp = new sport(this, dpt->get_dpid(), port->get_port_no(), port->get_name(), port_stage_table_id);
			logging::info << "[ethcore] adding port " << *sp << std::endl;

			cfib::get_fib(dpt->get_dpid(), default_vid).add_port(port->get_port_no(), /*untagged=*/false);

#if 0
			// for testing
			if (port->get_port_no() == 1) {
				cfib::get_fib(dpt->get_dpid(), default_vid).add_port(port->get_port_no(), /*untagged=*/false);
				cfib::get_fib(dpt->get_dpid(), 20).add_port(port->get_port_no(), /*tagged=*/true);
			}
			if (port->get_port_no() == 2) {
				cfib::get_fib(dpt->get_dpid(), 20).add_port(port->get_port_no(), /*untagged=*/false);
			}
#endif
		} catch (eSportExists& e) {
			logging::warn << "unable to add port:" << port->get_name() << ", already exists " << std::endl;
		}
	}
}



void
ethcore::handle_dpath_close(
		cofdpt *dpt)
{
	logging::info << "[ethcore] dpath detaching dpid: " << (unsigned long long)dpt->get_dpid() << std::endl;

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
			vid = msg->get_match().get_vlan_vid() & ~OFPVID_PRESENT;
		} catch (eOFmatchNotFound& e) {
			/* no VLAN VID OXM TLV => frame was most likely untagged */
			vid = sport::get_sport(dpt->get_dpid(), msg->get_match().get_in_port()).get_pvid() & ~OFPVID_PRESENT;
		}

		//logging::debug << "matching VID ===> " << (int)(msg->get_match().get_vlan_vid() & ~OFPVID_PRESENT) << std::endl;
		//logging::debug << "PVID ===> " << (int)(sport::get_sport(dpt->get_dpid(), msg->get_match().get_in_port()).get_pvid() & ~OFPVID_PRESENT) << std::endl;
		//logging::debug << "final VID ===> " << (int)vid << std::endl;

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
		logging::crit << "[ethcore] no PVID for sport instance found for Packet-In message" << *msg << std::endl;

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
		logging::info << "[ethcore] adding vid:" << std::dec << (int)vid << " to dpid:" << (unsigned long long)dpid << std::endl;
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
		logging::info << "[ethcore] dropping vid:" << std::dec << (int)vid << " from dpid:" << (unsigned long long)dpid << std::endl;
		delete &(cfib::get_fib(dpid, vid));
	} catch (eFibNotFound& e) {
		logging::warn << "[ethcore] dropping vid:" << (int)vid << " from dpid:" << (unsigned long long)dpid << " failed" << std::endl;
	}
}


void
ethcore::add_port_to_vlan(
		uint64_t dpid,
		std::string const& devname,
		uint16_t vid,
		bool tagged)
{
	try {
		logging::info << "[ethcore] adding port:" << devname << " to vid:" << (int)vid << " on dpid:" << (unsigned long long)dpid << std::endl;
		uint32_t portno = sport::get_sport(dpid, devname).get_portno();
		cfib::get_fib(dpid, vid).add_port(portno, tagged);
	} catch (eSportNotFound& e) {
		logging::warn << "[ethcore] adding port:" << devname << " to vid:" << (int)vid << " on dpid:" << (unsigned long long)dpid << " failed (no such port)" << std::endl;
	} catch (eFibNotFound& e) {
		logging::warn << "[ethcore] adding port:" << devname << " to vid:" << (int)vid << " on dpid:" << (unsigned long long)dpid << " failed (no such vlan)" << std::endl;
	}
}


void
ethcore::drop_port_from_vlan(
		uint64_t dpid,
		std::string const& devname,
		uint16_t vid)
{
	try {
		logging::info << "[ethcore] dropping port:" << devname << " from vid:" << (int)vid << " on dpid:" << (unsigned long long)dpid << std::endl;
		uint32_t portno = sport::get_sport(dpid, devname).get_portno();
		cfib::get_fib(dpid, vid).drop_port(portno);
	} catch (eSportNotFound& e) {
		logging::warn << "[ethcore] dropping port:" << devname << " from vid:" << (int)vid << " on dpid:" << (unsigned long long)dpid << " failed (no such port)" << std::endl;
	} catch (eFibNotFound& e) {
		logging::warn << "[ethcore] dropping port:" << devname << " from vid:" << (int)vid << " on dpid:" << (unsigned long long)dpid << " failed (no such vlan)" << std::endl;
	}
}


