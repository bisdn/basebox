/*
 * cethcore.cpp
 *
 *  Created on: 03.08.2014
 *      Author: andreas
 */

#include "cethcore.hpp"

using namespace roflibs::eth;

/*static*/std::map<rofl::cdptid, cethcore*> cethcore::ethcores;
/*static*/std::set<uint64_t> cethcore::dpids;
/*static*/std::string cethcore::script_path_port_up 	= std::string("/var/lib/basebox/port-up.sh");
/*static*/std::string cethcore::script_path_port_down 	= std::string("/var/lib/basebox/port-down.sh");

cethcore::cethcore(const rofl::cdptid& dptid,
		uint8_t table_id_eth_in, uint8_t table_id_eth_src,
		uint8_t table_id_eth_local, uint8_t table_id_eth_dst,
		uint16_t default_pvid) :
			state(STATE_IDLE),
			cookie_grp_addr(roflibs::common::openflow::ccookie_owner::acquire_cookie()),
			cookie_miss_entry_src(roflibs::common::openflow::ccookie_owner::acquire_cookie()),
			cookie_miss_entry_local(roflibs::common::openflow::ccookie_owner::acquire_cookie()),
			cookie_redirect_inject(roflibs::common::openflow::ccookie_owner::acquire_cookie()),
			dptid(dptid), default_pvid(default_pvid),
			table_id_eth_in(table_id_eth_in),
			table_id_eth_src(table_id_eth_src),
			table_id_eth_local(table_id_eth_local),
			table_id_eth_dst(table_id_eth_dst) {

	if (cethcore::ethcores.find(dptid) != cethcore::ethcores.end()) {
		throw eEthCoreExists("cethcore::cethcore() dpid already exists");
	}

	// deploy pre-defined ethernet endpoints
	add_eth_endpnts();
}


cethcore::~cethcore() {
	try {
		if (STATE_ATTACHED == state) {
			handle_dpt_close(rofl::crofdpt::get_dpt(dptid));
		}
	} catch (rofl::eRofDptNotFound& e) {}

	clear_tap_devs(dptid);
}


void
cethcore::add_eth_endpnts()
{
#if 0
	/*
	 * create virtual ports for predefined ethernet endpoints
	 */
	roflibs::eth::cportdb& portdb = roflibs::eth::cportdb::get_portdb("file");

	// install ethernet endpoints
	for (std::set<std::string>::const_iterator
			it = portdb.get_eth_entries(dpt.get_dpid()).begin(); it != portdb.get_eth_entries(dpt.get_dpid()).end(); ++it) {
		const roflibs::eth::cethentry& eth = portdb.get_eth_entry(dpt.get_dpid(), *it);

		if (not has_tap_dev(dpt.get_dpid(), eth.get_devname())) {
			add_tap_dev(dpt.get_dpid(), eth.get_devname(), eth.get_port_vid(), eth.get_hwaddr());
		}
	}
#endif

	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);

		cportdb& portdb = cportdb::get_portdb(std::string("file"));

		default_pvid = portdb.get_default_pvid(rofl::crofdpt::get_dpt(dptid).get_dpid());
		add_vlan(default_pvid);

		rofl::cdpid dpid = rofl::crofdpt::get_dpt(dptid).get_dpid();

		// install ethernet endpoints
		for (std::set<std::string>::const_iterator
				it = portdb.get_eth_entries(dpid).begin(); it != portdb.get_eth_entries(dpid).end(); ++it) {
			const cethentry& eth = portdb.get_eth_entry(dpid, *it);

			set_vlan(eth.get_port_vid()).add_eth_endpnt(eth.get_hwaddr(), /*tagged=*/false);

			if (not has_tap_dev(dpt.get_dptid(), eth.get_devname())) {
				add_tap_dev(dpt.get_dptid(), eth.get_devname(), eth.get_port_vid(), eth.get_hwaddr());
			}
		}

	} catch (rofl::eRofDptNotFound& e) {

	}
}


