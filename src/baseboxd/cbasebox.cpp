/*
 * crofbase.cpp
 *
 *  Created on: 05.08.2014
 *      Author: andreas
 */

#include "cbasebox.hpp"

using namespace basebox;

/*static*/cbasebox* cbasebox::rofbase = (cbasebox*)0;
/*static*/bool cbasebox::keep_on_running = true;
/*static*/const std::string cbasebox::BASEBOX_LOG_FILE = std::string("/var/log/baseboxd.log");
/*static*/const std::string cbasebox::BASEBOX_PID_FILE = std::string("/var/run/baseboxd.pid");
/*static*/const std::string cbasebox::BASEBOX_CONFIG_FILE = std::string("/usr/local/etc/baseboxd.conf");

/*static*/std::string cbasebox::script_path_dpt_open 	= std::string("/var/lib/basebox/dpath-open.sh");
/*static*/std::string cbasebox::script_path_dpt_close 	= std::string("/var/lib/basebox/dpath-close.sh");
/*static*/std::string cbasebox::script_path_port_up 	= std::string("/var/lib/basebox/port-up.sh");
/*static*/std::string cbasebox::script_path_port_down 	= std::string("/var/lib/basebox/port-down.sh");

/*static*/
int
cbasebox::run(int argc, char** argv)
{
	rofl::cunixenv env_parser(argc, argv);

	/* update defaults */
	//env_parser.update_default_option("logfile", ETHCORE_LOG_FILE);
	//env_parser.update_default_option("config-file", ETHCORE_CONFIG_FILE);
	env_parser.add_option(rofl::coption(true, NO_ARGUMENT,'D',"daemonize","daemonize",""));
	env_parser.add_option(rofl::coption(true, REQUIRED_ARGUMENT, 'l', "logfile", "set log-file", std::string(BASEBOX_LOG_FILE)));
	env_parser.add_option(rofl::coption(true, REQUIRED_ARGUMENT, 'c', "config-file", "set config-file", std::string(BASEBOX_CONFIG_FILE)));
	env_parser.add_option(rofl::coption(true, REQUIRED_ARGUMENT, 'i', "pidfile", "set pid-file", std::string(BASEBOX_PID_FILE)));
	env_parser.add_option(rofl::coption(true, REQUIRED_ARGUMENT, 'p', "port", "set port", "6653"));

	// command line arguments
	env_parser.parse_args();

	// configuration file
	ethcore::cconfig::get_instance().open(env_parser.get_arg("config-file"));


	/*
	 * read configuration file for roflibs related configuration
	 */
	roflibs::eth::cportdb_file& portdb =
			dynamic_cast<roflibs::eth::cportdb_file&>( roflibs::eth::cportdb::get_portdb("file") );
	portdb.read_config(env_parser.get_arg("config-file"), std::string("baseboxd"));

	/*
	 * extract debug level
	 */
	int rofl_debug = 0;
	if (ethcore::cconfig::get_instance().exists("baseboxd.daemon.logging.rofl.debug")) {
		rofl_debug = (int)ethcore::cconfig::get_instance().lookup("baseboxd.daemon.logging.rofl.debug");
	}
	int core_debug = 0;
	if (ethcore::cconfig::get_instance().exists("baseboxd.daemon.logging.core.debug")) {
		core_debug = (int)ethcore::cconfig::get_instance().lookup("baseboxd.daemon.logging.core.debug");
	}
	if (env_parser.is_arg_set("debug")) {
		rofl_debug = core_debug = atoi(env_parser.get_arg("debug").c_str());
	}

	/*
	 * extract log-file
	 */
	std::string logfile;
	if (env_parser.is_arg_set("logfile")) {
		logfile = env_parser.get_arg("logfile");
	} else if (ethcore::cconfig::get_instance().exists("baseboxd.daemon.logfile")) {
		logfile = (const char*)ethcore::cconfig::get_instance().lookup("baseboxd.daemon.logfile");
	} else {
		logfile = std::string(BASEBOX_LOG_FILE); // default
	}

	/*
	 * extract pid-file
	 */
	std::string pidfile;
	if (env_parser.is_arg_set("pidfile")) {
		pidfile = env_parser.get_arg("pidfile");
	} else if (ethcore::cconfig::get_instance().exists("baseboxd.daemon.pidfile")) {
		pidfile = (const char*)ethcore::cconfig::get_instance().lookup("baseboxd.daemon.pidfile");
	} else {
		pidfile = std::string(BASEBOX_PID_FILE); // default
	}

	/*
	 * extract daemonize flag
	 */
	bool daemonize = true;
	if (env_parser.is_arg_set("daemonize")) {
		daemonize = atoi(env_parser.get_arg("pidfile").c_str());
	} else if (ethcore::cconfig::get_instance().exists("baseboxd.daemon.daemonize")) {
		daemonize = (bool)ethcore::cconfig::get_instance().lookup("baseboxd.daemon.daemonize");
	} else {
		daemonize = true; // default
	}



	/*
	 * daemonize
	 */
	if (daemonize) {
		rofl::cdaemon::daemonize(pidfile, logfile);
	}
	rofl::logging::set_debug_level(rofl_debug);
	rofcore::logging::set_debug_level(core_debug);

	if (daemonize) {
		rofcore::logging::notice << "[baseboxd][main] daemonizing successful" << std::endl;
	}



	/*
	 * prepare OpenFlow socket for listening
	 */
	rofl::openflow::cofhello_elem_versionbitmap versionbitmap;
	if (ethcore::cconfig::get_instance().exists("baseboxd.openflow.version")) {
		int ofp_version = (int)ethcore::cconfig::get_instance().lookup("baseboxd.openflow.version");
		ofp_version = (ofp_version < rofl::openflow13::OFP_VERSION) ? rofl::openflow13::OFP_VERSION : ofp_version;
		versionbitmap.add_ofp_version(ofp_version);
	} else {
		versionbitmap.add_ofp_version(rofl::openflow13::OFP_VERSION);
	}

	rofcore::logging::notice << "[baseboxd][main] using OpenFlow version-bitmap:" << std::endl << versionbitmap;

	basebox::cbasebox& box = basebox::cbasebox::get_instance(versionbitmap);

	//base.init(/*port-table-id=*/0, /*fib-in-table-id=*/1, /*fib-out-table-id=*/2, /*default-vid=*/1);

	std::stringstream portno;
	if (ethcore::cconfig::get_instance().exists("baseboxd.openflow.bindport")) {
		portno << (int)ethcore::cconfig::get_instance().lookup("baseboxd.openflow.bindport");
	} else {
		portno << (int)6653;
	}

	std::stringstream bindaddr;
	if (ethcore::cconfig::get_instance().exists("baseboxd.openflow.bindaddr")) {
		bindaddr << (const char*)ethcore::cconfig::get_instance().lookup("baseboxd.openflow.bindaddr");
	} else {
		bindaddr << "::";
	}

	enum rofl::csocket::socket_type_t socket_type = rofl::csocket::SOCKET_TYPE_PLAIN;
	rofl::cparams socket_params = rofl::csocket::get_default_params(socket_type);
	socket_params.set_param(rofl::csocket::PARAM_KEY_LOCAL_PORT).set_string(portno.str());
	socket_params.set_param(rofl::csocket::PARAM_KEY_LOCAL_HOSTNAME).set_string(bindaddr.str());

	box.rpc_listen_for_dpts(socket_type, socket_params);


	/*
	 * extract python script to execute as business/use case specific logic
	 */
	if (ethcore::cconfig::get_instance().exists("baseboxd.usecase.script")) {
		std::string script = (const char*)ethcore::cconfig::get_instance().lookup("baseboxd.usecase.script");
		box.set_python_script(script);

		if (not script.empty()) {
			roflibs::python::cpython::get_instance().run(script);
		}
	}

	cbasebox::keep_on_running = true;
	while (cbasebox::keep_on_running) {
		try {
			/*
			 * start main loop, does not return
			 */
			rofl::cioloop::run();

		} catch (std::runtime_error& e) {
			rofcore::logging::error << "[cbasebox][run] caught exception in main loop, e.what() " << e.what() << std::endl;
		}
	}

	rofl::cioloop::shutdown();

	return 0;
}


