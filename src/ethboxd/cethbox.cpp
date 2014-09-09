/*
 * crofbase.cpp
 *
 *  Created on: 05.08.2014
 *      Author: andreas
 */

#include "cethbox.hpp"

using namespace ethbox;

/*static*/cethbox* cethbox::sethbox = (cethbox*)0;
/*static*/const std::string cethbox::BASEBOX_LOG_FILE = std::string("/var/log/ethcored.log");
/*static*/const std::string cethbox::BASEBOX_PID_FILE = std::string("/var/run/ethcored.pid");
/*static*/const std::string cethbox::BASEBOX_CONFIG_FILE = std::string("/usr/local/etc/ethcored.conf");
/*static*/const std::string cethbox::BASEBOX_CONFIG_DPT_LIST = std::string("ethcored.datapaths");

/*static*/std::string cethbox::script_path_dpt_open 	= std::string("/var/lib/ethcore/dpath-open.sh");
/*static*/std::string cethbox::script_path_dpt_close 	= std::string("/var/lib/ethcore/dpath-close.sh");
/*static*/std::string cethbox::script_path_port_up 	= std::string("/var/lib/ethcore/port-up.sh");
/*static*/std::string cethbox::script_path_port_down 	= std::string("/var/lib/ethcore/port-down.sh");

