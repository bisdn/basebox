/*
 * cethcore.cpp
 *
 *  Created on: 03.08.2014
 *      Author: andreas
 */

#include "cethcore.hpp"

using namespace roflibs::ethernet;

/*static*/std::map<rofl::cdpid, cethcore*> cethcore::ethcores;
/*static*/std::set<uint64_t> cethcore::dpids;

void
cethcore::handle_dpt_open(rofl::crofdpt& dpt)
{
	try {
		state = STATE_ATTACHED;

		for (std::map<uint16_t, cvlan>::iterator
				it = vlans.begin(); it != vlans.end(); ++it) {
			if (it->second.get_group_id() != 0) {
				dpt.release_group_id(it->second.get_group_id());
			}
			it->second.handle_dpt_open(dpt);
		}

		// install 01:80:c2:xx:xx:xx entry in table 0
		rofl::openflow::cofflowmod fm(dpt.get_version());
		fm.set_command(rofl::openflow::OFPFC_ADD);
		fm.set_priority(0xf000);
		fm.set_table_id(table_id_eth_in);
		fm.set_match().set_eth_dst(rofl::caddress_ll("01:80:c2:00:00:00"), rofl::caddress_ll("ff:ff:ff:00:00:00"));
		fm.set_instructions().set_inst_apply_actions().set_actions().
				add_action_output(rofl::cindex(0)).set_port_no(rofl::openflow::OFPP_CONTROLLER);
		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		// install miss entry for src stage table
		fm.clear();
		fm.set_command(rofl::openflow::OFPFC_ADD);
		fm.set_priority(0x1000);
		fm.set_table_id(table_id_eth_src);
		fm.set_instructions().set_inst_apply_actions().set_actions().
				add_action_output(rofl::cindex(0)).set_port_no(rofl::openflow::OFPP_CONTROLLER);
		fm.set_instructions().set_inst_goto_table().set_table_id(table_id_eth_local/*2*/);
		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		// install miss entry for local address stage table (src-stage + 1)
		fm.clear();
		fm.set_command(rofl::openflow::OFPFC_ADD);
		fm.set_priority(0x1000);
		fm.set_table_id(table_id_eth_src+1);
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
		state = STATE_DETACHED;

		for (std::map<uint16_t, cvlan>::iterator
				it = vlans.begin(); it != vlans.end(); ++it) {
			it->second.handle_dpt_close(dpt);
			dpt.release_group_id(it->second.get_group_id());
		}

		// remove miss entry for src stage table
		rofl::openflow::cofflowmod fm(dpt.get_version());
		fm.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		fm.set_priority(0xf000);
		fm.set_table_id(table_id_eth_src);
		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		// remove miss entry for local address stage table (src-stage + 1)
		fm.clear();
		fm.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		fm.set_priority(0x1000);
		fm.set_table_id(table_id_eth_src+1);
		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		// remove 01:80:c2:xx:xx:xx entry from table 0
		fm.clear();
		fm.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		fm.set_priority(0xf000);
		fm.set_table_id(table_id_eth_in);
		fm.set_match().set_eth_dst(rofl::caddress_ll("01:80:c2:00:00:00"), rofl::caddress_ll("ff:ff:ff:00:00:00"));
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
		uint16_t vid = msg.get_match().get_vlan_vid_value() & (uint16_t)(~rofl::openflow::OFPVID_PRESENT);

		if (has_vlan(vid)) {
			set_vlan(vid).handle_packet_in(dpt, auxid, msg);
		} else {
			dpt.drop_buffer(auxid, msg.get_buffer_id());
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
cethcore::link_created(unsigned int ifindex)
{
	std::string devname;
	uint16_t vid = 1;
	bool tagged = false;

	try {
		const rofcore::crtlink& rtl = rofcore::cnetlink::get_instance().get_links().get_link(ifindex);

		devname = rtl.get_devname();

		size_t pos = rtl.get_devname().find_first_of(".");
		if (pos != std::string::npos) {
			devname = rtl.get_devname().substr(0, pos);
			std::stringstream svid(rtl.get_devname().substr(pos+1));
			svid >> vid;
			tagged = true;
		}

		const rofl::openflow::cofport& port = rofl::crofdpt::get_dpt(dpid).get_ports().get_port(devname);

		if ((not has_vlan(vid)) || (not get_vlan(vid).has_port(port.get_port_no()))) {
			// add port (and vlan implicitly, if not exists)
			set_vlan(vid).add_port(port.get_port_no(), port.get_hwaddr(), true);
			rofcore::logging::debug << "[cethcore][link_created] adding new port" << std::endl
					<< get_vlan(vid).get_port(port.get_port_no());
		} else
		if (get_vlan(vid).get_port(port.get_port_no()).get_hwaddr() != port.get_hwaddr()) {
			// hwaddr has changed => drop port and re-add (virtual links between LSIs)
			set_vlan(vid).drop_port(port.get_port_no());
			set_vlan(vid).add_port(port.get_port_no(), port.get_hwaddr(), true);
			rofcore::logging::debug << "[cethcore][link_created] readding port, mac address changed" << std::endl
					<< get_vlan(vid).get_port(port.get_port_no());
		}


	} catch (rofcore::crtlink::eRtLinkNotFound& e) {
		// should (:) never happen
		rofcore::logging::error << "[cethcore][link_created] link not found, ifindex: " << ifindex << std::endl;
	} catch (rofl::openflow::ePortNotFound& e) {
		// unknown interface
		rofcore::logging::error << "[cethcore][link_created] port not found, devname: " << devname << std::endl;
	} catch (rofl::eRofDptNotFound& e) {
		// do nothing
		rofcore::logging::error << "[cethcore][link_created] dpt not found, dpid: " << dpid << std::endl;
	}
}



void
cethcore::link_updated(unsigned int ifindex)
{
	std::string devname;
	uint16_t vid = 1;
	bool tagged = false;

	try {
		const rofcore::crtlink& rtl = rofcore::cnetlink::get_instance().get_links().get_link(ifindex);

		devname = rtl.get_devname();

		size_t pos = rtl.get_devname().find_first_of(".");
		if (pos != std::string::npos) {
			devname = rtl.get_devname().substr(0, pos);
			std::stringstream svid(rtl.get_devname().substr(pos+1));
			svid >> vid;
			tagged = true;
		}

		const rofl::openflow::cofport& port = rofl::crofdpt::get_dpt(dpid).get_ports().get_port(devname);

		if ((not has_vlan(vid)) || (not get_vlan(vid).has_port(port.get_port_no()))) {
			// add port (and vlan implicitly, if not exists)
			set_vlan(vid).add_port(port.get_port_no(), port.get_hwaddr(), true);
			rofcore::logging::debug << "[cethcore][link_updated] adding new port" << std::endl
					<< get_vlan(vid).get_port(port.get_port_no());
		} else
		if (get_vlan(vid).get_port(port.get_port_no()).get_hwaddr() != port.get_hwaddr()) {
			// hwaddr has changed => drop port and re-add (virtual links between LSIs)
			set_vlan(vid).drop_port(port.get_port_no());
			set_vlan(vid).add_port(port.get_port_no(), port.get_hwaddr(), true);
			rofcore::logging::debug << "[cethcore][link_updated] readding port, mac address changed" << std::endl
					<< get_vlan(vid).get_port(port.get_port_no());
		}
		rofcore::logging::debug << "[cethcore][link_updated] state:" << std::endl << *this;

	} catch (rofcore::crtlink::eRtLinkNotFound& e) {
		// should (:) never happen
		rofcore::logging::error << "[cethcore][link_updated] link not found, ifindex: " << ifindex << std::endl;
	} catch (rofl::openflow::ePortNotFound& e) {
		// unknown interface
		rofcore::logging::error << "[cethcore][link_updated] port not found, devname: " << devname << std::endl;
	} catch (rofl::eRofDptNotFound& e) {
		// do nothing
		rofcore::logging::error << "[cethcore][link_updated] dpt not found, dpid: " << dpid << std::endl;
	}
}



void
cethcore::link_deleted(unsigned int ifindex)
{
	std::string devname;
	uint16_t vid = 1;
	bool tagged = false;

	try {
		const rofcore::crtlink& rtl = rofcore::cnetlink::get_instance().get_links().get_link(ifindex);

		devname = rtl.get_devname();

		size_t pos = rtl.get_devname().find_first_of(".");
		if (pos != std::string::npos) {
			devname = rtl.get_devname().substr(0, pos);
			std::stringstream svid(rtl.get_devname().substr(pos+1));
			svid >> vid;
			tagged = true;
		}

		const rofl::openflow::cofport& port = rofl::crofdpt::get_dpt(dpid).get_ports().get_port(devname);

		if (not has_vlan(vid)) {
			return;
		}
		if (not get_vlan(vid).has_port(port.get_port_no())) {
			return;
		}
		rofcore::logging::debug << "[cethcore][link_deleted] deleting port:" << std::endl
				<< get_vlan(vid).get_port(port.get_port_no());

		set_vlan(vid).drop_port(port.get_port_no());


	} catch (rofcore::crtlink::eRtLinkNotFound& e) {
		// should (:) never happen
		rofcore::logging::error << "[cethcore][link_deleted] link not found, ifindex: " << ifindex << std::endl;
	} catch (rofl::openflow::ePortNotFound& e) {
		// unknown interface
		rofcore::logging::error << "[cethcore][link_deleted] port not found, devname: " << devname << std::endl;
	} catch (rofl::eRofDptNotFound& e) {
		// do nothing
		rofcore::logging::error << "[cethcore][link_deleted] dpt not found, dpid: " << dpid << std::endl;
	}
}