void
cethcore::add_phy_ports()
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);

		cportdb& portdb = cportdb::get_portdb(std::string("file"));

		// install physical ports
		for (std::map<uint32_t, rofl::openflow::cofport*>::const_iterator
				it = dpt.get_ports().get_ports().begin(); it != dpt.get_ports().get_ports().end(); ++it) {

			const rofl::openflow::cofport& ofport = *(it->second);
			uint32_t portno = it->first;
			if (not portdb.has_port_entry(rofl::crofdpt::get_dpt(dptid).get_dpid(), ofport.get_port_no())) {
				set_vlan(default_pvid).add_phy_port(portno, ofport.get_hwaddr(), /*tagged*/false);
				continue;
			}
			const cportentry& port = portdb.get_port_entry(rofl::crofdpt::get_dpt(dptid).get_dpid(), portno);

			// create or update existing vlan for port's default vid
			set_vlan(port.get_port_vid()).add_phy_port(portno, ofport.get_hwaddr(), /*tagged=*/false);

			// add all tagged memberships of this port to the appropriate vlan
			for (std::set<uint16_t>::const_iterator
					jt = port.get_tagged_vids().begin(); jt != port.get_tagged_vids().end(); ++jt) {
				set_vlan(*jt).set_phy_port(portno, ofport.get_hwaddr(), /*tagged=*/true);
			}
		}
	} catch (rofl::eRofDptNotFound& e) {

	}
}


void
cethcore::clear_phy_ports()
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);

		cportdb& portdb = cportdb::get_portdb(std::string("file"));

		// install physical ports
		for (std::map<uint16_t, cvlan>::iterator
				it = vlans.begin(); it != vlans.end(); ++it) {
			it->second.clear_phy_ports();
		}
	} catch (rofl::eRofDptNotFound& e) {

	}
}


void
cethcore::handle_dpt_open(rofl::crofdpt& dpt)
{
	try {
		// deploy physical port vlan memberships
		add_phy_ports();

		state = STATE_ATTACHED;

		for (std::map<uint16_t, cvlan>::iterator
				it = vlans.begin(); it != vlans.end(); ++it) {
			if (it->second.get_group_id() != 0) {
				dpt.release_group_id(it->second.get_group_id());
			}
			it->second.handle_dpt_open(dpt);
		}

		// install 01:80:c2:xx:xx:xx entry in table 0
		rofl::openflow::cofflowmod fm(dpt.get_version_negotiated());
		fm.set_command(rofl::openflow::OFPFC_ADD);
		fm.set_priority(0xf000);
		fm.set_cookie(cookie_grp_addr);
		fm.set_table_id(table_id_eth_in);
		fm.set_match().set_eth_dst(rofl::caddress_ll("01:80:c2:00:00:00"), rofl::caddress_ll("ff:ff:ff:00:00:00"));
		fm.set_instructions().set_inst_apply_actions().set_actions().
				add_action_output(rofl::cindex(0)).set_port_no(rofl::openflow::OFPP_CONTROLLER);
		fm.set_instructions().set_inst_apply_actions().set_actions().
				set_action_output(rofl::cindex(0)).set_max_len(1526);
		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		// install miss entry for src stage table
		fm.clear();
		fm.set_command(rofl::openflow::OFPFC_ADD);
		fm.set_priority(0x1000);
		fm.set_cookie(cookie_miss_entry_src);
		fm.set_table_id(table_id_eth_src);
		fm.set_instructions().set_inst_apply_actions().set_actions().
				add_action_output(rofl::cindex(0)).set_port_no(rofl::openflow::OFPP_CONTROLLER);
		fm.set_instructions().set_inst_apply_actions().set_actions().
				set_action_output(rofl::cindex(0)).set_max_len(1526);
		fm.set_instructions().set_inst_goto_table().set_table_id(table_id_eth_local/*2*/);
		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		// install miss entry for local address stage table (src-stage + 1)
		fm.clear();
		fm.set_command(rofl::openflow::OFPFC_ADD);
		fm.set_priority(0x1000);
		fm.set_cookie(cookie_miss_entry_local);
		fm.set_table_id(table_id_eth_src+1);
		fm.set_instructions().set_inst_goto_table().set_table_id(table_id_eth_dst);
		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		// forward packets received from special port OFPP_CONTROLLER to table_id_eth_dst
		fm.set_table_id(table_id_eth_in);
		fm.set_priority(0x8200);
		fm.set_cookie(cookie_redirect_inject);
		fm.set_match().clear();
		fm.set_match().set_in_port(rofl::openflow::OFPP_CONTROLLER);
		fm.set_instructions().set_inst_goto_table().set_table_id(table_id_eth_dst);
		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::debug << "[cethcore][handle_dpt_open] control channel congested" << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::debug << "[cethcore][handle_dpt_open] control channel not connected" << std::endl;
	}
}