/*static*/
int
cethbox::run(int argc, char** argv)
{
	rofl::cunixenv env_parser(argc, argv);

	/* update defaults */
	//env_parser.update_default_option("logfile", ETHCORE_LOG_FILE);
	//env_parser.update_default_option("config-file", ETHCORE_CONFIG_FILE);
	env_parser.add_option(rofl::coption(true, NO_ARGUMENT,'D',"daemonize","daemonize",""));
	env_parser.add_option(rofl::coption(true, REQUIRED_ARGUMENT, 'l', "logfile", "set log-file", std::string(BASEBOX_LOG_FILE)));
	env_parser.add_option(rofl::coption(true, REQUIRED_ARGUMENT, 'c', "config-file", "set config-file", std::string(BASEBOX_CONFIG_FILE)));
	env_parser.add_option(rofl::coption(true, REQUIRED_ARGUMENT, 'i', "pidfile", "set pid-file", std::string(BASEBOX_PID_FILE)));
	env_parser.add_option(rofl::coption(true, REQUIRED_ARGUMENT, 'p', "port", "set port", ""+6653));

	// command line arguments
	env_parser.parse_args();

	// configuration file
	ethcore::cconfig::get_instance().open(env_parser.get_arg("config-file"));


	/*
	 * extract debug level
	 */
	int rofl_debug = 0;
	if (ethcore::cconfig::get_instance().exists("ethcored.daemon.logging.rofl.debug")) {
		rofl_debug = (int)ethcore::cconfig::get_instance().lookup("ethcored.daemon.logging.rofl.debug");
	}
	int core_debug = 0;
	if (ethcore::cconfig::get_instance().exists("ethcored.daemon.logging.core.debug")) {
		core_debug = (int)ethcore::cconfig::get_instance().lookup("ethcored.daemon.logging.core.debug");
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
	} else if (ethcore::cconfig::get_instance().exists("ethcored.daemon.logfile")) {
		logfile = (const char*)ethcore::cconfig::get_instance().lookup("ethcored.daemon.logfile");
	} else {
		logfile = std::string(BASEBOX_LOG_FILE); // default
	}

	/*
	 * extract pid-file
	 */
	std::string pidfile;
	if (env_parser.is_arg_set("pidfile")) {
		pidfile = env_parser.get_arg("pidfile");
	} else if (ethcore::cconfig::get_instance().exists("ethcored.daemon.pidfile")) {
		pidfile = (const char*)ethcore::cconfig::get_instance().lookup("ethcored.daemon.pidfile");
	} else {
		pidfile = std::string(BASEBOX_PID_FILE); // default
	}

	/*
	 * extract daemonize flag
	 */
	bool daemonize = true;
	if (env_parser.is_arg_set("daemonize")) {
		daemonize = atoi(env_parser.get_arg("pidfile").c_str());
	} else if (ethcore::cconfig::get_instance().exists("ethcored.daemon.daemonize")) {
		daemonize = (bool)ethcore::cconfig::get_instance().lookup("ethcored.daemon.daemonize");
	} else {
		daemonize = true; // default
	}



	/*
	 * daemonize
	 */
	if (daemonize) {
		rofl::cdaemon::daemonize(env_parser.get_arg("pidfile"), env_parser.get_arg("logfile"));
	}
	rofl::logging::set_debug_level(rofl_debug);
	rofcore::logging::set_debug_level(core_debug);

	if (daemonize) {
		rofcore::logging::notice << "[ethcored][main] daemonizing successful" << std::endl;
	}




	/*
	 * read configuration (for now: from libconfig++ file)
	 */

	ethcore::cconfig& config = ethcore::cconfig::get_instance();

#if 1
	if (config.exists(BASEBOX_CONFIG_DPT_LIST)) {
		for (int i = 0; i < config.lookup(BASEBOX_CONFIG_DPT_LIST).getLength(); i++) {
			try {
				libconfig::Setting& datapath = config.lookup(BASEBOX_CONFIG_DPT_LIST)[i];

				// get data path dpid
				if (not datapath.exists("dpid")) {
					continue; // as we do not know the data path dpid
				}
				rofl::cdpid dpid( (int)datapath["dpid"] );

				// get default port vid
				uint16_t default_pvid = 1;
				if (datapath.exists("default_pvid")) {
					default_pvid = (int)datapath["default_pvid"];
				}

				// this is the cethbox instance for this data path
				rofeth::cethcore& ethcore = rofeth::cethcore::set_eth_core(dpid, default_pvid, 0, 1, 5);

				// create vlan instance for default_pvid, just in case, there are no member ports defined
				ethcore.set_vlan(default_pvid);

				// extract all ports
				if (datapath.exists("ports")) {
					for (int j = 0; j < datapath["ports"].getLength(); ++j) {
						libconfig::Setting& port = datapath["ports"][j];

						// portno
						uint32_t portno(0);
						if (not port.exists("portno")) {
							continue; // no portno, skip this port
						} else {
							portno = (uint32_t)port["portno"];
						}

						// pvid
						int pvid(default_pvid);
						if (port.exists("pvid")) {
							pvid = (int)port["pvid"];
						}

						// add port "portno" in untagged mode to vlan pvid
						ethcore.set_vlan(pvid).add_port(portno, false);
					}
				}

				// extract all vlans (= tagged port memberships)
				if (datapath.exists("vlans")) {
					for (int j = 0; j < datapath["vlans"].getLength(); ++j) {
						libconfig::Setting& vlan = datapath["vlans"][j];

						// vid
						uint16_t vid(0);
						if (not vlan.exists("vid")) {
							continue; // no vid, skip this vlan
						} else {
							vid = (int)vlan["vid"];
						}

						// create vlan instance, just in case, there are no member ports defined
						ethcore.set_vlan(vid);

						// tagged port memberships
						if (vlan.exists("tagged")) {
							for (int k = 0; k < vlan["tagged"].getLength(); ++k) {
								libconfig::Setting& port = vlan["tagged"][k];

								// portno
								uint32_t portno = (uint32_t)port;

								// check whether port already exists in vlan
								if (ethcore.has_vlan(vid) && ethcore.get_vlan(vid).has_port(portno)) {
									continue;
								}

								// add port "portno" in tagged mode to vlan vid
								ethcore.set_vlan(vid).add_port(portno, true);
							}
						}
					}
				}

				rofcore::logging::debug << "after config:" << std::endl << rofeth::cethcore::get_eth_core(dpid);


			} catch (libconfig::SettingNotFoundException& e) {

			}
		}
	}
#endif

	/*
	 * prepare OpenFlow socket for listening
	 */
	rofl::openflow::cofhello_elem_versionbitmap versionbitmap;
	if (ethcore::cconfig::get_instance().exists("ethcored.openflow.version")) {
		int ofp_version = (int)ethcore::cconfig::get_instance().lookup("ethcored.openflow.version");
		ofp_version = (ofp_version < rofl::openflow13::OFP_VERSION) ? rofl::openflow13::OFP_VERSION : ofp_version;
		versionbitmap.add_ofp_version(ofp_version);
	} else {
		versionbitmap.add_ofp_version(rofl::openflow13::OFP_VERSION);
	}

	rofcore::logging::notice << "[ethcored][main] using OpenFlow version-bitmap:" << std::endl << versionbitmap;

	ethbox::cethbox& rofbase = ethbox::cethbox::get_instance(versionbitmap);

	//base.init(/*port-table-id=*/0, /*fib-in-table-id=*/1, /*fib-out-table-id=*/2, /*default-vid=*/1);

	std::stringstream portno;
	if (ethcore::cconfig::get_instance().exists("ethcored.openflow.bindport")) {
		portno << (int)ethcore::cconfig::get_instance().lookup("ethcored.openflow.bindport");
	} else {
		portno << (int)6653;
	}

	std::stringstream bindaddr;
	if (ethcore::cconfig::get_instance().exists("ethcored.openflow.bindaddr")) {
		bindaddr << (const char*)ethcore::cconfig::get_instance().lookup("ethcored.openflow.bindaddr");
	} else {
		bindaddr << "::";
	}

	enum rofl::csocket::socket_type_t socket_type = rofl::csocket::SOCKET_TYPE_PLAIN;
	rofl::cparams socket_params = rofl::csocket::get_default_params(socket_type);
	socket_params.set_param(rofl::csocket::PARAM_KEY_LOCAL_PORT).set_string(portno.str());
	socket_params.set_param(rofl::csocket::PARAM_KEY_LOCAL_HOSTNAME).set_string(bindaddr.str());

	rofbase.rpc_listen_for_dpts(socket_type, socket_params);


	/*
	 * start main loop, does not return
	 */
	rofl::cioloop::run();

	rofl::cioloop::shutdown();

	return 0;
}






