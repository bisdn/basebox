#include "cethcore.hpp"

#include <inttypes.h>

using namespace ethcore;

/*static*/std::map<cdpid, cethcore*> cethcore::ethcores;


void
cethcore::link_created(unsigned int ifindex)
{
#if 0
	if (not netlink_enabled)
		return;

	std::string devbase;
	uint16_t vid(default_vid);

	try {
		// ge0.100 => vid 100 assigned to ge0
		std::string devname = rofcore::cnetlink::get_instance().get_links().get_link(ifindex).get_devname();

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

	} catch (rofcore::eNetLinkNotFound& e) {

		/* interface index ifindex not found, skip this event */

	} catch (eSportNotFound& e) {

		/* device is not member of this data path, so skip this event */

	} catch (eSportNotMember& e) {

		/* device exists, but is not member yet of the new VLAN, add membership in mode tagged */
		add_port_to_vlan(dpid, devbase, vid, true);
	}
#endif
}


void
cethcore::link_updated(unsigned int ifindex)
{
#if 0
	if (not netlink_enabled)
		return;

	std::string devbase;
	uint16_t vid(default_vid);

	try {
		// ge0.100 => vid 100 assigned to ge0
		std::string devname = rofcore::cnetlink::get_instance().get_links().get_link(ifindex).get_devname();

		if (devname.find_first_of(".") == std::string::npos) {
			return;
		}

		devbase = devname.substr(0, devname.find_first_of("."));
		vid = atoi(devname.substr(devname.find_first_of(".") + 1).c_str());

		rofl::logging::debug << "[ethcore][link-updated] devbase:" << devbase << " vid:" << (int)vid << std::endl;

	} catch (rofcore::eNetLinkNotFound& e) {

	}
#endif
}


void
cethcore::link_deleted(unsigned int ifindex)
{
#if 0
	if (not netlink_enabled)
		return;

	std::string devbase;
	uint16_t vid(default_vid);

	try {
		// ge0.100 => vid 100 removed from ge0
		std::string devname = rofcore::cnetlink::get_instance().get_links().get_link(ifindex).get_devname();

		if (devname.find_first_of(".") == std::string::npos) {
			return;
		}

		devbase = devname.substr(0, devname.find_first_of("."));
		vid = atoi(devname.substr(devname.find_first_of(".") + 1).c_str());

		rofl::logging::debug << "[ethcore][link-deleted] devbase:" << devbase << " vid:" << (int)vid << std::endl;

		drop_port_from_vlan(dpid, devbase, vid);

	} catch (rofcore::eNetLinkNotFound& e) {

		/* interface index ifindex was not found, skip event */

	} catch (eSportNotFound& e) {

		/* device was not found, skip event */

	} catch (eSportNotMember& e) {

		/* device is not member of VLAN vid, skip event */

	}
#endif
}







