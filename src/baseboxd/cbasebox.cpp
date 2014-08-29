/*
 * crofbase.cpp
 *
 *  Created on: 05.08.2014
 *      Author: andreas
 */

#include "cbasebox.hpp"

using namespace basebox;

/*static*/cbasebox* cbasebox::rofbase = (cbasebox*)0;
/*static*/const std::string cbasebox::BASEBOX_LOG_FILE = std::string("/var/log/baseboxd.log");
/*static*/const std::string cbasebox::BASEBOX_PID_FILE = std::string("/var/run/baseboxd.pid");
/*static*/const std::string cbasebox::BASEBOX_CONFIG_FILE = std::string("/usr/local/etc/baseboxd.conf");
/*static*/const std::string cbasebox::BASEBOX_CONFIG_DPT_LIST = std::string("baseboxd.datapaths");

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
	env_parser.add_option(rofl::coption(true, REQUIRED_ARGUMENT, 'p', "port", "set port", ""+6653));

	// command line arguments
	env_parser.parse_args();

	// configuration file
	ethcore::cconfig::get_instance().open(env_parser.get_arg("config-file"));


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
		rofl::cdaemon::daemonize(env_parser.get_arg("pidfile"), env_parser.get_arg("logfile"));
	}
	rofl::logging::set_debug_level(rofl_debug);
	rofcore::logging::set_debug_level(core_debug);

	if (daemonize) {
		rofcore::logging::notice << "[baseboxd][main] daemonizing successful" << std::endl;
	}




	/*
	 * read configuration (for now: from libconfig++ file)
	 */

	ethcore::cconfig& config = ethcore::cconfig::get_instance();

#if 0
	if (config.exists(BASEBOX_CONFIG_DPT_LIST)) {
		for (int i = 0; i < config.lookup(BASEBOX_CONFIG_DPT_LIST).getLength(); i++) {
			try {
				libconfig::Setting& datapath = config.lookup(BASEBOX_CONFIG_DPT_LIST)[i];

				// get data path dpid
				if (not datapath.exists("dpid")) {
					continue; // as we do not know the data path dpid
				}
				ethcore::cdpid dpid( (int)datapath["dpid"] );

				// get default port vid
				uint16_t default_pvid = 1;
				if (datapath.exists("default_pvid")) {
					default_pvid = (int)datapath["default_pvid"];
				}

				// this is the cethcore instance for this data path
				ethcore::cethcore& ethcore = ethcore::cethcore::set_core(dpid, default_pvid, 0, 1, 5);

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

				rofcore::logging::debug << "after config:" << std::endl << ethcore::cethcore::get_core(dpid);


			} catch (libconfig::SettingNotFoundException& e) {

			}
		}
	}
#endif

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

	basebox::cbasebox& rofbase = basebox::cbasebox::get_instance(versionbitmap);

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

	rofbase.rpc_listen_for_dpts(socket_type, socket_params);


	/*
	 * start main loop, does not return
	 */
	rofl::cioloop::run();

	rofl::cioloop::shutdown();

	return 0;
}






