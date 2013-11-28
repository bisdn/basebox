/*
 * cfib.cc
 *
 *  Created on: 15.07.2013
 *      Author: andreas
 */

#include <cfib.h>

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
		delete (cfib::fibs[dpid].begin()->second);
	}
}


cfib::cfib(cfib_owner *fibowner, uint64_t dpid, uint16_t vid, uint8_t table_id) :
		fibowner(fibowner),
		dpid(dpid),
		vid(vid),
		table_id(table_id)
{
	if (NULL == fibowner) {
		throw eFibInval();
	}
	if (cfib::fibs[dpid].find(vid) != cfib::fibs[dpid].end()) {
		throw eFibExists();
	}
	cfib::fibs[dpid][vid] = this;

	flood_group_id = fibowner->get_idle_group_id();

	add_group_entry_flood(/*modify=*/false);

	add_flow_mod_flood();
}



cfib::~cfib()
{
	cfib::fibs[dpid].erase(vid);

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
cfib::fib_timer_expired(cfibentry *entry)
{
	if (fibtable.find(entry->get_lladdr()) != fibtable.end()) {
#if 0
		entry->set_out_port_no(OFPP12_FLOOD);
#else
		fibtable[entry->get_lladdr()]->flow_mod_delete();
		fibtable.erase(entry->get_lladdr());
		delete entry;
#endif
	}

	std::cerr << "EXPIRED: " << *this << std::endl;
}



uint32_t
cfib::get_flood_group_id(uint16_t vid)
{
	return 0;
}



void
cfib::add_flow_mod_flood()
{
	rofl::crofbase *rofbase = fibowner->get_rofbase();
	rofl::cofdpt *dpt = rofbase->dpt_find(dpid);

	rofl::cflowentry fe(dpt->get_version());

	fe.set_command(OFPFC_ADD);
	fe.set_table_id(table_id+1);
	fe.set_priority(0x4000);
	fe.set_idle_timeout(0);
	fe.set_hard_timeout(0);

	fe.match.set_vlan_vid(rofl::coxmatch_ofb_vlan_vid::VLAN_TAG_MODE_NORMAL, vid);
	fe.instructions.next() = rofl::cofinst_apply_actions(dpt->get_version());
	fe.instructions.back().actions.next() = rofl::cofaction_group(dpt->get_version(), flood_group_id);

	rofbase->send_flow_mod_message(dpt, fe);
}



void
cfib::drop_flow_mod_flood()
{
	rofl::crofbase *rofbase = fibowner->get_rofbase();
	rofl::cofdpt *dpt = rofbase->dpt_find(dpid);

	rofl::cflowentry fe(dpt->get_version());

	fe.set_command(OFPFC_DELETE_STRICT);
	fe.set_table_id(table_id+1);
	fe.set_priority(0x4000);
	fe.set_idle_timeout(0);
	fe.set_hard_timeout(0);

	fe.match.set_vlan_vid(rofl::coxmatch_ofb_vlan_vid::VLAN_TAG_MODE_NORMAL, vid);
	fe.instructions.next() = rofl::cofinst_apply_actions(dpt->get_version());
	fe.instructions.back().actions.next() = rofl::cofaction_group(dpt->get_version(), flood_group_id);

	rofbase->send_flow_mod_message(dpt, fe);
}



void
cfib::add_group_entry_flood(
		bool modify)
{
	try {
		rofl::crofbase *rofbase = fibowner->get_rofbase();
		rofl::cofdpt *dpt = rofbase->dpt_find(dpid);

		rofl::cgroupentry ge(dpt->get_version());

		if (modify)
			ge.set_command(OFPGC_MODIFY);
		else
			ge.set_command(OFPGC_ADD);

		ge.set_type(OFPGT_ALL);
		ge.set_group_id(flood_group_id);

		/* what about group chaining here? this would be useful ... */
		for (std::set<uint32_t>::iterator
				it = ports.begin(); it != ports.end(); ++it) {
			sport& port = sport::get_sport(dpid, *it);


			ge.buckets.next() = rofl::cofbucket(dpt->get_version(), 0, 0, 0);
			// only pop vlan, when vid is used untagged on this port, i.e. vid == port.pvid
			try {
				if (port.get_pvid() == vid) {
					ge.buckets.back().actions.next() = rofl::cofaction_pop_vlan(dpt->get_version());
				}
			} catch (eSportNoPvid& e) {}
			ge.buckets.back().actions.next() = rofl::cofaction_output(dpt->get_version(), port.get_portno());
		}

		rofbase->send_group_mod_message(dpt, ge);

	} catch (...) {

	}
}



void
cfib::drop_group_entry_flood()
{
	try {
		rofl::crofbase *rofbase = fibowner->get_rofbase();
		rofl::cofdpt *dpt = rofbase->dpt_find(dpid);

		rofl::cgroupentry ge(dpt->get_version());

		ge.set_command(OFPGC_DELETE);
		ge.set_type(OFPGT_ALL);	// necessary for OFPGC_DELETE?
		ge.set_group_id(flood_group_id);

		rofbase->send_group_mod_message(dpt, ge);

	} catch (...) {

	}
}



void
cfib::handle_packet_in(rofl::cofmsg_packet_in& msg)
{
	try {
		/* get sport instance for in-port */
		uint32_t inport = msg.get_match().get_in_port();
		sport& port = sport::get_sport(dpid, inport);

		/* get eth-src and eth-dst */
		rofl::cmacaddr eth_src = msg.get_packet().ether()->get_dl_src();
		rofl::cmacaddr eth_dst = msg.get_packet().ether()->get_dl_dst();


		/* sanity check: if source mac is multicast => invalid frame */
		if (eth_src.is_multicast()) {
			return;
		}


		/* Packet-In generated by table containing eth-src checks => i.e. source host is unknown so far */
		if (msg.get_table_id() == table_id) {
			fib_update(eth_src, inport);

		}
		/* Packet-In generated by table containing eth-dst entries => i.e. destination is not known yet (or entry has expired), flood it */
		else if (msg.get_table_id() == (table_id + 1)) {


		}


#if 0
		/*
		 * block mac address 01:80:c2:00:00:00
		 */
		if (msg.get_packet().ether()->get_dl_dst() == rofl::cmacaddr("01:80:c2:00:00:00") ||
			msg.get_packet().ether()->get_dl_dst() == rofl::cmacaddr("01:00:5e:00:00:fb")) {
			rofl::cflowentry fe(dpt->get_version());

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
								msg->get_packet().ether()->get_dl_src(),
								msg->get_match().get_in_port());

		if (eth_dst.is_multicast()) {

			actions.next() = cofaction_output(dpt->get_version(), OFPP12_FLOOD);

		} else {

			cfibentry& entry = cfib::get_fib(dpt->get_dpid()).fib_lookup(
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
#endif
	} catch (rofl::eOFmatchNotFound& e) {

	}
}



void
cfib::fib_update(
		rofl::cmacaddr const& src,
		uint32_t in_port)
{
	std::cerr << "UPDATE: src: " << src << std::endl;

	uint8_t table_id = 0;
	uint16_t vid = 0xffff;


	// update cfibentry for src/inport
	if (fibtable.find(src) == fibtable.end()) {
		fibtable[src] = new cfibentry(this, table_id, src, dpid, vid, in_port, /*tagged=*/true);
		fibtable[src]->flow_mod_add();

		std::cerr << "UPDATE[2.1]: " << *this << std::endl;

	} else {
		fibtable[src]->set_out_port_no(in_port);

		std::cerr << "UPDATE[3.1]: " << *this << std::endl;

	}
}



cfibentry&
cfib::fib_lookup(
		rofl::cmacaddr const& dst,
		rofl::cmacaddr const& src,
		uint32_t in_port)
{
	std::cerr << "LOOKUP: dst: " << dst << " src: " << src << std::endl;

	// sanity checks
	if (src.is_multicast()) {
		throw eFibInval();
	}

	fib_update(src, in_port);

	if (dst.is_multicast()) {
		throw eFibInval();
	}

	// find out-port for dst
	if (fibtable.find(dst) == fibtable.end()) {

		uint8_t table_id = 0;
		uint16_t vid = 0xffff;

		fibtable[dst] = new cfibentry(this, table_id, dst, dpid, vid, in_port, OFPP12_FLOOD);
		fibtable[dst]->flow_mod_add();

		std::cerr << "LOOKUP[1]: " << *this << std::endl;
	}

	return *(fibtable[dst]);
}