void
cethcore::handle_dpt_open(rofl::crofdpt& dpt)
{
	cvlantable::set_vtable(cdpid(dpt.get_dpid())).handle_dpt_open(dpt, );

#if 0
	try {
		netlink_enabled = (bool)cconfig::get_instance().lookup("ethcored.enable_netlink");
	} catch (...) {}

	logging::info << "[ethcore] dpath attaching dpid:" << (unsigned long long)dpt.get_dpid() << std::endl;

	dpt.flow_mod_reset();

	dpt.group_mod_reset();

	try {
		cfib::get_fib(dpt.get_dpid()).reset();
	} catch (eFibNotFound&e ) {
		// do nothing
	}

	sport::destroy_sports(dpt.get_dpid());

	try {
		default_vid = (int)cconfig::get_instance().lookup("ethcored.dpid_"+dpt.get_dpid_s()+".default_vid");
	} catch (SettingNotFoundException& e) {
		logging::warn << "[ethcore] unable to find config file entry for " << std::string("ethcored.dpid_"+dpt.get_dpid_s()+".default_vid") << std::endl;
	}

	/* we create a single default VLAN and add all ports in an untagged mode */
	add_vlan(dpt.get_dpid(), default_vid);

	dpid = dpt.get_dpid();

	/* create all VIDs defined for this datapath element */
	if (cconfig::get_instance().exists("ethcored.dpid_"+dpt.get_dpid_s()+".vids")) {
		for (unsigned int i = 0; i < cconfig::get_instance().lookup("ethcored.dpid_"+dpt.get_dpid_s()+".vids").getLength(); i++) {
			add_vlan(dpt.get_dpid(), (int)cconfig::get_instance().lookup("ethcored.dpid_"+dpt.get_dpid_s()+".vids")[i]);
		}
	}


	/*
	 * iterate over all ports announced by data path, create sport instances and set VID memberships
	 */
	for (std::map<uint32_t, rofl::openflow::cofport*>::const_iterator
			it = dpt.get_ports().get_ports().begin(); it != dpt.get_ports().get_ports().end(); ++it) {

		rofl::openflow::cofport* port = it->second;
		try {
			sport *sp = new sport(this, dpt.get_dpid(), port->get_port_no(), port->get_name(), port_stage_table_id, default_vid);
			logging::info << "[ethcore] adding port:" << std::endl << *sp;

			/* get VID memberships for port, if none exist, add port to default-vid */
			if (cconfig::get_instance().exists("ethcored.dpid_"+dpt.get_dpid_s()+"."+port->get_name())) {

				/* add untagged PVID for port */
				std::string port_pvid("ethcored.dpid_"+dpt.get_dpid_s()+"."+port->get_name()+".pvid");
				if (cconfig::get_instance().exists(port_pvid)) {
					sp->set_pvid((int)cconfig::get_instance().lookup(port_pvid));
					add_port_to_vlan(dpt.get_dpid(), port->get_name(), (int)cconfig::get_instance().lookup(port_pvid), false);
				}

				/* add tagged memberships */
				std::string tagged_vids("ethcored.dpid_"+dpt.get_dpid_s()+"."+port->get_name()+".tagged");
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
				if (not ltable[dpt.get_dptid()].has_link_by_ofp_port_no(port->get_port_no())) {
					ltable[dpt.get_dptid()].add_link(port->get_port_no());
				}
			}

		} catch (eSportExists& e) {
			logging::warn << "unable to add port:" << port->get_name() << ", already exists " << std::endl;
		}
	}

	sport::dump_sports();

	cfib::dump_fibs();
#endif
}



void
cethcore::handle_dpt_close(rofl::crofdpt& dpt)
{
#if 0
	logging::info << "[ethcore] dpath detaching dpid:" << (unsigned long long)dpt.get_dpid() << std::endl;

	cfib::destroy_fibs(dpt.get_dpid());

	// destroy all sports associated with dpt
	sport::destroy_sports(dpt.get_dpid());

	ltable.clear();
#endif
}



void
cethcore::handle_packet_in(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_packet_in& msg)
{
#if 0
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

		rofl::openflow::cofactions actions(dpt.get_version());
		dpt.send_packet_out_message(rofl::cauxid(0), msg.get_buffer_id(), msg.get_match().get_in_port(), actions);
	}
#endif
}