/*static*/
void
cbasebox::stop()
{
	cbasebox::keep_on_running = false;
}



void
cbasebox::enqueue(rofcore::cnetdev *netdev, rofl::cpacket* pkt)
{
	try {
		rofcore::ctapdev* tapdev = dynamic_cast<rofcore::ctapdev*>( netdev );
		if (0 == tapdev) {
			throw eLinkTapDevNotFound("cbasebox::enqueue() tap device not found");
		}

		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(tapdev->get_dpid());

		if (not dpt.get_channel().is_established()) {
			throw eLinkNoDptAttached("cbasebox::enqueue() dpt not found");
		}

		rofl::openflow::cofactions actions(dpt.get_version());
		actions.set_action_push_vlan(rofl::cindex(0)).set_eth_type(rofl::fvlanframe::VLAN_CTAG_ETHER);
		actions.set_action_set_field(rofl::cindex(1)).set_oxm(rofl::openflow::coxmatch_ofb_vlan_vid(tapdev->get_pvid()));
		actions.set_action_output(rofl::cindex(2)).set_port_no(rofl::openflow::OFPP_TABLE);

		dpt.send_packet_out_message(
				rofl::cauxid(0),
				rofl::openflow::base::get_ofp_no_buffer(dpt.get_version()),
				rofl::openflow::base::get_ofpp_controller_port(dpt.get_version()),
				actions,
				pkt->soframe(),
				pkt->length());

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[cbasebox][enqueue] no data path attached, dropping outgoing packet" << std::endl;

	} catch (eLinkNoDptAttached& e) {
		rofcore::logging::error << "[cbasebox][enqueue] no data path attached, dropping outgoing packet" << std::endl;

	} catch (eLinkTapDevNotFound& e) {
		rofcore::logging::error << "[cbasebox][enqueue] unable to find tap device" << std::endl;
	}

	rofcore::cpacketpool::get_instance().release_pkt(pkt);
}



