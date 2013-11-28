#include "ethcore.h"

#include <inttypes.h>

using namespace ethercore;

ethcore::ethcore(uint8_t table_id, uint16_t default_vid) :
		table_id(table_id),
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

	/* we create a single default VLAN and add all ports in an untagged mode */
	cfib *default_fib = new cfib(this, dpt->get_dpid(), default_vid, table_id+1);

	logging::info << "dpath attaching: " << *dpt << std::endl;

	for (std::map<uint32_t, rofl::cofport*>::iterator
			it = dpt->get_ports().begin(); it != dpt->get_ports().end(); ++it) {

		rofl::cofport* port = it->second;
		try {
			sport *sp = new sport(this, dpt->get_dpid(), port->get_port_no(), default_vid, table_id);
			logging::info << "   adding port " << *sp << std::endl;

			default_fib->add_port(port->get_port_no(), /*untagged=*/false);

		} catch (eSportExists& e) {
			logging::warn << "unable to add port:" << port->get_name() << ", already exists " << std::endl;
		}
	}
}



void
ethcore::handle_dpath_close(
		cofdpt *dpt)
{
	logging::info << "dpath detaching: " << *dpt << std::endl;

	delete cfib::get_fib(dpt->get_dpid());

	// destroy all sports associated with dpt
	sport::destroy_sports(dpt->get_dpid());
}



void
ethcore::handle_packet_in(
		cofdpt *dpt,
		cofmsg_packet_in *msg)
{
	try {
		cmacaddr eth_src = msg->get_packet().ether()->get_dl_src();
		cmacaddr eth_dst = msg->get_packet().ether()->get_dl_dst();

		/*
		 * sanity check: if source mac is multicast => invalid frame
		 */
		if (eth_src.is_multicast()) {
			delete msg; return;
		}

		/*
		 * get sport instance for in-port
		 */
		sport& port = sport::get_sport(dpt->get_dpid(), msg->get_match().get_in_port());

		/*
		 * block mac address 01:80:c2:00:00:00
		 */
		if (msg->get_packet().ether()->get_dl_dst() == cmacaddr("01:80:c2:00:00:00") ||
			msg->get_packet().ether()->get_dl_dst() == cmacaddr("01:00:5e:00:00:fb")) {
			cflowentry fe(dpt->get_version());

			fe.set_command(OFPFC_ADD);
			fe.set_buffer_id(msg->get_buffer_id());
			fe.set_idle_timeout(15);
			fe.set_table_id(msg->get_table_id());

			fe.match.set_in_port(msg->get_match().get_in_port());
			fe.match.set_eth_dst(msg->get_packet().ether()->get_dl_dst());
			fe.instructions.next() = cofinst_apply_actions(dpt->get_version());

			logging::info << "ethercore: installing FLOW-MOD with entry: " << fe << std::endl;

			send_flow_mod_message(dpt, fe);

			delete msg; return;
		}

		logging::info << "ethercore: PACKET-IN from dpid:" << (int)dpt->get_dpid() << " message: " << *msg << std::endl;

		cofaclist actions;

		cfib::get_fib(dpt->get_dpid()).fib_update(
								this,
								dpt,
								msg->get_packet().ether()->get_dl_src(),
								msg->get_match().get_in_port());

		if (eth_dst.is_multicast()) {

			actions.next() = cofaction_output(dpt->get_version(), OFPP12_FLOOD);

		} else {

			cfibentry& entry = cfib::get_fib(dpt->get_dpid()).fib_lookup(
						this,
						dpt,
						msg->get_packet().ether()->get_dl_dst(),
						msg->get_packet().ether()->get_dl_src(),
						msg->get_match().get_in_port());

			if (msg->get_match().get_in_port() == entry.get_out_port_no()) {
				delete msg; return;
			}

			actions.next() = cofaction_output(dpt->get_version(), entry.get_out_port_no());
		}



		if (OFP_NO_BUFFER != msg->get_buffer_id()) {
			send_packet_out_message(dpt, msg->get_buffer_id(), msg->get_match().get_in_port(), actions);
		} else {
			send_packet_out_message(dpt, msg->get_buffer_id(), msg->get_match().get_in_port(), actions,
					msg->get_packet().soframe(), msg->get_packet().framelen());
		}

	} catch (eOFmatchNotFound& e) {

	}

	delete msg; return;
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

}


void
ethcore::drop_vlan(
		uint64_t dpid,
		uint16_t vid)
{

}


void
ethcore::add_port_to_vlan(
		uint64_t dpid,
		uint32_t portno,
		uint16_t vid,
		bool tagged = true)
{

}


void
ethcore::drop_port_from_vlan(
		uint64_t dpid,
		uint32_t portno,
		uint16_t vid)
{

}