void
cbasebox::enqueue(rofcore::cnetdev *netdev, rofl::cpacket* pkt)
{
	try {
		if (not rofl::crofdpt::get_dpt(dptid).get_channel().is_established()) {
			throw eLinkNoDptAttached("cbasebox::enqueue() dpt not found");
		}

		rofcore::ctapdev* tapdev = dynamic_cast<rofcore::ctapdev*>( netdev );
		if (0 == tapdev) {
			throw eLinkTapDevNotFound("cbasebox::enqueue() tap device not found");
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
		rofcore::logging::warn << "[cbasebox][enqueue] no data path attached, dropping outgoing packet" << std::endl;

	} catch (eLinkTapDevNotFound& e) {
		rofcore::logging::warn << "[cbasebox][enqueue] unable to find tap device" << std::endl;
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
	dptid = dpt.get_dptid();

	rofip::cipcore::set_ip_core(dpt.get_dpid(), /*local-stage=*/3, /*out-stage=*/5);
	rofeth::cethcore::set_eth_core(dpt.get_dpid(), /*default_vid=*/1, /*port-membership=*/0, /*src-table=*/1, /*dst-table=*/6);
	rofgtp::cgtpcore::set_gtp_core(dpt.get_dpid(), /*ip-local-stage=*/3, /*gtp-stage=*/4); // yes, same as local for cipcore
	rofgtp::cgtprelay::set_gtp_relay(dpt.get_dpid(), /*ip-local-stage=*/3);
	dpt.flow_mod_reset();
	dpt.group_mod_reset();
	dpt.send_port_desc_stats_request(rofl::cauxid(0), 0);

	// call external scripting hook
	hook_dpt_attach();
}



void
cbasebox::handle_dpt_close(
		rofl::crofdpt& dpt) {
	// call external scripting hook
	hook_dpt_detach();

	rofgtp::cgtprelay::set_gtp_relay(dpt.get_dpid()).handle_dpt_close(dpt);
	rofgtp::cgtpcore::set_gtp_core(dpt.get_dpid()).handle_dpt_close(dpt);
	rofeth::cethcore::set_eth_core(dpt.get_dpid()).handle_dpt_close(dpt);
	rofip::cipcore::set_ip_core(dpt.get_dpid()).handle_dpt_close(dpt);
}



void
cbasebox::handle_packet_in(
		rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_packet_in& msg) {

	try {
		uint32_t ofp_port_no = msg.get_match().get_in_port();

		if (not has_tap_dev(ofp_port_no)) {
			// TODO: handle error?
			return;
		}

		rofl::cpacket *pkt = rofcore::cpacketpool::get_instance().acquire_pkt();
		*pkt = msg.get_packet();
		rofcore::logging::debug << "[cbasebox][handle_packet_in] pkt received [1]: " << std::endl << msg.get_packet();
		rofcore::logging::debug << "[cbasebox][handle_packet_in] pkt received [2]: " << std::endl << *pkt;
		set_tap_dev(ofp_port_no).enqueue(pkt);

		switch (msg.get_table_id()) {
		case 3:
		case 4: {
			rofip::cipcore::set_ip_core(dpt.get_dpid()).handle_packet_in(dpt, auxid, msg);
		} break;
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
cbasebox::handle_flow_removed(
		rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_flow_removed& msg) {
	switch (msg.get_table_id()) {
	case 3:
	case 4: {
		rofip::cipcore::set_ip_core(dpt.get_dpid()).handle_flow_removed(dpt, auxid, msg);
	} break;
	default: {
		if (rofeth::cethcore::has_eth_core(dpt.get_dpid())) {
			rofeth::cethcore::set_eth_core(dpt.get_dpid()).handle_flow_removed(dpt, auxid, msg);
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

		rofip::cipcore::set_ip_core(dpt.get_dpid()).handle_port_status(dpt, auxid, msg);
		if (rofeth::cethcore::has_eth_core(dpt.get_dpid())) {
			rofeth::cethcore::set_eth_core(dpt.get_dpid()).handle_port_status(dpt, auxid, msg);
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
	rofip::cipcore::set_ip_core(dpt.get_dpid()).handle_error_message(dpt, auxid, msg);
	if (rofeth::cethcore::has_eth_core(dpt.get_dpid())) {
		rofeth::cethcore::set_eth_core(dpt.get_dpid()).handle_error_message(dpt, auxid, msg);
	}
}



void
cbasebox::handle_port_desc_stats_reply(
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
	rofip::cipcore::set_ip_core(dpt.get_dpid()).handle_dpt_open(dpt);
	rofgtp::cgtpcore::set_gtp_core(dpt.get_dpid()).handle_dpt_open(dpt);
	rofgtp::cgtprelay::set_gtp_relay(dpt.get_dpid()).handle_dpt_open(dpt);

	test_workflow();
}



void
cbasebox::handle_port_desc_stats_reply_timeout(rofl::crofdpt& dpt, uint32_t xid) {
}




void
cbasebox::hook_dpt_attach()
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);

		std::vector<std::string> argv;
		std::vector<std::string> envp;

		std::stringstream s_dpid;
		s_dpid << "DPID=" << dpt.get_dpid();
		envp.push_back(s_dpid.str());

		cbasebox::execute(cbasebox::script_path_dpt_open, argv, envp);

	} catch (rofl::eRofDptNotFound& e) {

	}
}



void
cbasebox::hook_dpt_detach()
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);

		std::vector<std::string> argv;
		std::vector<std::string> envp;

		std::stringstream s_dpid;
		s_dpid << "DPID=" << dpt.get_dpid();
		envp.push_back(s_dpid.str());

		cbasebox::execute(cbasebox::script_path_dpt_close, argv, envp);

	} catch (rofl::eRofDptNotFound& e) {

	}
}



void
cbasebox::hook_port_up(std::string const& devname)
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

		cbasebox::execute(cbasebox::script_path_port_up, argv, envp);

	} catch (rofl::eRofDptNotFound& e) {

	}
}



void
cbasebox::hook_port_down(std::string const& devname)
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

		cbasebox::execute(cbasebox::script_path_port_down, argv, envp);

	} catch (rofl::eRofDptNotFound& e) {

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
		(void)rofip::cipcore::get_ip_core(rofl::crofdpt::get_dpt(dptid).get_dpid()).get_link(ifindex);

		const rofcore::crtaddr_in4& addr = rofcore::cnetlink::get_instance().
				get_links().get_link(ifindex).get_addrs_in4().get_addr(adindex);

		rofgtp::cgtprelay::set_gtp_relay(rofl::crofdpt::get_dpt(dptid).get_dpid()).
				add_socket_in4(rofgtp::caddress_gtp_in4(addr.get_local_addr(),
						rofgtp::cport(rofgtp::cgtpcore::DEFAULT_GTPU_PORT)));

	} catch (rofip::eLinkNotFound& e) {
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
		(void)rofip::cipcore::get_ip_core(rofl::crofdpt::get_dpt(dptid).get_dpid()).get_link(ifindex);

		const rofcore::crtaddr_in4& addr = rofcore::cnetlink::get_instance().
				get_links().get_link(ifindex).get_addrs_in4().get_addr(adindex);

		rofgtp::cgtprelay::set_gtp_relay(rofl::crofdpt::get_dpt(dptid).get_dpid()).
				drop_socket_in4(rofgtp::caddress_gtp_in4(addr.get_local_addr(),
						rofgtp::cport(rofgtp::cgtpcore::DEFAULT_GTPU_PORT)));

	} catch (rofip::eLinkNotFound& e) {
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
		(void)rofip::cipcore::get_ip_core(rofl::crofdpt::get_dpt(dptid).get_dpid()).get_link(ifindex);

		const rofcore::crtaddr_in6& addr = rofcore::cnetlink::get_instance().
				get_links().get_link(ifindex).get_addrs_in6().get_addr(adindex);

		rofgtp::cgtprelay::set_gtp_relay(rofl::crofdpt::get_dpt(dptid).get_dpid()).
				add_socket_in6(rofgtp::caddress_gtp_in6(addr.get_local_addr(),
						rofgtp::cport(rofgtp::cgtpcore::DEFAULT_GTPU_PORT)));

	} catch (rofip::eLinkNotFound& e) {
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
		(void)rofip::cipcore::get_ip_core(rofl::crofdpt::get_dpt(dptid).get_dpid()).get_link(ifindex);

		const rofcore::crtaddr_in6& addr = rofcore::cnetlink::get_instance().
				get_links().get_link(ifindex).get_addrs_in6().get_addr(adindex);

		rofgtp::cgtprelay::set_gtp_relay(rofl::crofdpt::get_dpt(dptid).get_dpid()).
				drop_socket_in6(rofgtp::caddress_gtp_in6(addr.get_local_addr(),
						rofgtp::cport(rofgtp::cgtpcore::DEFAULT_GTPU_PORT)));

	} catch (rofip::eLinkNotFound& e) {
		// ignore addresses assigned to non-datapath ports
	} catch (rofcore::crtlink::eRtLinkNotFound& e) {
		rofcore::logging::debug << "[cbasebox][addr_in6_deleted] link not found" << std::endl;
	} catch (rofcore::crtaddr::eRtAddrNotFound& e) {
		rofcore::logging::debug << "[cbasebox][addr_in6_deleted] address not found" << std::endl;
	} catch (rofl::eSysCall& e) {
			// ...
	}
}