void
cethcore::handle_port_status(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_port_status& msg)
{
#if 0
	// TODO

	switch (msg.get_reason()) {
	case OFPPR_ADD: {

		sport *sp = (sport*)0;
		try {
			sp = &(sport::get_sport(dpt.get_dpid(), msg.get_port().get_name()));
		} catch (eSportNotFound& e) {
			sp = new sport(this, dpt.get_dpid(), msg.get_port().get_port_no(), msg.get_port().get_name(), port_stage_table_id, default_vid);
			logging::info << "[ethcore] adding port:" << std::endl << *sp;
		}

		/* get VID memberships for port, if none exist, add port to default-vid */
		if (cconfig::get_instance().exists("ethcored.dpid_"+dpt.get_dpid_s()+"."+msg.get_port().get_name())) {

			/* add untagged PVID for port */
			std::string port_pvid("ethcored.dpid_"+dpt.get_dpid_s()+"."+msg.get_port().get_name()+".pvid");
			if (cconfig::get_instance().exists(port_pvid)) {
				sp->set_pvid((int)cconfig::get_instance().lookup(port_pvid));
				add_port_to_vlan(dpt.get_dpid(), msg.get_port().get_name(), (int)cconfig::get_instance().lookup(port_pvid), false);
			}

			/* add tagged memberships */
			std::string tagged_vids("ethcored.dpid_"+dpt.get_dpid_s()+"."+msg.get_port().get_name()+".tagged");
			if (cconfig::get_instance().exists(tagged_vids)) {
				for (unsigned int i = 0; i < cconfig::get_instance().lookup(tagged_vids).getLength(); i++) {
					add_port_to_vlan(dpt.get_dpid(), msg.get_port().get_name(), (int)cconfig::get_instance().lookup(tagged_vids)[i], true);
				}
			}


		} else {
			/* add port to default VID */
			cfib::get_fib(dpt.get_dpid(), default_vid).add_port(msg.get_port().get_port_no(), /*untagged=*/false);
		}



		/*
		 * create new tap device for port announced by (reconnecting?) data path
		 */
		if (netlink_enabled) {
			if (not ltable[dpt.get_dptid()].has_link_by_ofp_port_no(msg.get_port().get_port_no())) {
				ltable[dpt.get_dptid()].add_link(msg.get_port().get_port_no());
			}
		}

#if 0
		try {
			sport *sp = new sport(this, dpt.get_dpid(), msg.get_port().get_port_no(), msg.get_port().get_name(), port_stage_table_id);
			logging::info << "[ethcore] adding port via Port-Status:" << std::endl << *sp;

			cfib::get_fib(dpt.get_dpid(), default_vid).add_port(msg.get_port().get_port_no(), /*untagged=*/false);
		} catch (eSportExists& e) {
			logging::warn << "[ethcore] unable to add port:" << msg.get_port().get_name() << ", already exists " << std::endl;
		}
#endif
	} break;
	case OFPPR_MODIFY: {
		logging::warn << "[ethcore] unhandled Port-Status MODIFY:" << std::endl << msg;

		sport *sp = (sport*)0;
		try {
			sp = &(sport::get_sport(dpt.get_dpid(), msg.get_port().get_port_no()));
		} catch (eSportNotFound& e) {
			sp = new sport(this, dpt.get_dpid(), msg.get_port().get_port_no(), msg.get_port().get_name(), port_stage_table_id);
			logging::info << "[ethcore] adding port:" << std::endl << *sp;
		}

	} break;
	case OFPPR_DELETE: try {

		sport *sp = (sport*)0;
		try {
			sp = &(sport::get_sport(dpt.get_dpid(), msg.get_port().get_port_no()));
		} catch (eSportNotFound& e) {
			return;
		}

		/* get VID memberships for port, if none exist, add port to default-vid */
		if (cconfig::get_instance().exists("ethcored.dpid_"+dpt.get_dpid_s()+"."+msg.get_port().get_name())) {

			/* add untagged PVID for port */
			std::string port_pvid("ethcored.dpid_"+dpt.get_dpid_s()+"."+msg.get_port().get_name()+".pvid");
			if (cconfig::get_instance().exists(port_pvid)) {
				drop_port_from_vlan(dpt.get_dpid(), msg.get_port().get_name(), (int)cconfig::get_instance().lookup(port_pvid));
			}

			/* add tagged memberships */
			std::string tagged_vids("ethcored.dpid_"+dpt.get_dpid_s()+"."+msg.get_port().get_name()+".tagged");
			if (cconfig::get_instance().exists(tagged_vids)) {
				for (unsigned int i = 0; i < cconfig::get_instance().lookup(tagged_vids).getLength(); i++) {
					drop_port_from_vlan(dpt.get_dpid(), msg.get_port().get_name(), (int)cconfig::get_instance().lookup(tagged_vids)[i]);
				}
			}


		} else {
			/* add port to default VID */
			cfib::get_fib(dpt.get_dpid(), default_vid).drop_port(msg.get_port().get_port_no());
		}



		/*
		 * create new tap device for port announced by (reconnecting?) data path
		 */
		if (netlink_enabled) {
			if (ltable[dpt.get_dptid()].has_link_by_ofp_port_no(msg.get_port().get_port_no())) {
				ltable[dpt.get_dptid()].drop_link(msg.get_port().get_port_no());
			}
		}

		delete sp;

#if 0
		try {
			sport& sp = sport::get_sport(dpt.get_dpid(), msg.get_port().get_port_no());
			logging::info << "[ethcore] dropping port via Port-Status:" << std::endl << sp;

			cfib::get_fib(dpt.get_dpid(), default_vid).drop_port(msg.get_port().get_port_no());

			delete &sp;
		} catch (eSportNotFound& e) {
			logging::warn << "[ethcore] unable to drop port:" << msg.get_port().get_name() << ", not found " << std::endl;
		}
#endif

	} catch (eSportNotFound& e) {
		// do nothing
	} break;
	}

	sport::dump_sports();
#endif
}