void
cethcore::handle_dpt_close(rofl::crofdpt& dpt)
{
	try {
		// remove physical ports
		clear_phy_ports();

		state = STATE_DETACHED;

		for (std::map<uint16_t, cvlan>::iterator
				it = vlans.begin(); it != vlans.end(); ++it) {
			it->second.handle_dpt_close(dpt);
			dpt.release_group_id(it->second.get_group_id());
		}

		// remove miss entry for src stage table
		rofl::openflow::cofflowmod fm(dpt.get_version_negotiated());
		fm.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		fm.set_priority(0xf000);
		fm.set_cookie(cookie_miss_entry_src);
		fm.set_table_id(table_id_eth_src);
		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		// remove miss entry for local address stage table (src-stage + 1)
		fm.clear();
		fm.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		fm.set_priority(0x1000);
		fm.set_cookie(cookie_miss_entry_local);
		fm.set_table_id(table_id_eth_src+1);
		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		// remove 01:80:c2:xx:xx:xx entry from table 0
		fm.clear();
		fm.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		fm.set_priority(0xf000);
		fm.set_cookie(cookie_grp_addr);
		fm.set_table_id(table_id_eth_in);
		fm.set_match().set_eth_dst(rofl::caddress_ll("01:80:c2:00:00:00"), rofl::caddress_ll("ff:ff:ff:00:00:00"));
		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		// forward packets received from special port OFPP_CONTROLLER to table_id_eth_dst
		fm.set_table_id(table_id_eth_in);
		fm.set_priority(0x8200);
		fm.set_cookie(cookie_redirect_inject);
		fm.set_match().clear();
		fm.set_match().set_in_port(rofl::openflow::OFPP_CONTROLLER);
		dpt.send_flow_mod_message(rofl::cauxid(0), fm);


	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::debug << "[cethcore][handle_dpt_close] control channel congested" << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::debug << "[cethcore][handle_dpt_close] control channel not connected" << std::endl;
	}
}



void
cethcore::handle_packet_in(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_packet_in& msg)
{
	try {
		rofcore::logging::debug << "[cethcore][handle_packet_in] pkt received: " << std::endl << msg;

		// yet unknown source address found in eth-src stage
		if (msg.get_table_id() == table_id_eth_src) {

			uint16_t vid = msg.get_match().get_vlan_vid_value() & (uint16_t)(~rofl::openflow::OFPVID_PRESENT);

			if (has_vlan(vid)) {
				set_vlan(vid).handle_packet_in(dpt, auxid, msg);
			} else {
				dpt.drop_buffer(auxid, msg.get_buffer_id());
			}

		// all other stages: send frame to tap devices
		} else {

			if (not msg.get_match().has_vlan_vid()) {
				// no VLAN related metadata found, ignore packet
				rofcore::logging::debug << "[cethcore][handle_packet_in] frame without VLAN tag received, dropping" << std::endl;
				dpt.drop_buffer(auxid, msg.get_buffer_id());
				return;
			}

			if (not msg.get_match().get_eth_dst().is_multicast()) {
				/* non-multicast frames are directly injected into a tapdev */
				rofl::cpacket *pkt = rofcore::cpacketpool::get_instance().acquire_pkt();
				*pkt = msg.get_packet();
				pkt->pop(sizeof(struct rofl::fetherframe::eth_hdr_t)-sizeof(uint16_t), sizeof(struct rofl::fvlanframe::vlan_hdr_t));
				set_tap_dev(dptid, msg.get_match().get_eth_dst()).enqueue(pkt);

			} else {
				/* multicast frames carry a metadata field with the VLAN id
				 * for both tagged and untagged frames. Lookup all tapdev
				 * instances belonging to the specified vid and clone the packet
				 * for all of them. */

				uint16_t vid = msg.get_match().get_vlan_vid() & 0x0fff;

				for (std::map<std::string, rofcore::ctapdev*>::iterator
						it = devs[dptid].begin(); it != devs[dptid].end(); ++it) {
					rofcore::ctapdev& tapdev = *(it->second);
					if (tapdev.get_pvid() != vid) {
						continue;
					}
					rofl::cpacket *pkt = rofcore::cpacketpool::get_instance().acquire_pkt();
					*pkt = msg.get_packet();
					// offset: 12 bytes (eth-hdr without type), nbytes: 4 bytes
					pkt->pop(sizeof(struct rofl::fetherframe::eth_hdr_t)-sizeof(uint16_t), sizeof(struct rofl::fvlanframe::vlan_hdr_t));
					tapdev.enqueue(pkt);
				}
			}

			// drop buffer on data path, if the packet was stored there, as it is consumed entirely by the underlying kernel
			if (rofl::openflow::base::get_ofp_no_buffer(dpt.get_version_negotiated()) != msg.get_buffer_id()) {
				dpt.drop_buffer(auxid, msg.get_buffer_id());
			}
		}


	} catch (rofl::openflow::eOxmNotFound& e) {
		rofcore::logging::debug << "[cethcore][handle_packet_in] no VID match found" << std::endl;
	}
}



