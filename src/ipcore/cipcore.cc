#include "cipcore.h"

using namespace ipcore;

std::map<rofl::cdptid, cipcore*> cipcore::ipcores;

std::string cipcore::script_path_dpt_open 	= std::string("/var/lib/ipcore/dpath-open.sh");
std::string cipcore::script_path_dpt_close 	= std::string("/var/lib/ipcore/dpath-close.sh");
std::string cipcore::script_path_port_up 	= std::string("/var/lib/ipcore/port-up.sh");
std::string cipcore::script_path_port_down 	= std::string("/var/lib/ipcore/port-down.sh");

cipcore::cipcore(
		const rofl::openflow::cofhello_elem_versionbitmap& versionbitmap) :
		rofl::crofbase(versionbitmap)
{

}


cipcore::~cipcore()
{
	rtables.clear();
	ltable.clear();
}



void
cipcore::handle_dpt_open(rofl::crofdpt& dpt)
{
	dptid = dpt.get_dptid();

	rtables.clear();
	ltable.clear();

	purge_dpt_entries();
	block_stp_frames();
	redirect_ipv4_multicast();
	redirect_ipv6_multicast();

	/*
	 * create new tap devices for (reconnecting?) data path
	 */
	std::map<uint32_t, rofl::openflow::cofport*> ports = dpt.get_ports().get_ports();
	for (std::map<uint32_t, rofl::openflow::cofport*>::iterator
			it = ports.begin(); it != ports.end(); ++it) try {
		/* set_link() avoids destruction of an already existing tap device.
		 * this may avoid race conditions with the kernel, when a fast reconstruction is needed.
		 */
		ltable.set_link(/*port_no*/it->first).tap_open();
		ltable.set_link(/*port_no*/it->first).install();
	} catch (rofcore::eNetDevCritical& e) {
		rofcore::logging::debug << "[ipcore] new data path attaching: unable to create tap device: " << it->second->get_name() << std::endl;
	}

	// deprecated:
	// get full-length packets (what is the ethernet max length on dpt?)
	//dpt.send_set_config_message(rofl::cauxid(0), 0, __ETH_FRAME_LEN);


	// install default policy for table 0 => GotoTable(1)
	if (rofcore::cconfig::get_instance().exists("ipcored.enable_forwarding")) {
		set_forwarding(true);
	} else {
		set_forwarding(false);
	}

	// call external scripting hook
	hook_dpt_attach();
}



void
cipcore::handle_dpt_close(rofl::crofdpt& dpt)
{
	if (dptid != dpt.get_dptid()) {
		return;
	}

	// call external scripting hook
	hook_dpt_detach();

	rtables.clear();

	ltable.clear();
}



void
cipcore::handle_port_status(
		rofl::crofdpt& dpt,
		const rofl::cauxid& auxid,
		rofl::openflow::cofmsg_port_status& msg)
{

	if (dpt.get_dptid() != dptid) {
		rofcore::logging::debug << "[ipcore] received PortStatus from invalid data path, ignoring" << std::endl;
		return;
	}

	uint32_t port_no = msg.get_port().get_port_no();

	rofcore::logging::debug << "[ipcore] Port-Status message rcvd:" << std::endl << msg;

	try {
		switch (msg.get_reason()) {
		case rofl::openflow::OFPPR_ADD: {
			ltable.set_link(port_no).tap_open();
			hook_port_up(msg.get_port().get_name());
		} break;
		case rofl::openflow::OFPPR_MODIFY: {
			ltable.set_link(port_no).tap_open();
			hook_port_up(msg.get_port().get_name());
		} break;
		case rofl::openflow::OFPPR_DELETE: {
			ltable.set_link(port_no).tap_close();
			ltable.drop_link(port_no);
			hook_port_down(msg.get_port().get_name());
		} break;
		default: {
			rofcore::logging::debug << "[ipcore] received PortStatus with unknown reason code received, ignoring" << std::endl;
		};
		}

	} catch (rofcore::eNetDevCritical& e) {
		rofcore::logging::debug << "[ipcore] new port created: unable to create tap device: " << msg.get_port().get_name() << std::endl;
	}
}