void
cethbox::enqueue(rofcore::cnetdev *netdev, rofl::cpacket* pkt)
{
	try {
		if (not rofl::crofdpt::get_dpt(dptid).get_channel().is_established()) {
			throw eLinkNoDptAttached("cethbox::enqueue() dpt not found");
		}

		rofcore::ctapdev* tapdev = dynamic_cast<rofcore::ctapdev*>( netdev );
		if (0 == tapdev) {
			throw eLinkTapDevNotFound("cethbox::enqueue() tap device not found");
		}

		rofl::openflow::cofactions actions(rofl::crofdpt::get_dpt(dptid).get_version());
		actions.set_action_output(rofl::cindex(0)).set_port_no(tapdev->get_ofp_port_no());

		rofl::crofdpt::get_dpt(dptid).send_packet_out_message(
				rofl::cauxid(0),
				rofl::openflow::base::get_ofp_no_buffer(rofl::crofdpt::get_dpt(dptid).get_version()),
				rofl::openflow::base::get_ofpp_controller_port(rofl::crofdpt::get_dpt(dptid).get_version()),
				actions,
				pkt->soframe(),
				pkt->length());

	} catch (eLinkNoDptAttached& e) {
		rofcore::logging::warn << "[cethbox][enqueue] no data path attached, dropping outgoing packet" << std::endl;

	} catch (eLinkTapDevNotFound& e) {
		rofcore::logging::warn << "[cethbox][enqueue] unable to find tap device" << std::endl;
	}

	rofcore::cpacketpool::get_instance().release_pkt(pkt);
}



void
cethbox::enqueue(rofcore::cnetdev *netdev, std::vector<rofl::cpacket*> pkts)
{
	for (std::vector<rofl::cpacket*>::iterator
			it = pkts.begin(); it != pkts.end(); ++it) {
		enqueue(netdev, *it);
	}
}




void
cethbox::handle_dpt_open(
		rofl::crofdpt& dpt) {
	dptid = dpt.get_dptid();

	rofeth::cethcore::set_eth_core(dpt.get_dpid(), /*default_vid=*/1, /*port-membership=*/0, /*src-table=*/1, /*dst-table=*/6);
	dpt.flow_mod_reset();
	dpt.group_mod_reset();
	dpt.send_port_desc_stats_request(rofl::cauxid(0), 0);

	// call external scripting hook
	hook_dpt_attach();
}



void
cethbox::handle_dpt_close(
		rofl::crofdpt& dpt) {
	// call external scripting hook
	hook_dpt_detach();

	rofeth::cethcore::set_eth_core(dpt.get_dpid()).handle_dpt_close(dpt);
}



