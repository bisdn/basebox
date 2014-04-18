/*
 * cfib.cc
 *
 *  Created on: 15.07.2013
 *      Author: andreas
 */

#include "cfib.h"

using namespace ethercore;

std::map<uint64_t, std::map<uint16_t, cfib*> > cfib::fibs;


/*static*/
cfib&
cfib::get_fib(uint64_t dpid, uint16_t vid)
{
	if (cfib::fibs[dpid].find(vid) == cfib::fibs[dpid].end()) {
		throw eFibNotFound();
	}
	return *(cfib::fibs[dpid][vid]);
}


/*static*/
void
cfib::destroy_fibs(uint64_t dpid)
{
	while (not cfib::fibs[dpid].empty()) {
		for (std::map<uint16_t, cfib*>::iterator
				it = cfib::fibs[dpid].begin(); it != cfib::fibs[dpid].end(); ++it) {
			logging::debug << "dumping FIB[1]: " << *(it->second) << std::endl;
		}
		delete (cfib::fibs[dpid].begin()->second);
		for (std::map<uint16_t, cfib*>::iterator
				it = cfib::fibs[dpid].begin(); it != cfib::fibs[dpid].end(); ++it) {
			logging::debug << "dumping FIB[2]: " << *(it->second) << std::endl;
		}
	}
	cfib::fibs[dpid].clear();
}


/*static*/
void
cfib::dump_fibs()
{
	for (std::map<uint64_t, std::map<uint16_t, cfib*> >::iterator
			it = cfib::fibs.begin(); it != cfib::fibs.end(); ++it) {
		rofl::logging::info << "[cfib][dump] dpid:" << (unsigned long long)it->first << std::endl;
		for (std::map<uint16_t, cfib*>::iterator
				jt = it->second.begin(); jt != it->second.end(); ++jt) {
			cfib& fib = *(jt->second);
			rofl::indent i(2);
			std::cout << fib;
		}
	}
}


cfib::cfib(
		cfib_owner *fibowner,
		uint64_t dpid,
		uint16_t vid,
		uint8_t src_stage_table_id,
		uint8_t dst_stage_table_id,
		unsigned int timer_dump_interval) :
		fibowner(fibowner),
		dpid(dpid),
		vid(vid),
		src_stage_table_id(src_stage_table_id),
		dst_stage_table_id(dst_stage_table_id),
		timer_dump_interval(timer_dump_interval)
{
	if (NULL == fibowner) {
		throw eFibInval();
	}
	if (cfib::fibs[dpid].find(vid) != cfib::fibs[dpid].end()) {
		throw eFibExists();
	}
	cfib::fibs[dpid][vid] = this;

	flood_group_id = fibowner->get_idle_group_id();

	block_stp();

	add_group_entry_flood(/*modify=*/false);

	add_flow_mod_flood();

	add_flow_mod_in_stage();

	//register_timer(CFIB_TIMER_DUMP, timer_dump_interval);
}



cfib::~cfib()
{
	while (not fibtable.empty()) {
		delete (fibtable.begin()->second);
		fibtable.erase(fibtable.begin());
	}
	fibtable.clear();

	cfib::fibs[dpid].erase(vid);

	drop_flow_mod_in_stage();

	drop_flow_mod_flood();

	drop_group_entry_flood();

	fibowner->release_group_id(flood_group_id);
}


void
cfib::add_port(
		uint32_t portno,
		bool tagged)
{
	try {
		sport& port = sport::get_sport(dpid, portno);

		/* notify sport about new VLAN membership => setup of associated FlowTable and GroupTable entries */
		port.add_membership(vid, tagged);

		ports.insert(portno);

		add_group_entry_flood(/*modify=*/true);

	} catch (eSportNotFound& e) {
		logging::error << "cfib::add_port() sport instance for OF portno:" << (int)portno << " not found" << std::endl;
	} catch (eSportFailed& e) {
		logging::error << "cfib::add_port() adding membership to sport instance failed" << std::endl;
	}
}