void
cbasebox::enqueue(rofcore::cnetdev *netdev, std::vector<rofl::cpacket*> pkts)
{
	for (std::vector<rofl::cpacket*>::iterator
			it = pkts.begin(); it != pkts.end(); ++it) {
		enqueue(netdev, *it);
	}
}




void
cbasebox::handle_dpt_open(
		rofl::crofdpt& dpt) {

	dpt.flow_mod_reset();
	dpt.group_mod_reset();
	dpt.send_port_desc_stats_request(rofl::cauxid(0), 0);

	// call external scripting hook
	hook_dpt_attach(dpt);
}



void
cbasebox::handle_dpt_close(
		rofl::crofdpt& dpt) {

	clear_tap_devs(dpt.get_dpid());

	// call external scripting hook
	hook_dpt_detach(dpt);

	roflibs::svc::cflowcore::set_flow_core(dpt.get_dpid()).handle_dpt_close(dpt);
	roflibs::gre::cgrecore::set_gre_core(dpt.get_dpid()).handle_dpt_close(dpt);
	roflibs::gtp::cgtprelay::set_gtp_relay(dpt.get_dpid()).handle_dpt_close(dpt);
	roflibs::gtp::cgtpcore::set_gtp_core(dpt.get_dpid()).handle_dpt_close(dpt);
	roflibs::eth::cethcore::set_eth_core(dpt.get_dpid()).handle_dpt_close(dpt);
	roflibs::ip::cipcore::set_ip_core(dpt.get_dpid()).handle_dpt_close(dpt);

#if 0
	roflibs::gre::cgrecore::set_gre_core(dpt.get_dpid()).clear_gre_terms_in4();
	roflibs::gre::cgrecore::set_gre_core(dpt.get_dpid()).clear_gre_terms_in6();
#endif
}