void
cethcore::handle_flow_removed(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_flow_removed& msg)
{
	try {
		uint16_t vid = msg.get_match().get_vlan_vid_value() & (uint16_t)(~rofl::openflow::OFPVID_PRESENT);

		if (has_vlan(vid)) {
			set_vlan(vid).handle_flow_removed(dpt, auxid, msg);
		}

	} catch (rofl::openflow::eOxmNotFound& e) {
		rofcore::logging::debug << "[cethcore][handle_flow_removed] no VID match found" << std::endl;
	}
}



void
cethcore::handle_port_status(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_port_status& msg)
{
	cportdb& portdb = cportdb::get_portdb(std::string("file"));

	switch (msg.get_reason()) {
	case rofl::openflow::OFPPR_ADD:
	case rofl::openflow::OFPPR_MODIFY: {
		// no configuration for this port => add to default vlan
		if (not portdb.has_port_entry(rofl::crofdpt::get_dpt(dptid).get_dpid(), msg.get_port().get_port_no())) {
			set_vlan(default_pvid).add_phy_port(msg.get_port().get_port_no(), msg.get_port().get_hwaddr(), /*tagged*/false);

		// configuration for this port found => add port vid and tagged memberships
		} else {
			const cportentry& port = portdb.get_port_entry(rofl::crofdpt::get_dpt(dptid).get_dpid(), msg.get_port().get_port_no());

			// create or update existing vlan for port's default vid
			set_vlan(port.get_port_vid()).add_phy_port(msg.get_port().get_port_no(), msg.get_port().get_hwaddr(), /*tagged=*/false);

			// add all tagged memberships of this port to the appropriate vlan
			for (std::set<uint16_t>::const_iterator
					jt = port.get_tagged_vids().begin(); jt != port.get_tagged_vids().end(); ++jt) {
				set_vlan(*jt).set_phy_port(msg.get_port().get_port_no(), msg.get_port().get_hwaddr(), /*tagged=*/true);
			}
		}

	} break;
	case rofl::openflow::OFPPR_DELETE: {

		// no configuration for this port => drop port from default vlan
		if (not portdb.has_port_entry(rofl::crofdpt::get_dpt(dptid).get_dpid(), msg.get_port().get_port_no())) {
			set_vlan(default_pvid).drop_phy_port(msg.get_port().get_port_no());

		// configuration for this port found => drop port vid and tagged memberships
		} else {
			const cportentry& port = portdb.get_port_entry(rofl::crofdpt::get_dpt(dptid).get_dpid(), msg.get_port().get_port_no());

			// drop port from port's default vid
			set_vlan(port.get_port_vid()).drop_phy_port(msg.get_port().get_port_no());

			// drop all tagged memberships of this port from the appropriate vlan
			for (std::set<uint16_t>::const_iterator
					jt = port.get_tagged_vids().begin(); jt != port.get_tagged_vids().end(); ++jt) {
				set_vlan(*jt).drop_phy_port(msg.get_port().get_port_no());
			}
		}

	} break;
	default: {

		rofcore::logging::error << "[cethcore][handle_port_status] unknown reason" << std::endl;
	} return;
	}


	for (std::map<uint16_t, cvlan>::iterator
			it = vlans.begin(); it != vlans.end(); ++it) {
		it->second.handle_port_status(dpt, auxid, msg);
	}
}



