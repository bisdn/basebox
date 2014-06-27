#include "ipcore.h"

using namespace dptmap;

std::string	ipcore::dpath_open_script_path(DEFAULT_DPATH_OPEN_SCRIPT_PATH);
std::string	ipcore::dpath_close_script_path(DEFAULT_DPATH_CLOSE_SCRIPT_PATH);
std::string	ipcore::port_up_script_path(DEFAULT_PORT_UP_SCRIPT_PATH);
std::string	ipcore::port_down_script_path(DEFAULT_PORT_DOWN_SCRIPT_PATH);


ipcore::ipcore(
		rofl::openflow::cofhello_elem_versionbitmap const& versionbitmap) :
		dpt(0),
		dump_state_interval(15),
		rofl::crofbase(versionbitmap)
{
	register_timer(IPCORE_TIMER_DUMP, dump_state_interval);
}


ipcore::~ipcore()
{
	if (0 != dpt) {
		delete_all_routes();

		delete_all_ports();
	}
}



void
ipcore::handle_timeout(int opaque, void *data)
{
	switch (opaque) {
	case IPCORE_TIMER_DUMP: {
		dump_state();
	} break;
	default:
		crofbase::handle_timeout(opaque);
	}
}



bool
ipcore::link_is_mapped_from_dpt(int ifindex)
{
	std::map<uint32_t, dptlink*>::const_iterator it;
	for (it = dptlinks[dpt].begin(); it != dptlinks[dpt].end(); ++it) {
		if (it->second->get_ifindex() == ifindex) {
			return true;
		}
	}
	return false;
}



dptlink&
ipcore::get_mapped_link_from_dpt(int ifindex)
{
	std::map<uint32_t, dptlink*>::const_iterator it;
	for (it = dptlinks[dpt].begin(); it != dptlinks[dpt].end(); ++it) {
		if (it->second->get_ifindex() == ifindex) {
			return *(it->second);
		}
	}
	throw eVmCoreNotFound();
}



void
ipcore::delete_all_ports()
{
	for (std::map<rofl::crofdpt*, std::map<uint32_t, dptlink*> >::iterator
			jt = dptlinks.begin(); jt != dptlinks.end(); ++jt) {
		for (std::map<uint32_t, dptlink*>::iterator
				it = dptlinks[jt->first].begin(); it != dptlinks[jt->first].end(); ++it) {
			run_port_down_script(it->second->get_devname());
			delete (it->second); // remove dptport instance from heap
		}
	}
	dptlinks.clear();
}



void
ipcore::delete_all_routes()
{
	for (std::map<uint8_t, std::map<unsigned int, dptroute*> >::iterator
			it = dptroutes.begin(); it != dptroutes.end(); ++it) {
		for (std::map<unsigned int, dptroute*>::iterator
				jt = it->second.begin(); jt != it->second.end(); ++jt) {
			delete (jt->second); // remove dptroute instance from heap
		}
	}
	dptroutes.clear();
}



void
ipcore::handle_dpath_open(
		rofl::crofdpt& dpt)
{
	try {
		this->dpt = &dpt;

		/*
		 * remove all routes
		 */
		delete_all_routes();

		/*
		 * remove all old pending tap devices
		 */
		delete_all_ports();

		/*
		 * purge all entries from data path
		 */
		rofl::openflow::cofflowmod fe(dpt.get_version());
		fe.set_command(rofl::openflow::OFPFC_DELETE);
		fe.set_table_id(rofl::openflow::base::get_ofptt_all(dpt.get_version()));
		dpt.send_flow_mod_message(fe);

		/*
		 * block all STP related frames for now
		 */
		block_stp_frames();

		/*
		 * default: redirect multicast traffic always to controller
		 */
		redirect_ipv4_multicast();
		redirect_ipv6_multicast();

		// TODO: how many data path elements are allowed to connect to ourselves? only one makes sense ...


		/*
		 * create new tap devices for (reconnecting?) data path
		 */
		std::map<uint32_t, rofl::openflow::cofport*> ports = dpt.get_ports().get_ports();
		for (std::map<uint32_t, rofl::openflow::cofport*>::iterator
				it = ports.begin(); it != ports.end(); ++it) {
			rofl::openflow::cofport *port = it->second;
			if (dptlinks[&dpt].find(port->get_port_no()) == dptlinks[&dpt].end()) {
				dptlinks[&dpt][port->get_port_no()] = new dptlink(this, &dpt, port->get_port_no());
				run_port_up_script(port->get_name());
			}
		}

		// get full-length packets (what is the ethernet max length on dpt?)
		dpt.send_set_config_message(0, 1518);

		/*
		 * install default FlowMod entry for table 0 => GotoTable(1)
		 */
		rofl::openflow::cofflowmod fed = rofl::openflow::cofflowmod(dpt.get_version());

		fed.set_command(rofl::openflow::OFPFC_ADD);
		fed.set_table_id(0);
		fed.set_idle_timeout(0);
		fed.set_hard_timeout(0);
		fed.set_priority(0); // lowest priority
		fed.instructions.add_inst_goto_table().set_table_id(1);

		dpt.send_flow_mod_message(fed);

		run_dpath_open_script();

	} catch (eNetDevCritical& e) {

		fprintf(stderr, "ipcore::handle_dpath_open() unable to create tap device\n");
		throw;

	}
}