void
cipcore::handle_packet_out(rofl::crofctl& ctl, const rofl::cauxid& auxid, rofl::openflow::cofmsg_packet_out& msg)
{

}




void
cipcore::handle_packet_in(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_packet_in& msg)
{
	try {
		uint32_t port_no = msg.get_match().get_in_port();

		if (not ltable.has_link(port_no)) {
			rofcore::logging::debug << "[ipcore] received PacketIn message for unknown port, ignoring" << std::endl;
			return;
		}

		ltable.set_link(port_no).handle_packet_in(msg.get_packet());

		if (rofl::openflow::base::get_ofp_no_buffer(dpt.get_version()) != msg.get_buffer_id()) {
			rofl::openflow::cofactions actions(dpt.get_version());
			dpt.send_packet_out_message(auxid, msg.get_buffer_id(), msg.get_match().get_in_port(), actions);
		}

	} catch (rofcore::ePacketPoolExhausted& e) {
		rofcore::logging::warn << "[ipcore] received PacketIn message, but buffer-pool exhausted" << std::endl;
	} catch (rofl::openflow::eOxmNotFound& e) {
		rofcore::logging::debug << "[ipcore] received PacketIn message without in-port match, ignoring" << std::endl;
	}
}



void
cipcore::handle_error_message(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_error& msg)
{
	rofl::logging::warn << "[ipcore] error message rcvd:" << std::endl << msg;
}



void
cipcore::handle_flow_removed(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_flow_removed& msg)
{
	rofl::logging::info << "[ipcore] Flow-Removed message rcvd:" << std::endl << msg;
}



void
cipcore::set_forwarding(bool forward)
{
	try {
		rofl::openflow::cofflowmod fed = rofl::openflow::cofflowmod(rofl::crofdpt::get_dpt(dptid).get_version());

		if (forward == true) {
			fed.set_command(rofl::openflow::OFPFC_MODIFY_STRICT);
		} else {
			fed.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		}
		fed.set_table_id(0);
		fed.set_idle_timeout(0);
		fed.set_hard_timeout(0);
		fed.set_priority(0); // lowest priority
		fed.set_instructions().add_inst_goto_table().set_table_id(1);

		rofl::crofdpt::get_dpt(dptid).send_flow_mod_message(rofl::cauxid(0), fed);

	} catch (rofl::eSocketTxAgain& e) {

		rofcore::logging::debug << "[ipcore] enable forwarding: control channel congested, rescheduling (TODO)" << std::endl;

	} catch (rofl::eRofDptNotFound& e) {

		rofcore::logging::debug << "[ipcore] enable forwarding: dptid not found: " << dptid << std::endl;

	}
}


void
cipcore::addr_in4_created(unsigned int ifindex, uint16_t adindex)
{
	try {

		if (not ltable.has_link(ifindex)) {
			rofcore::logging::error << "[ipcore] addr_in4 create: link ifindex not found: " << ifindex << std::endl;
			return;
		}

		if (not ltable.get_link(ifindex).get_addr_table().has_addr_in4(adindex)) {
			ltable.set_link(ifindex).set_addr_table().set_addr_in4(adindex).install();
		}

	} catch (std::runtime_error& e) {
		rofcore::logging::error << "[ipcore] addr_in4 create: exception " << e.what() << std::endl;
	}
}

void
cipcore::addr_in4_updated(unsigned int ifindex, uint16_t adindex)
{
	try {

		if (not ltable.has_link(ifindex)) {
			rofcore::logging::error << "[ipcore] addr_in4 update: link ifindex not found: " << ifindex << std::endl;
			return;
		}

		if (not ltable.get_link(ifindex).get_addr_table().has_addr_in4(adindex)) {
			ltable.set_link(ifindex).set_addr_table().set_addr_in4(adindex).install();
		}

	} catch (std::runtime_error& e) {
		rofcore::logging::error << "[ipcore] addr_in4 update: exception " << e.what() << std::endl;
	}
}