void
cethcore::handle_error_message(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_error& msg)
{
	for (std::map<uint16_t, cvlan>::iterator
			it = vlans.begin(); it != vlans.end(); ++it) {
		it->second.handle_error_message(dpt, auxid, msg);
	}
}


void
cethcore::enqueue(rofcore::cnetdev *netdev, rofl::cpacket* pkt)
{
	try {
		rofcore::ctapdev* tapdev = dynamic_cast<rofcore::ctapdev*>( netdev );
		if (0 == tapdev) {
			throw eLinkTapDevNotFound("cethcore::enqueue() tap device not found");
		}

		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(tapdev->get_dptid());

		if (not dpt.is_established()) {
			throw eLinkNoDptAttached("cethcore::enqueue() dpt not found");
		}

		rofl::openflow::cofactions actions(dpt.get_version_negotiated());
		actions.set_action_push_vlan(rofl::cindex(0)).set_eth_type(rofl::fvlanframe::VLAN_CTAG_ETHER);
		actions.set_action_set_field(rofl::cindex(1)).set_oxm(rofl::openflow::coxmatch_ofb_vlan_vid(tapdev->get_pvid()));
		actions.set_action_output(rofl::cindex(2)).set_port_no(rofl::openflow::OFPP_TABLE);

		dpt.send_packet_out_message(
				rofl::cauxid(0),
				rofl::openflow::base::get_ofp_no_buffer(dpt.get_version_negotiated()),
				rofl::openflow::base::get_ofpp_controller_port(dpt.get_version_negotiated()),
				actions,
				pkt->soframe(),
				pkt->length());

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[cethcore][enqueue] no data path attached, dropping outgoing packet" << std::endl;

	} catch (eLinkNoDptAttached& e) {
		rofcore::logging::error << "[cethcore][enqueue] no data path attached, dropping outgoing packet" << std::endl;

	} catch (eLinkTapDevNotFound& e) {
		rofcore::logging::error << "[cethcore][enqueue] unable to find tap device" << std::endl;
	}

	rofcore::cpacketpool::get_instance().release_pkt(pkt);
}



void
cethcore::enqueue(rofcore::cnetdev *netdev, std::vector<rofl::cpacket*> pkts)
{
	for (std::vector<rofl::cpacket*>::iterator
			it = pkts.begin(); it != pkts.end(); ++it) {
		enqueue(netdev, *it);
	}
}




void
cethcore::hook_port_up(const rofl::crofdpt& dpt, std::string const& devname)
{
	try {

		std::vector<std::string> argv;
		std::vector<std::string> envp;

		std::stringstream s_dpid;
		s_dpid << "DPID=" << dpt.get_dpid();
		envp.push_back(s_dpid.str());

		std::stringstream s_devname;
		s_devname << "DEVNAME=" << devname;
		envp.push_back(s_devname.str());

		cethcore::execute(cethcore::script_path_port_up, argv, envp);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[cbasebox][hook_port_up] script execution failed" << std::endl;
	}
}



void
cethcore::hook_port_down(const rofl::crofdpt& dpt, std::string const& devname)
{
	try {

		std::vector<std::string> argv;
		std::vector<std::string> envp;

		std::stringstream s_dpid;
		s_dpid << "DPID=" << dpt.get_dpid();
		envp.push_back(s_dpid.str());

		std::stringstream s_devname;
		s_devname << "DEVNAME=" << devname;
		envp.push_back(s_devname.str());

		cethcore::execute(cethcore::script_path_port_down, argv, envp);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[cbasebox][hook_port_down] script execution failed" << std::endl;
	}
}



void
cethcore::execute(
		std::string const& executable,
		std::vector<std::string> argv,
		std::vector<std::string> envp)
{
	pid_t pid = 0;

	if ((pid = fork()) < 0) {
		rofcore::logging::error << "[cbasebox][execute] syscall error fork(): " << errno << ":" << strerror(errno) << std::endl;
		return;
	}

	if (pid > 0) { // father process
		int status;
		waitpid(pid, &status, 0);
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


