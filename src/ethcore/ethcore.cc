#include "ethcore.h"

#include <inttypes.h>

using namespace ethercore;

ethcore* ethcore::sethcore = (ethcore*)0;


ethcore&
ethcore::get_instance(cofhello_elem_versionbitmap const& versionbitmap)
{
	if (0 == ethcore::sethcore) {
		ethcore::sethcore =
				new ethcore(versionbitmap);
	}
	return *(ethcore::sethcore);
}


ethcore::ethcore(cofhello_elem_versionbitmap const& versionbitmap) :
		rofl::crofbase(versionbitmap),
		dpid(0),
		port_stage_table_id(0),
		fib_in_stage_table_id(1),
		fib_out_stage_table_id(2),
		default_vid(1),
		timer_dump_interval(DEFAULT_TIMER_DUMP_INTERVAL),
		netlink_enabled(false)
{
	//register_timer(ETHSWITCH_TIMER_DUMP, timer_dump_interval);
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
ethcore::delete_all_ports()
{
	for (std::map<rofl::crofdpt*, std::map<uint32_t, dptlink*> >::iterator
			jt = dptlinks.begin(); jt != dptlinks.end(); ++jt) {
		for (std::map<uint32_t, dptlink*>::iterator
				it = dptlinks[jt->first].begin(); it != dptlinks[jt->first].end(); ++it) {
			delete (it->second); // remove dptport instance from heap
		}
	}
	dptlinks.clear();
}


void
ethcore::link_created(unsigned int ifindex)
{
	if (not netlink_enabled)
		return;

	std::string devbase;
	uint16_t vid(default_vid);

	try {
		// ge0.100 => vid 100 assigned to ge0
		std::string devname = cnetlink::get_instance().get_link(ifindex).get_devname();

		if (devname.find_first_of(".") == std::string::npos) {
			return;
		}

		devbase = devname.substr(0, devname.find_first_of("."));
		vid = atoi(devname.substr(devname.find_first_of(".") + 1).c_str());

		try {
			cfib::get_fib(dpid, vid);
		} catch (eFibNotFound& e) {
			add_vlan(dpid, vid);
		}

		rofl::logging::debug << "[ethcore][link-created] devbase:" << devbase << " vid:" << (int)vid << std::endl;

		sport::get_sport(dpid, devbase).get_membership(vid);

	} catch (eNetLinkNotFound& e) {

		/* interface index ifindex not found, skip this event */

	} catch (eSportNotFound& e) {

		/* device is not member of this data path, so skip this event */

	} catch (eSportNotMember& e) {

		/* device exists, but is not member yet of the new VLAN, add membership in mode tagged */
		add_port_to_vlan(dpid, devbase, vid, true);
	}
}


void
ethcore::link_updated(unsigned int ifindex)
{
	if (not netlink_enabled)
		return;

	std::string devbase;
	uint16_t vid(default_vid);

	try {
		// ge0.100 => vid 100 assigned to ge0
		std::string devname = cnetlink::get_instance().get_link(ifindex).get_devname();

		if (devname.find_first_of(".") == std::string::npos) {
			return;
		}

		devbase = devname.substr(0, devname.find_first_of("."));
		vid = atoi(devname.substr(devname.find_first_of(".") + 1).c_str());

		rofl::logging::debug << "[ethcore][link-updated] devbase:" << devbase << " vid:" << (int)vid << std::endl;

	} catch (eNetLinkNotFound& e) {

	}
}


void
ethcore::link_deleted(unsigned int ifindex)
{
	if (not netlink_enabled)
		return;

	std::string devbase;
	uint16_t vid(default_vid);

	try {
		// ge0.100 => vid 100 removed from ge0
		std::string devname = cnetlink::get_instance().get_link(ifindex).get_devname();

		if (devname.find_first_of(".") == std::string::npos) {
			return;
		}

		devbase = devname.substr(0, devname.find_first_of("."));
		vid = atoi(devname.substr(devname.find_first_of(".") + 1).c_str());

		rofl::logging::debug << "[ethcore][link-deleted] devbase:" << devbase << " vid:" << (int)vid << std::endl;

		drop_port_from_vlan(dpid, devbase, vid);

	} catch (eNetLinkNotFound& e) {

		/* interface index ifindex was not found, skip event */

	} catch (eSportNotFound& e) {

		/* device was not found, skip event */

	} catch (eSportNotMember& e) {

		/* device is not member of VLAN vid, skip event */

	}
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
	std::map<crofdpt*, std::map<uint16_t, std::map<cmacaddr, struct fibentry_t> > >::iterator it;

	for (it = fib.begin(); it != fib.end(); ++it) {
		crofdpt *dpt = it->first;

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
ethcore::handle_flow_stats_reply(crofdpt& dpt, cofmsg_flow_stats_reply& msg, uint8_t aux_id)
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
}





void
ethcore::handle_dpath_open(crofdpt& dpt)
{
	try {
		netlink_enabled = (bool)cconfig::get_instance().lookup("ethcored.enable_netlink");
	} catch (...) {}

	logging::info << "[ethcore] dpath attaching dpid:" << (unsigned long long)dpt.get_dpid() << std::endl;

	dpt.flow_mod_reset();

	dpt.group_mod_reset();

	try {
		default_vid = (int)cconfig::get_instance().lookup("ethcored."+dpt.get_dpid_s()+".default_vid");
	} catch (SettingNotFoundException& e) {
		logging::warn << "[ethcore] unable to find config file entry for " << std::string("ethcored."+dpt.get_dpid_s()+".default_vid") << std::endl;
	}

	/* we create a single default VLAN and add all ports in an untagged mode */
	add_vlan(dpt.get_dpid(), default_vid);

	dpid = dpt.get_dpid();

	/* create all VIDs defined for this datapath element */
	if (cconfig::get_instance().exists("ethcored."+dpt.get_dpid_s()+".vids")) {
		for (unsigned int i = 0; i < cconfig::get_instance().lookup("ethcored."+dpt.get_dpid_s()+".vids").getLength(); i++) {
			add_vlan(dpt.get_dpid(), (int)cconfig::get_instance().lookup("ethcored."+dpt.get_dpid_s()+".vids")[i]);
		}
	}


	/*
	 * iterate over all ports announced by data path, create sport instances and set VID memberships
	 */
	for (std::map<uint32_t, rofl::cofport*>::iterator
			it = dpt.get_ports().begin(); it != dpt.get_ports().end(); ++it) {

		rofl::cofport* port = it->second;
		try {
			sport *sp = new sport(this, dpt.get_dpid(), port->get_port_no(), port->get_name(), port_stage_table_id);
			logging::info << "[ethcore] adding port:" << std::endl << *sp;

			/* get VID memberships for port, if none exist, add port to default-vid */
			if (cconfig::get_instance().exists("ethcored."+dpt.get_dpid_s()+"."+port->get_name())) {

				/* add untagged PVID for port */
				std::string port_pvid("ethcored."+dpt.get_dpid_s()+"."+port->get_name()+".pvid");
				if (cconfig::get_instance().exists(port_pvid)) {
					add_port_to_vlan(dpt.get_dpid(), port->get_name(), (int)cconfig::get_instance().lookup(port_pvid), false);
				}

				/* add tagged memberships */
				std::string tagged_vids("ethcored."+dpt.get_dpid_s()+"."+port->get_name()+".tagged");
				if (cconfig::get_instance().exists(tagged_vids)) {
					for (unsigned int i = 0; i < cconfig::get_instance().lookup(tagged_vids).getLength(); i++) {
						add_port_to_vlan(dpt.get_dpid(), port->get_name(), (int)cconfig::get_instance().lookup(tagged_vids)[i], true);
					}
				}


			} else {
				/* add port to default VID */
				cfib::get_fib(dpt.get_dpid(), default_vid).add_port(port->get_port_no(), /*untagged=*/false);
			}



			/*
			 * create new tap device for port announced by (reconnecting?) data path
			 */
			if (netlink_enabled) {
				if (dptlinks[&dpt].find(port->get_port_no()) == dptlinks[&dpt].end()) {
					dptlinks[&dpt][port->get_port_no()] = new dptlink(this, &dpt, port->get_port_no());
				}
			}

		} catch (eSportExists& e) {
			logging::warn << "unable to add port:" << port->get_name() << ", already exists " << std::endl;
		}
	}
}



void
ethcore::handle_dpath_close(crofdpt& dpt)
{
	logging::info << "[ethcore] dpath detaching dpid:" << (unsigned long long)dpt.get_dpid() << std::endl;

	cfib::destroy_fibs(dpt.get_dpid());

	// destroy all sports associated with dpt
	sport::destroy_sports(dpt.get_dpid());

	delete_all_ports();
}



void
ethcore::handle_packet_in(crofdpt& dpt, cofmsg_packet_in& msg, uint8_t aux_id)
{
	//uint16_t vid = 0xffff;
	uint16_t vid = default_vid;
	try {

		try {
			/* check for VLAN VID OXM TLV in Packet-In message */
			vid = msg.get_match().get_vlan_vid() & ~OFPVID_PRESENT;
		} catch (eOFmatchNotFound& e) {
			/* no VLAN VID OXM TLV => frame was most likely untagged */
			vid = sport::get_sport(dpt.get_dpid(), msg.get_match().get_in_port()).get_pvid() & ~OFPVID_PRESENT;
		}

		//logging::debug << "matching VID ===> " << (int)(msg->get_match().get_vlan_vid() & ~OFPVID_PRESENT) << std::endl;
		//logging::debug << "PVID ===> " << (int)(sport::get_sport(dpt->get_dpid(), msg->get_match().get_in_port()).get_pvid() & ~OFPVID_PRESENT) << std::endl;
		//logging::debug << "final VID ===> " << (int)vid << std::endl;

		cfib::get_fib(dpt.get_dpid(), vid).handle_packet_in(msg);
		//logging::debug << "[ethcore] state changed:" << std::endl;
		//rofl::indent i(2);
		//logging::debug << cfib::get_fib(dpt.get_dpid(), vid);

	} catch (eFibNotFound& e) {
		// no FIB for specified VLAN found
		logging::warn << "[ethcore] no VLAN vid:" << (int)vid << " configured on dpid:" << (unsigned long long)dpt.get_dpid() << std::endl;

	} catch (eOFmatchNotFound& e) {
		// OXM-TLV in-port not found
		logging::warn << "[ethcore] no in-port found in Packet-In message" << msg << std::endl;

	} catch (eSportNotFound& e) {
		// sport instance not found? critical error!
		logging::crit << "[ethcore] no sport instance for in-port found in Packet-In message" << msg << std::endl;

	} catch (eSportNoPvid& e) {
		// drop frame on data path
		logging::crit << "[ethcore] no PVID for sport instance found for Packet-In message" << msg << std::endl;

		rofl::cofactions actions(dpt.get_version());
		dpt.send_packet_out_message(msg.get_buffer_id(), msg.get_match().get_in_port(), actions);
	}
}


void
ethcore::handle_port_status(crofdpt& dpt, cofmsg_port_status& msg, uint8_t aux_id)
{
	// TODO

	switch (msg.get_reason()) {
	case OFPPR_ADD: {
		try {
			sport *sp = new sport(this, dpt.get_dpid(), msg.get_port().get_port_no(), msg.get_port().get_name(), port_stage_table_id);
			logging::info << "[ethcore] adding port via Port-Status:" << std::endl << *sp;

			cfib::get_fib(dpt.get_dpid(), default_vid).add_port(msg.get_port().get_port_no(), /*untagged=*/false);
		} catch (eSportExists& e) {
			logging::warn << "[ethcore] unable to add port:" << msg.get_port().get_name() << ", already exists " << std::endl;
		}
	} break;
	case OFPPR_MODIFY: {
		logging::warn << "[ethcore] unhandled Port-Status MODIFY:" << std::endl << msg;
	} break;
	case OFPPR_DELETE: {
		try {
			sport& sp = sport::get_sport(dpt.get_dpid(), msg.get_port().get_port_no());
			logging::info << "[ethcore] dropping port via Port-Status:" << std::endl << sp;

			cfib::get_fib(dpt.get_dpid(), default_vid).drop_port(msg.get_port().get_port_no());

			delete &sp;
		} catch (eSportNotFound& e) {
			logging::warn << "[ethcore] unable to drop port:" << msg.get_port().get_name() << ", not found " << std::endl;
		}
	} break;
	}
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