void
cipcore::addr_in4_deleted(unsigned int ifindex, uint16_t adindex)
{
	try {

		if (not ltable.has_link(ifindex)) {
			rofcore::logging::error << "[ipcore] addr_in4 delete: link ifindex not found: " << ifindex << std::endl;
			return;
		}

		if (ltable.get_link(ifindex).get_addr_table().has_addr_in4(adindex)) {
			ltable.set_link(ifindex).set_addr_table().set_addr_in4(adindex).uninstall();
			ltable.set_link(ifindex).set_addr_table().drop_addr_in4(adindex);
		}

	} catch (std::runtime_error& e) {
		rofcore::logging::error << "[ipcore] addr_in4 delete: exception " << e.what() << std::endl;
	}
}

void
cipcore::addr_in6_created(unsigned int ifindex, uint16_t adindex)
{
	try {

		if (not ltable.has_link(ifindex)) {
			rofcore::logging::error << "[ipcore] addr_in6 create: link ifindex not found: " << ifindex << std::endl;
			return;
		}

		if (not ltable.get_link(ifindex).get_addr_table().has_addr_in6(adindex)) {
			ltable.set_link(ifindex).set_addr_table().set_addr_in6(adindex).install();
		}

	} catch (std::runtime_error& e) {
		rofcore::logging::error << "[ipcore] addr_in6 create: exception " << e.what() << std::endl;
	}
}

void
cipcore::addr_in6_updated(unsigned int ifindex, uint16_t adindex)
{
	try {

		if (not ltable.has_link(ifindex)) {
			rofcore::logging::error << "[ipcore] addr_in6 update: link ifindex not found: " << ifindex << std::endl;
			return;
		}

		if (not ltable.get_link(ifindex).get_addr_table().has_addr_in6(adindex)) {
			ltable.set_link(ifindex).set_addr_table().set_addr_in6(adindex).install();
		}

	} catch (std::runtime_error& e) {
		rofcore::logging::error << "[ipcore] addr_in6 update: exception " << e.what() << std::endl;
	}
}

void
cipcore::addr_in6_deleted(unsigned int ifindex, uint16_t adindex)
{
	try {

		if (not ltable.has_link(ifindex)) {
			rofcore::logging::error << "[ipcore] addr_in6 delete: link ifindex not found: " << ifindex << std::endl;
			return;
		}

		if (ltable.get_link(ifindex).get_addr_table().has_addr_in6(adindex)) {
			ltable.set_link(ifindex).set_addr_table().set_addr_in6(adindex).uninstall();
			ltable.set_link(ifindex).set_addr_table().drop_addr_in6(adindex);
		}

	} catch (std::runtime_error& e) {
		rofcore::logging::error << "[ipcore] addr_in6 delete: exception " << e.what() << std::endl;
	}
}