void
ipcore::handle_dpath_close(
		rofl::crofdpt& dpt)
{
	run_dpath_close_script();

	delete_all_routes();

	delete_all_ports();

	this->dpt = (rofl::crofdpt*)0;
}



void
ipcore::handle_port_status(
		rofl::crofdpt& dpt,
		rofl::openflow::cofmsg_port_status& msg,
		uint8_t aux_id)
{
	if (this->dpt != &dpt) {
		fprintf(stderr, "ipcore::handle_port_stats() received PortStatus from invalid data path\n");
		return;
	}

	uint32_t port_no = msg.get_port().get_port_no();

	rofl::logging::info << "[ipcore] Port-Status message rcvd:" << std::endl << msg;

	try {
		switch (msg.get_reason()) {
		case rofl::openflow::OFPPR_ADD: {
			if (dptlinks[&dpt].find(port_no) == dptlinks[&dpt].end()) {
				dptlinks[&dpt][port_no] = new dptlink(this, &dpt, msg.get_port().get_port_no());
				run_port_up_script(msg.get_port().get_name());
				dptlinks[&dpt][port_no]->open();
			}
		} break;
		case rofl::openflow::OFPPR_MODIFY: {
			if (dptlinks[&dpt].find(port_no) != dptlinks[&dpt].end()) {
				dptlinks[&dpt][port_no]->handle_port_status();
				//run_port_up_script(msg->get_port().get_name()); // TODO: check flags
			}
		} break;
		case rofl::openflow::OFPPR_DELETE: {
			if (dptlinks[&dpt].find(port_no) != dptlinks[&dpt].end()) {
				dptlinks[&dpt][port_no]->close();
				delete dptlinks[&dpt][port_no];
				dptlinks[&dpt].erase(port_no);
				run_port_down_script(msg.get_port().get_name());
			}
		} break;
		default: {

			fprintf(stderr, "ipcore::handle_port_status() message with invalid reason code received, ignoring\n");

		} break;
		}

	} catch (eNetDevCritical& e) {

		// TODO: handle error condition appropriately, for now: rethrow
		throw;

	}
}



void
ipcore::handle_packet_out(rofl::crofctl& ctl, rofl::openflow::cofmsg_packet_out& msg, uint8_t aux_id)
{

}




void
ipcore::handle_packet_in(rofl::crofdpt& dpt, rofl::openflow::cofmsg_packet_in& msg, uint8_t aux_id)
{
	try {
		uint32_t port_no = msg.get_match().get_in_port();

		if (dptlinks[&dpt].find(port_no) == dptlinks[&dpt].end()) {
			fprintf(stderr, "ipcore::handle_packet_in() frame for port_no=%d received, but port not found\n", port_no);

			return;
		}

		dptlinks[&dpt][port_no]->handle_packet_in(msg.get_packet());

		if (rofl::openflow::base::get_ofp_no_buffer(dpt.get_version()) != msg.get_buffer_id()) {
			rofl::openflow::cofactions actions(dpt.get_version());
			dpt.send_packet_out_message(msg.get_buffer_id(), msg.get_match().get_in_port(), actions);
		}

	} catch (ePacketPoolExhausted& e) {

		fprintf(stderr, "ipcore::handle_packet_in() packetpool exhausted\n");

	}
}



void
ipcore::handle_error(rofl::crofdpt& dpt, rofl::openflow::cofmsg_error& msg, uint8_t aux_id)
{
	rofl::logging::warn << "[ipcore] error message rcvd:" << std::endl << msg;
}



void
ipcore::handle_flow_removed(rofl::crofdpt& dpt, rofl::openflow::cofmsg_flow_removed& msg, uint8_t aux_id)
{
	rofl::logging::info << "[ipcore] Flow-Removed message rcvd:" << std::endl << msg;
}