void
cbasebox::handle_packet_in(
		rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_packet_in& msg) {

	try {
		rofcore::logging::debug << "[cbasebox][handle_packet_in] pkt received: " << std::endl << msg.get_packet();

		if (msg.get_table_id() == table_id_eth_src) {

			if (roflibs::eth::cethcore::has_eth_core(dpt.get_dpid())) {
				roflibs::eth::cethcore::set_eth_core(dpt.get_dpid()).handle_packet_in(dpt, auxid, msg);
			}
		} else
		if ((msg.get_table_id() >= table_id_eth_local) &&
			(msg.get_table_id() <= table_id_ip_fwd)) {

			if (not msg.get_match().get_eth_dst().is_multicast()) {
				/* non-multicast frames are directly injected into a tapdev */
				rofl::cpacket *pkt = rofcore::cpacketpool::get_instance().acquire_pkt();
				*pkt = msg.get_packet();
				pkt->pop(sizeof(struct rofl::fetherframe::eth_hdr_t)-sizeof(uint16_t), sizeof(struct rofl::fvlanframe::vlan_hdr_t));
				set_tap_dev(dpt.get_dpid(), msg.get_match().get_eth_dst()).enqueue(pkt);

			} else {
				/* multicast frames carry a metadata field with the VLAN id
				 * for both tagged and untagged frames. Lookup all tapdev
				 * instances belonging to the specified vid and clone the packet
				 * for all of them. */

				if (not msg.get_match().has_vlan_vid()) {
					// no VLAN related metadata found, ignore packet
					return;
				}

				uint16_t vid = msg.get_match().get_vlan_vid() & 0x0fff;

				for (std::map<std::string, rofcore::ctapdev*>::iterator
						it = devs[dpt.get_dpid()].begin(); it != devs[dpt.get_dpid()].end(); ++it) {
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

		}


	} catch (rofl::openflow::eOxmNotFound& e) {
		// TODO: log error
	}

	// drop buffer on data path, if the packet was stored there, as it is consumed entirely by the underlying kernel
	if (rofl::openflow::base::get_ofp_no_buffer(dpt.get_version()) != msg.get_buffer_id()) {
		dpt.drop_buffer(auxid, msg.get_buffer_id());
	}
}



void
cbasebox::handle_flow_removed(
		rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_flow_removed& msg) {
	switch (msg.get_table_id()) {
	case 3:
	case 4: {
		roflibs::ip::cipcore::set_ip_core(dpt.get_dpid()).handle_flow_removed(dpt, auxid, msg);
	} break;
	default: {
		if (roflibs::eth::cethcore::has_eth_core(dpt.get_dpid())) {
			roflibs::eth::cethcore::set_eth_core(dpt.get_dpid()).handle_flow_removed(dpt, auxid, msg);
		}
	};
	}
}



void
cbasebox::handle_port_status(
		rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_port_status& msg) {
	try {
		const rofl::openflow::cofport& port = msg.get_port();
		uint32_t ofp_port_no = msg.get_port().get_port_no();

		switch (msg.get_reason()) {
		case rofl::openflow::OFPPR_ADD: {
			dpt.set_ports().set_port(msg.get_port().get_port_no()) = msg.get_port();
			hook_port_up(dpt, msg.get_port().get_name());

#if 0
			if (not has_tap_dev(port.get_name())) {
				add_tap_dev(port.get_name(), 0, port.get_hwaddr());
			}

			if (port.link_state_is_link_down() || port.config_is_port_down()) {
				set_tap_dev(port.get_name()).disable_interface();
			} else {
				set_tap_dev(port.get_name()).enable_interface();
			}
#endif

		} break;
		case rofl::openflow::OFPPR_MODIFY: {
			dpt.set_ports().set_port(msg.get_port().get_port_no()) = msg.get_port();

#if 0
			if (not has_tap_dev(port.get_name())) {
				add_tap_dev(port.get_name(), 0, port.get_hwaddr());
			}

			if (port.link_state_is_link_down() || port.config_is_port_down()) {
				set_tap_dev(port.get_name()).disable_interface();
			} else {
				set_tap_dev(port.get_name()).enable_interface();
			}
#endif

		} break;
		case rofl::openflow::OFPPR_DELETE: {
			dpt.set_ports().drop_port(msg.get_port().get_name());
			hook_port_down(dpt, msg.get_port().get_name());
#if 0
			drop_tap_dev(port.get_name());
#endif

		} break;
		default: {
			// invalid/unknown reason
		} return;
		}

		if (roflibs::eth::cethcore::has_eth_core(dpt.get_dpid())) {
			roflibs::eth::cethcore::set_eth_core(dpt.get_dpid()).handle_port_status(dpt, auxid, msg);
		}

	} catch (rofl::openflow::ePortNotFound& e) {
		// TODO: log error
	} catch (rofcore::eNetDevCritical& e) {
		rofcore::logging::debug << "[cbasebox] new port created: unable to create tap device: " << msg.get_port().get_name() << std::endl;
	}
}



void
cbasebox::handle_error_message(
		rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_error& msg) {
	roflibs::ip::cipcore::set_ip_core(dpt.get_dpid()).handle_error_message(dpt, auxid, msg);
	if (roflibs::eth::cethcore::has_eth_core(dpt.get_dpid())) {
		roflibs::eth::cethcore::set_eth_core(dpt.get_dpid()).handle_error_message(dpt, auxid, msg);
	}
}



void
cbasebox::handle_port_desc_stats_reply(
		rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_port_desc_stats_reply& msg) {

	dpt.set_ports() = msg.get_ports();

	roflibs::svc::cflowcore::set_flow_core(dpt.get_dpid()).handle_dpt_close(dpt);

	roflibs::ip::cipcore::set_ip_core(dpt.get_dpid(),
												table_id_ip_local,
												table_id_ip_fwd).handle_dpt_close(dpt);

	roflibs::eth::cethcore::set_eth_core(dpt.get_dpid(),
												table_id_eth_port_membership,
												table_id_eth_src,
												table_id_eth_local,
												table_id_eth_dst,
												default_pvid).handle_dpt_close(dpt);

	roflibs::gtp::cgtpcore::set_gtp_core(dpt.get_dpid(),
												table_id_ip_local,
												table_id_gtp_local).handle_dpt_close(dpt); // yes, same as local for cipcore

	roflibs::gtp::cgtprelay::set_gtp_relay(dpt.get_dpid(),
												table_id_ip_local).handle_dpt_close(dpt); // yes, same as local for cipcore


	roflibs::gre::cgrecore::set_gre_core(dpt.get_dpid(),
												table_id_eth_port_membership,
												table_id_ip_local,
												table_id_gre_local,
												table_id_ip_fwd).handle_dpt_close(dpt);


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

	rofl::openflow::cofflowmod fm(dpt.get_version());
	fm.set_table_id(table_id_svc_flows);
	fm.set_priority(0);
	fm.set_instructions().set_inst_goto_table().set_table_id(table_id_svc_flows+1);
	roflibs::svc::cflowcore::set_flow_core(dpt.get_dpid()).add_svc_flow(0xff000000, fm);

	/*
	 * notify core instances
	 */
	roflibs::svc::cflowcore::set_flow_core(dpt.get_dpid()).handle_dpt_open(dpt);
	roflibs::eth::cethcore::set_eth_core(dpt.get_dpid()).handle_dpt_open(dpt);
	roflibs::ip::cipcore::set_ip_core(dpt.get_dpid()).handle_dpt_open(dpt);
	roflibs::gtp::cgtpcore::set_gtp_core(dpt.get_dpid()).handle_dpt_open(dpt);
	roflibs::gtp::cgtprelay::set_gtp_relay(dpt.get_dpid()).handle_dpt_open(dpt);
	roflibs::gre::cgrecore::set_gre_core(dpt.get_dpid()).handle_dpt_open(dpt);


	//test_workflow(dpt);

	rofcore::logging::debug << "=====================================" << std::endl;
	rofcore::logging::debug << roflibs::eth::cethcore::get_eth_core(dpt.get_dpid());
	rofcore::logging::debug << roflibs::ip::cipcore::get_ip_core(dpt.get_dpid());
	rofcore::logging::debug << roflibs::gre::cgrecore::get_gre_core(dpt.get_dpid());
	rofcore::logging::debug << "=====================================" << std::endl;
}



void
cbasebox::handle_port_desc_stats_reply_timeout(rofl::crofdpt& dpt, uint32_t xid) {
}




void
cbasebox::hook_dpt_attach(const rofl::crofdpt& dpt)
{
	try {

		std::vector<std::string> argv;
		std::vector<std::string> envp;

		std::stringstream s_dpid;
		s_dpid << "DPID=" << dpt.get_dpid();
		envp.push_back(s_dpid.str());

		cbasebox::execute(cbasebox::script_path_dpt_open, argv, envp);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[cbasebox][hook_dpt_attach] script execution failed" << std::endl;
	}
}



void
cbasebox::hook_dpt_detach(const rofl::crofdpt& dpt)
{
	try {

		std::vector<std::string> argv;
		std::vector<std::string> envp;

		std::stringstream s_dpid;
		s_dpid << "DPID=" << dpt.get_dpid();
		envp.push_back(s_dpid.str());

		cbasebox::execute(cbasebox::script_path_dpt_close, argv, envp);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[cbasebox][hook_dpt_detach] script execution failed" << std::endl;
	}
}



void
cbasebox::hook_port_up(const rofl::crofdpt& dpt, std::string const& devname)
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

		cbasebox::execute(cbasebox::script_path_port_up, argv, envp);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[cbasebox][hook_port_up] script execution failed" << std::endl;
	}
}