void
cipcore::route_in4_created(uint8_t table_id, unsigned int rtindex)
{
	try {
		// ignore local route table and unspecified table_id
		if ((RT_TABLE_LOCAL/*255*/ == table_id) || (RT_TABLE_UNSPEC/*0*/ == table_id)) {
		//if ((RT_TABLE_UNSPEC/*0*/ == table_id)) {
			rofcore::logging::debug << "[ipcore] create route => suppressing table_id=" << (unsigned int)table_id << std::endl;
			return;
		}

		rofl::logging::info << "[ipcore] crtroute CREATE:" << std::endl << rofcore::cnetlink::get_instance().get_routes_in4(table_id).get_route(rtindex);

		if (not rtables.get_table(table_id).has_route_in4(rtindex)) {
			rtables.set_table(table_id).add_route_in4(rtindex).install();
		}

	} catch (rofcore::eNetLinkNotFound& e) {
		rofl::logging::warn << "[ipcore] crtroute CREATE notification rcvd => "
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
			rofcore::logging::debug << "[ipcore] route update => suppressing table_id=" << (unsigned int)table_id << std::endl;
			return;
		}

		rofl::logging::info << "[ipcore] route update:" << std::endl << rofcore::cnetlink::get_instance().get_routes_in4(table_id).get_route(rtindex);

		if (not rtables.get_table(table_id).has_route_in4(rtindex)) {
			rtables.set_table(table_id).add_route_in4(rtindex).install();
		}

		// do nothing here, this event is handled directly by dptroute instance

	} catch (rofcore::eNetLinkNotFound& e) {
		rofl::logging::warn << "[ipcore] crtroute UPDATE notification rcvd => "
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
			rofcore::logging::debug << "[ipcore] route delete => suppressing table_id=" << (unsigned int)table_id << std::endl;
			return;
		}

		rofl::logging::info << "[ipcore] crtroute DELETE:" << std::endl << rofcore::cnetlink::get_instance().get_routes_in4(table_id).get_route(rtindex);

		if (rtables.get_table(table_id).has_route_in4(rtindex)) {
			rtables.set_table(table_id).set_route_in4(rtindex).uninstall();
			rtables.set_table(table_id).drop_route_in4(rtindex);
		}

	} catch (rofcore::eNetLinkNotFound& e) {
		rofl::logging::warn << "[ipcore] crtroute DELETE notification rcvd => "
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
			std::cerr << "ipcore::route_created() => suppressing table_id=" << (unsigned int)table_id << std::endl;
			return;
		}

		rofl::logging::info << "[ipcore] crtroute CREATE:" << std::endl << rofcore::cnetlink::get_instance().get_routes_in6(table_id).get_route(rtindex);

		if (not rtables.get_table(table_id).has_route_in6(rtindex)) {
			rtables.set_table(table_id).add_route_in6(rtindex).install();
		}

	} catch (rofcore::eNetLinkNotFound& e) {
		rofl::logging::warn << "[ipcore] crtroute CREATE notification rcvd => "
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
			rofcore::logging::debug << "[ipcore] route update => suppressing table_id=" << (unsigned int)table_id << std::endl;
			return;
		}

		rofl::logging::info << "[ipcore] route update:" << std::endl << rofcore::cnetlink::get_instance().get_routes_in6(table_id).get_route(rtindex);

		if (not rtables.get_table(table_id).has_route_in6(rtindex)) {
			rtables.set_table(table_id).add_route_in6(rtindex).install();
		}

		// do nothing here, this event is handled directly by dptroute instance

	} catch (rofcore::eNetLinkNotFound& e) {
		rofl::logging::warn << "[ipcore] crtroute UPDATE notification rcvd => "
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
			std::cerr << "ipcore::route_deleted() => suppressing table_id=" << (unsigned int)table_id << std::endl;
			return;
		}

		rofl::logging::info << "[ipcore] crtroute DELETE:" << std::endl << rofcore::cnetlink::get_instance().get_routes_in6(table_id).get_route(rtindex);

		if (rtables.get_table(table_id).has_route_in6(rtindex)) {
			rtables.set_table(table_id).set_route_in6(rtindex).uninstall();
			rtables.set_table(table_id).drop_route_in6(rtindex);
		}

	} catch (rofcore::eNetLinkNotFound& e) {
		rofl::logging::warn << "[ipcore] crtroute DELETE notification rcvd => "
				<< "unable to find crtroute for table-id:" << table_id << " rtindex:" << rtindex << std::endl;
	}
}



void
cipcore::purge_dpt_entries()
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);

		// all wildcard matches with non-strict deletion => removes all state from data path
		rofl::openflow::cofflowmod fe(dpt.get_version());
		fe.set_command(rofl::openflow::OFPFC_DELETE);
		fe.set_table_id(rofl::openflow::base::get_ofptt_all(dpt.get_version()));
		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

	} catch (rofl::eRofDptNotFound& e) {

	}
}



void
cipcore::block_stp_frames()
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);

		rofl::openflow::cofflowmod fe(dpt.get_version());

		fe.set_command(rofl::openflow::OFPFC_ADD);
		fe.set_table_id(0);
		fe.set_match().set_eth_dst(rofl::cmacaddr("01:80:c2:00:00:00"), rofl::cmacaddr("ff:ff:ff:00:00:00"));

		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

	} catch (rofl::eRofDptNotFound& e) {

	}
}



void
cipcore::unblock_stp_frames()
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);

		rofl::openflow::cofflowmod fe(dpt.get_version());

		fe.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		fe.set_table_id(0);
		fe.set_match().set_eth_dst(rofl::cmacaddr("01:80:c2:00:00:00"), rofl::cmacaddr("ff:ff:ff:00:00:00"));

		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

	} catch (rofl::eRofDptNotFound& e) {

	}
}