void
cfib::drop_port(
		uint32_t portno)
{
	try {
		sport& port = sport::get_sport(dpid, portno);

		/* notify sport about new VLAN membership => setup of associated FlowTable and GroupTable entries */
		port.drop_membership(vid);

		ports.erase(portno);

		add_group_entry_flood(/*modify=*/true);

	} catch (eSportNotFound& e) {
		logging::error << "cfib::drop_port() sport instance for OF portno:" << (int)portno << " not found" << std::endl;
	} catch (eSportFailed& e) {
		logging::error << "cfib::drop_port() dropping membership from sport instance failed" << std::endl;
	}
}


void
cfib::reset()
{
	for (std::map<rofl::cmacaddr, cfibentry*>::iterator
			it = fibtable.begin(); it != fibtable.end(); ++it) {
		delete (it->second);
	}
	fibtable.clear();
}



void
cfib::handle_timeout(
			int opaque, void *data)
{
	switch (opaque) {
	case CFIB_TIMER_DUMP: {
		register_timer(CFIB_TIMER_DUMP, timer_dump_interval);
		logging::info << std::endl << *this;
	} break;
	default: {

	};
	}
}



void
cfib::fib_timer_expired(cfibentry *entry)
{
	if (fibtable.find(entry->get_lladdr()) == fibtable.end()) {
		return;
	}

	rofl::indent i(2);

	logging::info << "[ethcore][fib] FIB entry expired:" << std::endl << *entry;

	fibtable.erase(entry->get_lladdr());

	delete entry;

	logging::info << "[ethcore][fib] Forwarding Information Base:" << std::endl << *this;
}



uint32_t
cfib::get_flood_group_id(uint16_t vid)
{
	return flood_group_id;
}



void
cfib::add_flow_mod_in_stage()
{
	rofl::crofbase *rofbase = fibowner->get_rofbase();
	rofl::crofdpt *dpt = rofbase->dpt_find(dpid);

	rofl::openflow::cofflowmod fe(dpt->get_version());

	fe.set_command(rofl::openflow::OFPFC_ADD);
	fe.set_table_id(src_stage_table_id);
	fe.set_priority(0x4000);
	//fe.set_idle_timeout(0);
	//fe.set_hard_timeout(0);

	fe.match.set_vlan_vid(vid);
#if 1
	fe.instructions.add_inst_apply_actions().get_actions().append_action_output(
			rofl::openflow::base::get_ofpp_controller_port(dpt->get_version()));
#endif
	fe.instructions.add_inst_goto_table().set_table_id(dst_stage_table_id);

	dpt->send_flow_mod_message(fe);
}



void
cfib::drop_flow_mod_in_stage()
{
	try {
		rofl::crofbase *rofbase = fibowner->get_rofbase();
		rofl::crofdpt *dpt = rofbase->dpt_find(dpid);

		if ((NULL == dpt) || (rofl::openflow::OFP_VERSION_UNKNOWN == dpt->get_version())) {
			return;
		}

		rofl::openflow::cofflowmod fe(dpt->get_version());

		fe.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		fe.set_table_id(src_stage_table_id);
		fe.set_priority(0x4000);
		//fe.set_idle_timeout(0);
		//fe.set_hard_timeout(0);

		fe.match.set_vlan_vid(vid);

		dpt->send_flow_mod_message(fe);

	} catch (rofl::eBadVersion& e) {
		logging::error << "[ethcore][cfib] data path already disconnected, unable to remove our entries" << std::endl;
	}
}



void
cfib::add_flow_mod_flood()
{
	rofl::crofbase *rofbase = fibowner->get_rofbase();
	rofl::crofdpt *dpt = rofbase->dpt_find(dpid);

	rofl::openflow::cofflowmod fe(dpt->get_version());

	fe.set_command(rofl::openflow::OFPFC_ADD);
	fe.set_table_id(dst_stage_table_id);
	fe.set_priority(0x4000);
	//fe.set_idle_timeout(0);
	//fe.set_hard_timeout(0);

	fe.match.set_vlan_vid(vid);

	fe.instructions.add_inst_apply_actions().get_actions().append_action_group(flood_group_id);

	dpt->send_flow_mod_message(fe);
}