void
cbasebox::hook_port_down(const rofl::crofdpt& dpt, std::string const& devname)
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

		cbasebox::execute(cbasebox::script_path_port_down, argv, envp);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[cbasebox][hook_port_down] script execution failed" << std::endl;
	}
}



void
cbasebox::execute(
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



void
cbasebox::addr_in4_created(unsigned int ifindex, uint16_t adindex)
{
	try {
		//(void)roflibs::ip::cipcore::get_ip_core(rofl::crofdpt::get_dpt(dptid).get_dpid()).get_link(ifindex);

	} catch (roflibs::ip::eLinkNotFound& e) {
		// ignore addresses assigned to non-datapath ports
	} catch (rofcore::crtlink::eRtLinkNotFound& e) {
		rofcore::logging::debug << "[cbasebox][addr_in4_created] link not found" << std::endl;
	} catch (rofcore::crtaddr::eRtAddrNotFound& e) {
		rofcore::logging::debug << "[cbasebox][addr_in4_created] address not found" << std::endl;
	} catch (rofl::eSysCall& e) {
			// ...
	}
}



void
cbasebox::addr_in4_deleted(unsigned int ifindex, uint16_t adindex)
{
	try {
		//(void)roflibs::ip::cipcore::get_ip_core(rofl::crofdpt::get_dpt(dptid).get_dpid()).get_link(ifindex);

	} catch (roflibs::ip::eLinkNotFound& e) {
		// ignore addresses assigned to non-datapath ports
	} catch (rofcore::crtlink::eRtLinkNotFound& e) {
		rofcore::logging::debug << "[cbasebox][addr_in4_deleted] link not found" << std::endl;
	} catch (rofcore::crtaddr::eRtAddrNotFound& e) {
		rofcore::logging::debug << "[cbasebox][addr_in4_deleted] address not found" << std::endl;
	} catch (rofl::eSysCall& e) {
			// ...
	}
}



void
cbasebox::addr_in6_created(unsigned int ifindex, uint16_t adindex)
{
	try {
		//(void)roflibs::ip::cipcore::get_ip_core(rofl::crofdpt::get_dpt(dptid).get_dpid()).get_link(ifindex);

	} catch (roflibs::ip::eLinkNotFound& e) {
		// ignore addresses assigned to non-datapath ports
	} catch (rofcore::crtlink::eRtLinkNotFound& e) {
		rofcore::logging::debug << "[cbasebox][addr_in6_created] link not found" << std::endl;
	} catch (rofcore::crtaddr::eRtAddrNotFound& e) {
		rofcore::logging::debug << "[cbasebox][addr_in6_created] address not found" << std::endl;
	} catch (rofl::eSysCall& e) {
		// ...
	}
}



void
cbasebox::addr_in6_deleted(unsigned int ifindex, uint16_t adindex)
{
	try {
		//(void)roflibs::ip::cipcore::get_ip_core(rofl::crofdpt::get_dpt(dptid).get_dpid()).get_link(ifindex);

	} catch (roflibs::ip::eLinkNotFound& e) {
		// ignore addresses assigned to non-datapath ports
	} catch (rofcore::crtlink::eRtLinkNotFound& e) {
		rofcore::logging::debug << "[cbasebox][addr_in6_deleted] link not found" << e.what() << std::endl;
	} catch (rofcore::crtaddr::eRtAddrNotFound& e) {
		rofcore::logging::debug << "[cbasebox][addr_in6_deleted] address not found" << e.what() << std::endl;
	} catch (rofl::eSysCall& e) {
			// ...
	}
}





void
cbasebox::test_workflow(rofl::crofdpt& dpt)
{
	bool gre_test = true;
	bool gtp_test = false;

	/*
	 * GRE test
	 */
	if (gre_test) {

		uint32_t term_id = 1;
		uint32_t gre_portno = 3;
		rofl::caddress_in4 laddr("10.1.1.1");
		rofl::caddress_in4 raddr("10.1.1.10");
		uint32_t gre_key = 0x11223344;

		roflibs::gre::cgrecore::set_gre_core(dpt.get_dpid()).
				add_gre_term_in4(term_id, gre_portno, laddr, raddr, gre_key);
	}

	/*
	 * GTP test
	 */
	if (gtp_test) {

		roflibs::gtp::cgtprelay::set_gtp_relay(dpt.get_dpid()).set_termdev("tun57").
				add_prefix_in4(rofcore::cprefix_in4(rofl::caddress_in4("192.168.2.1"), 24));

		roflibs::gtp::cgtprelay::set_gtp_relay(dpt.get_dpid()).set_termdev("tun57").
				add_prefix_in4(rofcore::cprefix_in4(rofl::caddress_in4("192.168.4.1"), 24));

		roflibs::gtp::cgtprelay::set_gtp_relay(dpt.get_dpid()).set_termdev("tun57").
				add_prefix_in6(rofcore::cprefix_in6(rofl::caddress_in6("3000::1"), 64));

		rofcore::logging::debug << roflibs::gtp::cgtprelay::set_gtp_relay(dpt.get_dpid()).get_termdev("tun57");
	}
	if (gtp_test) {
		roflibs::gtp::clabel_in4 label_egress(
				roflibs::gtp::caddress_gtp_in4(rofl::caddress_in4("10.1.1.10"), roflibs::gtp::cport(roflibs::gtp::cgtpcore::DEFAULT_GTPU_PORT)),
				roflibs::gtp::caddress_gtp_in4(rofl::caddress_in4("10.1.1.1") , roflibs::gtp::cport(roflibs::gtp::cgtpcore::DEFAULT_GTPU_PORT)),
				roflibs::gtp::cteid(111111));

		roflibs::gtp::clabel_in4 label_ingress(
				roflibs::gtp::caddress_gtp_in4(rofl::caddress_in4("10.1.1.1") , roflibs::gtp::cport(roflibs::gtp::cgtpcore::DEFAULT_GTPU_PORT)),
				roflibs::gtp::caddress_gtp_in4(rofl::caddress_in4("10.1.1.10"), roflibs::gtp::cport(roflibs::gtp::cgtpcore::DEFAULT_GTPU_PORT)),
				roflibs::gtp::cteid(111111));

		rofl::openflow::cofmatch tft_match(dpt.get_version());
		tft_match.set_eth_type(rofl::fipv4frame::IPV4_ETHER);
		tft_match.set_ipv4_src(rofl::caddress_in4("10.2.2.20"));
		tft_match.set_ipv4_dst(rofl::caddress_in4("192.168.4.33"));

		roflibs::gtp::cgtpcore::set_gtp_core(dpt.get_dpid()).add_term_in4(label_egress, label_ingress, tft_match);
	}
	if (gtp_test) {
		roflibs::gtp::clabel_in4 label_in(
				roflibs::gtp::caddress_gtp_in4(rofl::caddress_in4("10.1.1.10"), roflibs::gtp::cport(roflibs::gtp::cgtpcore::DEFAULT_GTPU_PORT)),
				roflibs::gtp::caddress_gtp_in4(rofl::caddress_in4("10.1.1.1") , roflibs::gtp::cport(roflibs::gtp::cgtpcore::DEFAULT_GTPU_PORT)),
				roflibs::gtp::cteid(111111));
		roflibs::gtp::clabel_in4 label_out(
				roflibs::gtp::caddress_gtp_in4(rofl::caddress_in4("10.2.2.1") , roflibs::gtp::cport(roflibs::gtp::cgtpcore::DEFAULT_GTPU_PORT)),
				roflibs::gtp::caddress_gtp_in4(rofl::caddress_in4("10.2.2.20"), roflibs::gtp::cport(roflibs::gtp::cgtpcore::DEFAULT_GTPU_PORT)),
				roflibs::gtp::cteid(222222));
		roflibs::gtp::cgtpcore::set_gtp_core(dpt.get_dpid()).add_relay_in4(label_in, label_out);
	}
	if (gtp_test) {
		roflibs::gtp::clabel_in4 label_in(
				roflibs::gtp::caddress_gtp_in4(rofl::caddress_in4("10.2.2.20"), roflibs::gtp::cport(roflibs::gtp::cgtpcore::DEFAULT_GTPU_PORT)),
				roflibs::gtp::caddress_gtp_in4(rofl::caddress_in4("10.2.2.1") , roflibs::gtp::cport(roflibs::gtp::cgtpcore::DEFAULT_GTPU_PORT)),
				roflibs::gtp::cteid(222222));
		roflibs::gtp::clabel_in4 label_out(
				roflibs::gtp::caddress_gtp_in4(rofl::caddress_in4("10.1.1.1") , roflibs::gtp::cport(roflibs::gtp::cgtpcore::DEFAULT_GTPU_PORT)),
				roflibs::gtp::caddress_gtp_in4(rofl::caddress_in4("10.1.1.10"), roflibs::gtp::cport(roflibs::gtp::cgtpcore::DEFAULT_GTPU_PORT)),
				roflibs::gtp::cteid(111111));
		roflibs::gtp::cgtpcore::set_gtp_core(dpt.get_dpid()).add_relay_in4(label_in, label_out);

		rofcore::logging::debug << roflibs::gtp::cgtpcore::get_gtp_core(dpt.get_dpid());
	}
}