void
cipcore::redirect_ipv4_multicast()
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);

		rofl::openflow::cofflowmod fe(dpt.get_version());

		fe.set_command(rofl::openflow::OFPFC_ADD);
		fe.set_table_id(0);

		fe.set_match().set_eth_type(rofl::fipv4frame::IPV4_ETHER);
		fe.set_match().set_ipv4_dst(rofl::caddress_in4("224.0.0.0"), rofl::caddress_in4("240.0.0.0"));

		rofl::cindex index(0);

		fe.set_instructions().add_inst_apply_actions().set_actions().add_action_output(index).
				set_port_no(rofl::openflow::base::get_ofpp_controller_port(dpt.get_version()));
		fe.set_instructions().add_inst_apply_actions().set_actions().set_action_output(index).
				set_max_len(ETH_FRAME_LEN);

		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

	} catch (rofl::eRofDptNotFound& e) {

	}
}



void
cipcore::redirect_ipv6_multicast()
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);

		rofl::openflow::cofflowmod fe(dpt.get_version());

		fe.set_command(rofl::openflow::OFPFC_ADD);
		fe.set_table_id(0);

		fe.set_match().set_eth_type(rofl::fipv6frame::IPV6_ETHER);
		fe.set_match().set_ipv6_dst(rofl::caddress_in6("ff00::"), rofl::caddress_in6("ff00::"));

		rofl::cindex index(0);

		fe.set_instructions().add_inst_apply_actions().set_actions().add_action_output(index).
				set_port_no(rofl::openflow::base::get_ofpp_controller_port(dpt.get_version()));
		fe.set_instructions().add_inst_apply_actions().set_actions().set_action_output(index).
				set_max_len(ETH_FRAME_LEN);

		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

	} catch (rofl::eRofDptNotFound& e) {

	}
}



void
cipcore::hook_dpt_attach()
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);

		std::vector<std::string> argv;
		std::vector<std::string> envp;

		std::stringstream s_dpid;
		s_dpid << "DPID=" << dpt.get_dpid();
		envp.push_back(s_dpid.str());

		cipcore::execute(cipcore::script_path_dpt_open, argv, envp);

	} catch (rofl::eRofDptNotFound& e) {

	}
}



void
cipcore::hook_dpt_detach()
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);

		std::vector<std::string> argv;
		std::vector<std::string> envp;

		std::stringstream s_dpid;
		s_dpid << "DPID=" << dpt.get_dpid();
		envp.push_back(s_dpid.str());

		cipcore::execute(cipcore::script_path_dpt_close, argv, envp);

	} catch (rofl::eRofDptNotFound& e) {

	}
}



void
cipcore::hook_port_up(std::string const& devname)
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);

		std::vector<std::string> argv;
		std::vector<std::string> envp;

		std::stringstream s_dpid;
		s_dpid << "DPID=" << dpt.get_dpid();
		envp.push_back(s_dpid.str());

		std::stringstream s_devname;
		s_devname << "DEVNAME=" << devname;
		envp.push_back(s_devname.str());

		cipcore::execute(cipcore::script_path_port_up, argv, envp);

	} catch (rofl::eRofDptNotFound& e) {

	}
}



void
cipcore::hook_port_down(std::string const& devname)
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);

		std::vector<std::string> argv;
		std::vector<std::string> envp;

		std::stringstream s_dpid;
		s_dpid << "DPID=" << dpt.get_dpid();
		envp.push_back(s_dpid.str());

		std::stringstream s_devname;
		s_devname << "DEVNAME=" << devname;
		envp.push_back(s_devname.str());

		cipcore::execute(cipcore::script_path_port_down, argv, envp);

	} catch (rofl::eRofDptNotFound& e) {

	}
}



void
cipcore::execute(
		std::string const& executable,
		std::vector<std::string> argv,
		std::vector<std::string> envp)
{
	pid_t pid = 0;

	if ((pid = fork()) < 0) {
		fprintf(stderr, "syscall error fork(): %d (%s)\n", errno, strerror(errno));
		return;
	}

	if (pid > 0) { // father process
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