void
cbasebox::test_workflow()
{
	rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);

	/*
	 * test
	 */
	if (false) {

		rofgtp::cgtprelay::set_gtp_relay(dpt.get_dpid()).set_termdev("tun57").
				add_prefix_in4(rofcore::cprefix_in4(rofl::caddress_in4("192.168.2.1"), 24));

		rofgtp::cgtprelay::set_gtp_relay(dpt.get_dpid()).set_termdev("tun57").
				add_prefix_in4(rofcore::cprefix_in4(rofl::caddress_in4("192.168.4.1"), 24));

		rofgtp::cgtprelay::set_gtp_relay(dpt.get_dpid()).set_termdev("tun57").
				add_prefix_in6(rofcore::cprefix_in6(rofl::caddress_in6("3000::1"), 64));

		rofcore::logging::debug << rofgtp::cgtprelay::set_gtp_relay(dpt.get_dpid()).get_termdev("tun57");
	}
	if (false) {
		rofgtp::clabel_in4 label_egress(
				rofgtp::caddress_gtp_in4(rofl::caddress_in4("10.1.1.10"), rofgtp::cport(rofgtp::cgtpcore::DEFAULT_GTPU_PORT)),
				rofgtp::caddress_gtp_in4(rofl::caddress_in4("10.1.1.1") , rofgtp::cport(rofgtp::cgtpcore::DEFAULT_GTPU_PORT)),
				rofgtp::cteid(111111));

		rofgtp::clabel_in4 label_ingress(
				rofgtp::caddress_gtp_in4(rofl::caddress_in4("10.1.1.1") , rofgtp::cport(rofgtp::cgtpcore::DEFAULT_GTPU_PORT)),
				rofgtp::caddress_gtp_in4(rofl::caddress_in4("10.1.1.10"), rofgtp::cport(rofgtp::cgtpcore::DEFAULT_GTPU_PORT)),
				rofgtp::cteid(111111));

		rofl::openflow::cofmatch tft_match(dpt.get_version());
		tft_match.set_eth_type(rofl::fipv4frame::IPV4_ETHER);
		tft_match.set_ipv4_src(rofl::caddress_in4("10.2.2.20"));
		tft_match.set_ipv4_dst(rofl::caddress_in4("192.168.4.33"));

		rofgtp::cgtpcore::set_gtp_core(dpt.get_dpid()).add_term_in4(label_egress, label_ingress, tft_match);
	}
	if (true) {
		rofgtp::clabel_in4 label_in(
				rofgtp::caddress_gtp_in4(rofl::caddress_in4("10.1.1.10"), rofgtp::cport(rofgtp::cgtpcore::DEFAULT_GTPU_PORT)),
				rofgtp::caddress_gtp_in4(rofl::caddress_in4("10.1.1.1") , rofgtp::cport(rofgtp::cgtpcore::DEFAULT_GTPU_PORT)),
				rofgtp::cteid(111111));
		rofgtp::clabel_in4 label_out(
				rofgtp::caddress_gtp_in4(rofl::caddress_in4("10.2.2.1") , rofgtp::cport(rofgtp::cgtpcore::DEFAULT_GTPU_PORT)),
				rofgtp::caddress_gtp_in4(rofl::caddress_in4("10.2.2.20"), rofgtp::cport(rofgtp::cgtpcore::DEFAULT_GTPU_PORT)),
				rofgtp::cteid(222222));
		rofgtp::cgtpcore::set_gtp_core(dpt.get_dpid()).add_relay_in4(label_in, label_out);
	}
	if (true) {
		rofgtp::clabel_in4 label_in(
				rofgtp::caddress_gtp_in4(rofl::caddress_in4("10.2.2.20"), rofgtp::cport(rofgtp::cgtpcore::DEFAULT_GTPU_PORT)),
				rofgtp::caddress_gtp_in4(rofl::caddress_in4("10.2.2.1") , rofgtp::cport(rofgtp::cgtpcore::DEFAULT_GTPU_PORT)),
				rofgtp::cteid(222222));
		rofgtp::clabel_in4 label_out(
				rofgtp::caddress_gtp_in4(rofl::caddress_in4("10.1.1.1") , rofgtp::cport(rofgtp::cgtpcore::DEFAULT_GTPU_PORT)),
				rofgtp::caddress_gtp_in4(rofl::caddress_in4("10.1.1.10"), rofgtp::cport(rofgtp::cgtpcore::DEFAULT_GTPU_PORT)),
				rofgtp::cteid(111111));
		rofgtp::cgtpcore::set_gtp_core(dpt.get_dpid()).add_relay_in4(label_in, label_out);

		rofcore::logging::debug << rofgtp::cgtpcore::get_gtp_core(dpt.get_dpid());
	}
}