void
ipcore::route_created(uint8_t table_id, unsigned int rtindex)
{
	try {
		if (0 == dpt)
			return;

		// ignore local route table and unspecified table_id
		if ((RT_TABLE_LOCAL/*255*/ == table_id) || (RT_TABLE_UNSPEC/*0*/ == table_id)) {
		//if ((RT_TABLE_UNSPEC/*0*/ == table_id)) {
			std::cerr << "ipcore::route_created() => suppressing table_id=" << (unsigned int)table_id << std::endl;
			return;
		}

		rofl::logging::info << "[ipcore] crtroute CREATE:" << std::endl << cnetlink::get_instance().get_route(table_id, rtindex);

		if (dptroutes[table_id].find(rtindex) == dptroutes[table_id].end()) {
			dptroutes[table_id][rtindex] = new dptroute(this, dpt, table_id, rtindex);
			if (dpt->get_channel().is_established()) {
				dptroutes[table_id][rtindex]->open();
			}
		}

	} catch (eNetLinkNotFound& e) {
		rofl::logging::warn << "[ipcore] crtroute CREATE notification rcvd => "
				<< "unable to find crtroute for table-id:" << table_id << " rtindex:" << rtindex << std::endl;
	}
}



void
ipcore::route_updated(uint8_t table_id, unsigned int rtindex)
{
	try {
		if (0 == dpt)
			return;

		// ignore local route table and unspecified table_id
		if ((RT_TABLE_LOCAL/*255*/ == table_id) || (RT_TABLE_UNSPEC/*0*/ == table_id)) {
		//if ((RT_TABLE_UNSPEC/*0*/ == table_id)) {
			std::cerr << "ipcore::route_updated() => suppressing table_id=" << (unsigned int)table_id << std::endl;
			return;
		}

		rofl::logging::info << "[ipcore] crtroute UPDATE:" << std::endl << cnetlink::get_instance().get_route(table_id, rtindex);

		//std::cerr << "ipcore::route_updated() " << cnetlink::get_instance().get_route(table_id, rtindex) << std::endl;
		if (dptroutes[table_id].find(rtindex) == dptroutes[table_id].end()) {
			route_created(table_id, rtindex);
		}

		// do nothing here, this event is handled directly by dptroute instance

	} catch (eNetLinkNotFound& e) {
		rofl::logging::warn << "[ipcore] crtroute UPDATE notification rcvd => "
				<< "unable to find crtroute for table-id:" << table_id << " rtindex:" << rtindex << std::endl;
	}
}



void
ipcore::route_deleted(uint8_t table_id, unsigned int rtindex)
{
	try {
		if (0 == dpt)
			return;

		// ignore local route table and unspecified table_id
		if ((RT_TABLE_LOCAL/*255*/ == table_id) || (RT_TABLE_UNSPEC/*0*/ == table_id)) {
		//if ((RT_TABLE_UNSPEC/*0*/ == table_id)) {
			std::cerr << "ipcore::route_deleted() => suppressing table_id=" << (unsigned int)table_id << std::endl;
			return;
		}

		rofl::logging::info << "[ipcore] crtroute DELETE:" << std::endl << cnetlink::get_instance().get_route(table_id, rtindex);

		//std::cerr << "ipcore::route_deleted() " << cnetlink::get_instance().get_route(table_id, rtindex) << std::endl;
		if (dptroutes[table_id].find(rtindex) != dptroutes[table_id].end()) {
			if (dpt->get_channel().is_established()) {
				dptroutes[table_id][rtindex]->close();
			}
			delete dptroutes[table_id][rtindex];
			dptroutes[table_id].erase(rtindex);
		}

	} catch (eNetLinkNotFound& e) {
		rofl::logging::warn << "[ipcore] crtroute DELETE notification rcvd => "
				<< "unable to find crtroute for table-id:" << table_id << " rtindex:" << rtindex << std::endl;
	}
}



void
ipcore::block_stp_frames()
{
	if (0 == dpt)
		return;

	rofl::openflow::cofflowmod fe(dpt->get_version());

	fe.set_command(rofl::openflow::OFPFC_ADD);
	fe.set_table_id(0);
	fe.match.set_eth_dst(rofl::cmacaddr("01:80:c2:00:00:00"), rofl::cmacaddr("ff:ff:ff:00:00:00"));

	dpt->send_flow_mod_message(fe);
}



void
ipcore::unblock_stp_frames()
{
	if (0 == dpt)
		return;

	rofl::openflow::cofflowmod fe(dpt->get_version());

	fe.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
	fe.set_table_id(0);
	fe.match.set_eth_dst(rofl::cmacaddr("01:80:c2:00:00:00"), rofl::cmacaddr("ff:ff:ff:00:00:00"));

	dpt->send_flow_mod_message(fe);
}



