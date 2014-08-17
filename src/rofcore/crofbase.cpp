/*
 * crofbase.cpp
 *
 *  Created on: 05.08.2014
 *      Author: andreas
 */

#include "crofbase.hpp"

using namespace basebox;

/*static*/crofbase* crofbase::rofbase = (crofbase*)0;
/*static*/const std::string crofbase::ROFCORE_LOG_FILE = std::string("/var/log/ethcored.log");
/*static*/const std::string crofbase::ROFCORE_PID_FILE = std::string("/var/run/ethcored.pid");
/*static*/const std::string crofbase::ROFCORE_CONFIG_FILE = std::string("/usr/local/etc/ethcored.conf");
/*static*/const std::string crofbase::ROFCORE_CONFIG_DPT_LIST = std::string("ethcored.datapaths");


/*static*/
int
crofbase::run(int argc, char** argv)
{
	rofl::cunixenv env_parser(argc, argv);

	/* update defaults */
	//env_parser.update_default_option("logfile", ETHCORE_LOG_FILE);
	//env_parser.update_default_option("config-file", ETHCORE_CONFIG_FILE);
	env_parser.add_option(rofl::coption(true, NO_ARGUMENT,'D',"daemonize","daemonize",""));
	env_parser.add_option(rofl::coption(true, REQUIRED_ARGUMENT, 'l', "logfile", "set log-file", std::string(ROFCORE_LOG_FILE)));
	env_parser.add_option(rofl::coption(true, REQUIRED_ARGUMENT, 'c', "config-file", "set config-file", std::string(ROFCORE_CONFIG_FILE)));
	env_parser.add_option(rofl::coption(true, REQUIRED_ARGUMENT, 'i', "pidfile", "set pid-file", std::string(ROFCORE_PID_FILE)));
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
		logfile = std::string(ROFCORE_LOG_FILE); // default
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
		pidfile = std::string(ROFCORE_PID_FILE); // default
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

	if (config.exists(ROFCORE_CONFIG_DPT_LIST)) {
		for (int i = 0; i < config.lookup(ROFCORE_CONFIG_DPT_LIST).getLength(); i++) {
			try {
				libconfig::Setting& datapath = config.lookup(ROFCORE_CONFIG_DPT_LIST)[i];

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

	basebox::crofbase& rofbase = basebox::crofbase::get_instance(versionbitmap);

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