void
cethbox::handle_packet_in(
		rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_packet_in& msg) {

	try {
		uint32_t ofp_port_no = msg.get_match().get_in_port();

		if (not has_tap_dev(ofp_port_no)) {
			// TODO: handle error?
			return;
		}

		rofl::cpacket *pkt = rofcore::cpacketpool::get_instance().acquire_pkt();
		*pkt = msg.get_packet();
		rofcore::logging::debug << "[cethbox][handle_packet_in] pkt received [1]: " << std::endl << msg.get_packet();
		rofcore::logging::debug << "[cethbox][handle_packet_in] pkt received [2]: " << std::endl << *pkt;
		set_tap_dev(ofp_port_no).enqueue(pkt);

		switch (msg.get_table_id()) {
		default: {
			if (rofeth::cethcore::has_eth_core(dpt.get_dpid())) {
				rofeth::cethcore::set_eth_core(dpt.get_dpid()).handle_packet_in(dpt, auxid, msg);
			}
		};
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
cethbox::handle_flow_removed(
		rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_flow_removed& msg) {
	switch (msg.get_table_id()) {
	default: {
		if (rofeth::cethcore::has_eth_core(dpt.get_dpid())) {
			rofeth::cethcore::set_eth_core(dpt.get_dpid()).handle_flow_removed(dpt, auxid, msg);
		}
	};
	}
}



void
cethbox::handle_port_status(
		rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_port_status& msg) {
	try {
		const rofl::openflow::cofport& port = msg.get_port();
		uint32_t ofp_port_no = msg.get_port().get_port_no();

		switch (msg.get_reason()) {
		case rofl::openflow::OFPPR_ADD: {
			if (not has_tap_dev(ofp_port_no)) {
				add_tap_dev(port.get_port_no(), port.get_name(), port.get_hwaddr());
				hook_port_up(msg.get_port().get_name());
			}

			if (port.link_state_is_link_down() || port.config_is_port_down()) {
				set_tap_dev(ofp_port_no).disable_interface();
			} else {
				set_tap_dev(ofp_port_no).enable_interface();
			}

		} break;
		case rofl::openflow::OFPPR_MODIFY: {
			if (not has_tap_dev(ofp_port_no)) {
				add_tap_dev(port.get_port_no(), port.get_name(), port.get_hwaddr());
			}

			if (port.link_state_is_link_down() || port.config_is_port_down()) {
				set_tap_dev(ofp_port_no).disable_interface();
			} else {
				set_tap_dev(ofp_port_no).enable_interface();
			}

		} break;
		case rofl::openflow::OFPPR_DELETE: {
			hook_port_down(msg.get_port().get_name());
			drop_tap_dev(port.get_port_no());

		} break;
		default: {
			// invalid/unknown reason
		} return;
		}

		if (rofeth::cethcore::has_eth_core(dpt.get_dpid())) {
			rofeth::cethcore::set_eth_core(dpt.get_dpid()).handle_port_status(dpt, auxid, msg);
		}

	} catch (rofl::openflow::ePortNotFound& e) {
		// TODO: log error
	} catch (rofcore::eNetDevCritical& e) {
		rofcore::logging::debug << "[cethbox] new port created: unable to create tap device: " << msg.get_port().get_name() << std::endl;
	}
}



void
cethbox::handle_error_message(
		rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_error& msg) {
	if (rofeth::cethcore::has_eth_core(dpt.get_dpid())) {
		rofeth::cethcore::set_eth_core(dpt.get_dpid()).handle_error_message(dpt, auxid, msg);
	}
}



void
cethbox::handle_port_desc_stats_reply(
		rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_port_desc_stats_reply& msg) {

	dpt.set_ports() = msg.get_ports();

	for (std::map<uint32_t, rofl::openflow::cofport*>::const_iterator
			it = dpt.get_ports().get_ports().begin(); it != dpt.get_ports().get_ports().end(); ++it) {
		const rofl::openflow::cofport& port = *(it->second);

		rofeth::cethcore::set_eth_core(dpt.get_dpid()).set_vlan(/*default_vid=*/1).add_port(port.get_port_no(), /*tagged=*/false);
	}

	for (std::map<uint32_t, rofl::openflow::cofport*>::const_iterator
			it = dpt.get_ports().get_ports().begin(); it != dpt.get_ports().get_ports().end(); ++it) {
		const rofl::openflow::cofport& port = *(it->second);

		if (not has_tap_dev(port.get_port_no())) {
			add_tap_dev(port.get_port_no(), port.get_name(), port.get_hwaddr());
		}
	}

	rofeth::cethcore::set_eth_core(dpt.get_dpid()).handle_dpt_open(dpt);
}



void
cethbox::handle_port_desc_stats_reply_timeout(rofl::crofdpt& dpt, uint32_t xid) {
}




void
cethbox::hook_dpt_attach()
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);

		std::vector<std::string> argv;
		std::vector<std::string> envp;

		std::stringstream s_dpid;
		s_dpid << "DPID=" << dpt.get_dpid();
		envp.push_back(s_dpid.str());

		cethbox::execute(cethbox::script_path_dpt_open, argv, envp);

	} catch (rofl::eRofDptNotFound& e) {

	}
}



void
cethbox::hook_dpt_detach()
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);

		std::vector<std::string> argv;
		std::vector<std::string> envp;

		std::stringstream s_dpid;
		s_dpid << "DPID=" << dpt.get_dpid();
		envp.push_back(s_dpid.str());

		cethbox::execute(cethbox::script_path_dpt_close, argv, envp);

	} catch (rofl::eRofDptNotFound& e) {

	}
}



void
cethbox::hook_port_up(std::string const& devname)
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

		cethbox::execute(cethbox::script_path_port_up, argv, envp);

	} catch (rofl::eRofDptNotFound& e) {

	}
}



void
cethbox::hook_port_down(std::string const& devname)
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

		cethbox::execute(cethbox::script_path_port_down, argv, envp);

	} catch (rofl::eRofDptNotFound& e) {

	}
}



void
cethbox::execute(
		std::string const& executable,
		std::vector<std::string> argv,
		std::vector<std::string> envp)
{
	pid_t pid = 0;

	if ((pid = fork()) < 0) {
		rofcore::logging::error << "[cethbox][execute] syscall error fork(): " << errno << ":" << strerror(errno) << std::endl;
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