void
ipcore::redirect_ipv4_multicast()
{
	if (0 == dpt)
		return;

	rofl::openflow::cofflowmod fe(dpt->get_version());

	fe.set_command(rofl::openflow::OFPFC_ADD);
	fe.set_table_id(0);
	fe.match.set_eth_type(rofl::fipv4frame::IPV4_ETHER);
	fe.match.set_ipv4_dst(rofl::caddress(AF_INET, "224.0.0.0"), rofl::caddress(AF_INET, "240.0.0.0"));
	fe.instructions.add_inst_apply_actions().get_actions().append_action_output(
			rofl::openflow::base::get_ofpp_controller_port(dpt->get_version()), 1518);

	dpt->send_flow_mod_message(fe);
}



void
ipcore::redirect_ipv6_multicast()
{
	if (0 == dpt)
		return;

	rofl::openflow::cofflowmod fe(dpt->get_version());

	fe.set_command(rofl::openflow::OFPFC_ADD);
	fe.set_table_id(0);
	fe.match.set_eth_type(rofl::fipv6frame::IPV6_ETHER);
	fe.match.set_ipv6_dst(rofl::caddress(AF_INET6, "ff00::"), rofl::caddress(AF_INET6, "ff00::"));
	fe.instructions.add_inst_apply_actions().get_actions().append_action_output(
			rofl::openflow::base::get_ofpp_controller_port(dpt->get_version()), 1518);

	dpt->send_flow_mod_message(fe);
}



void
ipcore::run_dpath_open_script()
{
	std::vector<std::string> argv;
	std::vector<std::string> envp;

	std::stringstream s_dpid;
	s_dpid << "DPID=" << dpt->get_dpid();
	envp.push_back(s_dpid.str());

	ipcore::execute(ipcore::dpath_open_script_path, argv, envp);
}



void
ipcore::run_dpath_close_script()
{
	std::vector<std::string> argv;
	std::vector<std::string> envp;

	std::stringstream s_dpid;
	s_dpid << "DPID=" << dpt->get_dpid();
	envp.push_back(s_dpid.str());

	ipcore::execute(ipcore::dpath_close_script_path, argv, envp);
}



void
ipcore::run_port_up_script(std::string const& devname)
{
	std::vector<std::string> argv;
	std::vector<std::string> envp;

	std::stringstream s_dpid;
	s_dpid << "DPID=" << dpt->get_dpid();
	envp.push_back(s_dpid.str());

	std::stringstream s_devname;
	s_devname << "DEVNAME=" << devname;
	envp.push_back(s_devname.str());

	ipcore::execute(ipcore::port_up_script_path, argv, envp);
}



void
ipcore::run_port_down_script(std::string const& devname)
{
	std::vector<std::string> argv;
	std::vector<std::string> envp;

	std::stringstream s_dpid;
	s_dpid << "DPID=" << dpt->get_dpid();
	envp.push_back(s_dpid.str());

	std::stringstream s_devname;
	s_devname << "DEVNAME=" << devname;
	envp.push_back(s_devname.str());

	ipcore::execute(ipcore::port_down_script_path, argv, envp);
}



void
ipcore::execute(
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



void
ipcore::dump_state()
{
	register_timer(IPCORE_TIMER_DUMP, dump_state_interval);

	for (std::map<rofl::crofdpt*, std::map<uint32_t, dptlink*> >::iterator
			it = dptlinks.begin(); it != dptlinks.end(); ++it) {

		rofl::crofdpt *dpt = it->first;

		std::cerr << "data path <dpid: " << dpt->get_dpid_s() << "> => " << std::endl;

		for (std::map<uint32_t, dptlink*>::iterator
				jt = dptlinks[dpt].begin(); jt != dptlinks[dpt].end(); ++jt) {

			std::cerr << "  " << *(jt->second) << std::endl;
		}

		std::cerr << std::endl;

		// FIXME: routes should be handled by data path

		for (std::map<uint8_t, std::map<unsigned int, dptroute*> >::iterator
				jt = dptroutes.begin(); jt != dptroutes.end(); ++jt) {

			uint8_t scope = jt->first;

			for (std::map<unsigned int, dptroute*>::iterator
					kt = dptroutes[scope].begin(); kt != dptroutes[scope].end(); ++kt) {

				std::cerr << "  " << (*kt->second) << std::endl;
			}
		}

		std::cerr << std::endl;
	}
}