void
cfib::drop_flow_mod_flood()
{
	try {
		rofl::crofbase *rofbase = fibowner->get_rofbase();
		rofl::crofdpt *dpt = rofbase->dpt_find(dpid);

		if ((NULL == dpt) || (rofl::openflow::OFP_VERSION_UNKNOWN == dpt->get_version())) {
			return;
		}

		rofl::openflow::cofflowmod fe(dpt->get_version());

		fe.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		fe.set_table_id(dst_stage_table_id);
		fe.set_priority(0x4000);
		//fe.set_idle_timeout(0);
		//fe.set_hard_timeout(0);

		fe.match.set_vlan_vid(vid);

		dpt->send_flow_mod_message(fe);

	} catch (rofl::eBadVersion& e) {
		logging::error << "[ethcore][cfib] data path already disconnected, unable to remove our entries" << std::endl;
	}
}



void
cfib::add_group_entry_flood(
		bool modify)
{
	try {
		rofl::crofbase *rofbase = fibowner->get_rofbase();
		rofl::crofdpt *dpt = rofbase->dpt_find(dpid);

		rofl::openflow::cofgroupmod ge(dpt->get_version());

		if (modify)
			ge.set_command(rofl::openflow::OFPGC_MODIFY);
		else
			ge.set_command(rofl::openflow::OFPGC_ADD);

		ge.set_type(rofl::openflow::OFPGT_ALL);
		ge.set_group_id(flood_group_id);

		/* what about group chaining here? this would be useful ... */
		for (std::set<uint32_t>::iterator
				it = ports.begin(); it != ports.end(); ++it) {
			sport& port = sport::get_sport(dpid, *it);

			ge.set_buckets().set_bucket(0).set_watch_group(0);
			ge.set_buckets().set_bucket(0).set_watch_port(0);
			ge.set_buckets().set_bucket(0).set_weight(0);

			// only pop vlan, when vid is used untagged on this port, i.e. vid == port.pvid
			try {
				if (port.get_pvid() == vid) {
					ge.set_buckets().set_bucket(0).set_actions().append_action_pop_vlan();
				}
			} catch (eSportNoPvid& e) {}
			ge.set_buckets().set_bucket(0).set_actions().append_action_output(port.get_portno());
		}

		dpt->send_group_mod_message(ge);

	} catch (...) {

	}
}



void
cfib::drop_group_entry_flood()
{
	try {
		rofl::crofbase *rofbase = fibowner->get_rofbase();
		rofl::crofdpt *dpt = rofbase->dpt_find(dpid);

		if ((NULL == dpt) || (rofl::openflow::OFP_VERSION_UNKNOWN == dpt->get_version())) {
			return;
		}

		rofl::openflow::cofgroupmod ge(dpt->get_version());

		ge.set_command(rofl::openflow::OFPGC_DELETE);
		ge.set_type(rofl::openflow::OFPGT_ALL);	// necessary for OFPGC_DELETE?
		ge.set_group_id(flood_group_id);

		dpt->send_group_mod_message(ge);

	} catch (...) {

	}
}