void
cethcore::handle_flow_removed(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_flow_removed& msg)
{
#if 0
	uint16_t vid = default_vid;
	try {

		try {
			/* check for VLAN VID OXM TLV in Packet-In message */
			vid = msg.get_match().get_vlan_vid() & ~OFPVID_PRESENT;
		} catch (eOFmatchNotFound& e) {
			/* no VLAN VID OXM TLV => frame was most likely untagged */
			vid = sport::get_sport(dpt.get_dpid(), msg.get_match().get_in_port()).get_pvid() & ~OFPVID_PRESENT;
		}

		cfib::get_fib(dpt.get_dpid(), vid).handle_flow_removed(msg);

	} catch (eFibNotFound& e) {
		// no FIB for specified VLAN found
		logging::warn << "[ethcore][flow-removed] no VLAN vid:" << (int)vid << " configured on dpid:" << (unsigned long long)dpt.get_dpid() << std::endl;

	} catch (eSportNotFound& e) {
		// sport instance not found? critical error!
		logging::crit << "[ethcore][flow-removed] no sport instance for in-port found in Flow-Removed message" << msg << std::endl;

	} catch (eSportNoPvid& e) {
		// drop frame on data path
		logging::crit << "[ethcore][flow-removed] no PVID for sport instance found for Flow-Removed message" << msg << std::endl;

	}
#endif
}


void
cethcore::handle_error_message(
		rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_error& msg)
{

}


uint32_t
cethcore::get_idle_group_id()
{
	uint32_t group_id = 0;
	while (group_ids.find(group_id) != group_ids.end()) {
		group_id++;
	}
	group_ids.insert(group_id);
	return group_id;
}


void
cethcore::release_group_id(uint32_t group_id)
{
	if (group_ids.find(group_id) != group_ids.end()) {
		group_ids.erase(group_id);
	}
}

#if 0
void
cethcore::add_vlan(
		uint64_t dpid,
		uint16_t vid)
{
	try {
		logging::info << "[ethcore] adding vid:" << std::dec << (int)vid << " to dpid:" << (unsigned long long)dpid << std::endl;
		new cfib(this, dpid, vid, fib_in_stage_table_id, fib_out_stage_table_id);
	} catch (eFibExists& e) {
		logging::warn << "[ethcore] adding vid:" << (int)vid << " to dpid:" << (unsigned long long)dpid << " failed" << std::endl;
	}

	cfib::dump_fibs();
}


void
cethcore::drop_vlan(
		uint64_t dpid,
		uint16_t vid)
{
	try {
		logging::info << "[ethcore] dropping vid:" << std::dec << (int)vid << " from dpid:" << (unsigned long long)dpid << std::endl;
		delete &(cfib::get_fib(dpid, vid));
	} catch (eFibNotFound& e) {
		logging::warn << "[ethcore] dropping vid:" << (int)vid << " from dpid:" << (unsigned long long)dpid << " failed" << std::endl;
	}

	cfib::dump_fibs();
}


void
cethcore::add_port_to_vlan(
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

	sport::dump_sports();
}


void
cethcore::drop_port_from_vlan(
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

	sport::dump_sports();
}
#endif