void
cfib::handle_packet_in(rofl::openflow::cofmsg_packet_in& msg)
{
	try {
		/* get sport instance for in-port */
		uint32_t inport = msg.get_match().get_in_port();
		sport& port = sport::get_sport(dpid, inport);

		/* get eth-src and eth-dst */
		rofl::cmacaddr eth_src = msg.set_packet().ether()->get_dl_src();
		rofl::cmacaddr eth_dst = msg.set_packet().ether()->get_dl_dst();

		logging::info << "[ethcore][fib] frame seen: " << eth_src << " => " << eth_dst << " on vid:" << std::dec << (int)vid << std::endl;

		/* sanity check: if source mac is multicast => invalid frame */
		if (eth_src.is_multicast()) {
			return;
		}

		/* no FIB entry so far, create new one */
		if (fibtable.find(eth_src) == fibtable.end()) {

#if 1
			fibtable[eth_src] = new cfibentry(this,
					src_stage_table_id,
					dst_stage_table_id,
					eth_src,
					dpid,
					vid, inport, port.get_is_tagged(vid));
#endif
			rofl::indent i(2);
			logging::debug << "[ethcore][fib] creating new FIB entry:" << std::endl << *(fibtable[eth_src]);
			logging::info << "[ethcore][fib] state changed:" << std::endl << *this;


		/* either inport has changed or the FlowMod entry was removed => in either case, refresh entry */
		} else if (inport != fibtable[eth_src]->get_portno()) {
			fibtable[eth_src]->set_portno(inport);

			rofl::indent i(2);
			logging::debug << "[ethcore][fib] updating FIB entry:" << std::endl << (*fibtable[eth_src]);
			logging::info << "[ethcore][fib] state changed:" << std::endl << *this;
		}

		cfib::dump_fibs();

		/* Packet-In generated by table containing eth-src checks => i.e. source host is unknown so far */
		if (msg.get_table_id() == src_stage_table_id) {

		}
		/* Packet-In generated by table containing eth-dst entries => i.e. destination is not known yet (or entry has expired), flood it */
		else if (msg.get_table_id() == dst_stage_table_id) {
			// flood packet

			/* This should actually never happen, as we have a FlowMod entry in table
			 * dst_stage_table_id that forwards a packet destined to an unknown destination
			 * to the flooding-group-table-entry of this vlan.
			 */

			rofl::crofbase *rofbase = fibowner->get_rofbase();
			rofl::crofdpt *dpt = rofbase->dpt_find(dpid);

			rofl::openflow::cofactions actions(dpt->get_version());
			actions.append_action_group(flood_group_id);

			if (rofl::openflow::OFP_NO_BUFFER == msg.get_buffer_id()) {
				dpt->send_packet_out_message(msg.get_buffer_id(), msg.get_in_port(), actions, msg.get_packet().soframe(), msg.get_packet().framelen());
			} else {
				dpt->send_packet_out_message(msg.get_buffer_id(), msg.get_in_port(), actions);
			}
		}



	} catch (rofl::openflow::eOFmatchNotFound& e) {
		logging::info << "[ethcore][fib] dropping frame, unable to find portno in packet-in" << std::endl;
	} catch (eSportNotFound& e) {
		logging::info << "[ethcore][fib] dropping frame, unable to find sport instance for packet-in" << std::endl;
	} catch (eSportNotMember& e) {
		logging::info << "[ethcore][fib] dropping frame, missing port membership" << std::endl;
	}
}



void
cfib::handle_flow_removed(
		rofl::openflow::cofmsg_flow_removed& msg)
{
	try {
		/* get sport instance for in-port */
		uint32_t inport = msg.get_match().get_in_port();
		sport& port = sport::get_sport(dpid, inport);

		/* get eth-src and eth-dst */
		rofl::cmacaddr eth_src = msg.get_match().get_eth_src();

		if (fibtable.find(eth_src) == fibtable.end()) {
			logging::warn << "[ethcore][fib][flow-removed] inactivity indication received for unknown station, eth-src:" << eth_src << std::endl;
			return;
		}

		logging::debug << "[ethcore][fib][flow-removed] inactivity idle-timeout seen, eth-src:" << eth_src << std::endl;

		delete fibtable[eth_src];

		fibtable.erase(eth_src);

	} catch (rofl::openflow::eOFmatchNotFound& e) {
		logging::info << "[ethcore][fib] dropping frame, unable to find portno in packet-in" << std::endl;
	} catch (eSportNotFound& e) {
		logging::info << "[ethcore][fib] dropping frame, unable to find sport instance for packet-in" << std::endl;
	} catch (eSportNotMember& e) {
		logging::info << "[ethcore][fib] dropping frame, missing port membership" << std::endl;
	}
}



void
cfib::block_stp()
{
	/*
	 * block mac address 01:80:c2:00:00:00
	 */
	rofl::crofbase *rofbase = fibowner->get_rofbase();
	rofl::crofdpt *dpt = rofbase->dpt_find(dpid);

	if ((NULL == dpt) || (rofl::openflow::OFP_VERSION_UNKNOWN == dpt->get_version())) {
		return;
	}

	rofl::openflow::cofflowmod fe(dpt->get_version());

	fe.set_command(rofl::openflow::OFPFC_ADD);
	fe.set_priority(0xffff);
	fe.set_idle_timeout(0);
	fe.set_hard_timeout(0);
	fe.set_table_id(0);

	fe.match.set_eth_dst(rofl::cmacaddr("01:80:c2:00:00:00"));

	fe.instructions.add_inst_apply_actions();

	//logging::info << "ethercore: installing FLOW-MOD with entry: " << fe << std::endl;

	dpt->send_flow_mod_message(fe);
}



