/*
 * cethbase.cpp
 *
 *  Created on: 05.08.2014
 *      Author: andreas
 */

#include "cethbase.hpp"

using namespace ethcore;

/*static*/cethbase* cethbase::ethbase = (cethbase*)0;
/*static*/const std::string cethbase::ETHCORE_LOG_FILE = std::string("/var/log/ethcored.log");
/*static*/const std::string cethbase::ETHCORE_PID_FILE = std::string("/var/run/ethcored.pid");
/*static*/const std::string cethbase::ETHCORE_CONFIG_FILE = std::string("/usr/local/etc/ethcored.conf");
/*static*/const std::string cethbase::ETHCORE_CONFIG_DPT_LIST = std::string("ethcored.datapaths");


/*static*/
int
cethbase::run(int argc, char** argv)
{
	rofl::cunixenv env_parser(argc, argv);

	/* update defaults */
	//env_parser.update_default_option("logfile", ETHCORE_LOG_FILE);
	//env_parser.update_default_option("config-file", ETHCORE_CONFIG_FILE);
	env_parser.add_option(rofl::coption(true, REQUIRED_ARGUMENT, 'l', "logfile", "set log-file", std::string(ETHCORE_LOG_FILE)));
	env_parser.add_option(rofl::coption(true, REQUIRED_ARGUMENT, 'c', "config-file", "set config-file", std::string(ETHCORE_CONFIG_FILE)));
	env_parser.add_option(rofl::coption(true, REQUIRED_ARGUMENT, 'i', "pidfile", "set pid-file", std::string(ETHCORE_PID_FILE)));
	env_parser.add_option(rofl::coption(true, REQUIRED_ARGUMENT, 'p', "port", "set port", ""+6653));

	// command line arguments
	env_parser.parse_args();

	// configuration file
	cconfig::get_instance().open(env_parser.get_arg("config-file"));


	/*
	 * extract debug level
	 */
	int debug;
	if (env_parser.is_arg_set("debug")) {
		debug = atoi(env_parser.get_arg("debug").c_str());
	} else if (cconfig::get_instance().exists("ethcored.daemon.debug")) {
		debug = (int)cconfig::get_instance().lookup("ethcored.daemon.debug");
	} else {
		debug = 0; // default
	}

	/*
	 * extract log-file
	 */
	std::string logfile;
	if (env_parser.is_arg_set("logfile")) {
		logfile = env_parser.get_arg("logfile");
	} else if (cconfig::get_instance().exists("ethcored.daemon.logfile")) {
		logfile = (const char*)cconfig::get_instance().lookup("ethcored.daemon.logfile");
	} else {
		logfile = std::string(ETHCORE_LOG_FILE); // default
	}

	/*
	 * extract pid-file
	 */
	std::string pidfile;
	if (env_parser.is_arg_set("pidfile")) {
		pidfile = env_parser.get_arg("pidfile");
	} else if (cconfig::get_instance().exists("ethcored.daemon.pidfile")) {
		pidfile = (const char*)cconfig::get_instance().lookup("ethcored.daemon.pidfile");
	} else {
		pidfile = std::string(ETHCORE_PID_FILE); // default
	}

	/*
	 * extract daemonize flag
	 */
	bool daemonize = true;
	if (env_parser.is_arg_set("daemonize")) {
		daemonize = atoi(env_parser.get_arg("pidfile").c_str());
	} else if (cconfig::get_instance().exists("ethcored.daemon.daemonize")) {
		daemonize = (bool)cconfig::get_instance().lookup("ethcored.daemon.daemonize");
	} else {
		daemonize = true; // default
	}



	/*
	 * daemonize
	 */
	if (daemonize) {
		rofl::cdaemon::daemonize(env_parser.get_arg("pidfile"), env_parser.get_arg("logfile"));
	}
	rofl::logging::set_debug_level(debug);
	if (daemonize) {
		rofl::logging::notice << "[ethcore][main] daemonizing successful" << std::endl;
	}


	rofcore::logging::set_debug_level(8);


	/*
	 * read configuration (for now: from libconfig++ file)
	 */

	cconfig& config = cconfig::get_instance();

	if (config.exists(ETHCORE_CONFIG_DPT_LIST)) {
		for (int i = 0; i < config.lookup(ETHCORE_CONFIG_DPT_LIST).getLength(); i++) {
			try {
				libconfig::Setting& datapath = config.lookup(ETHCORE_CONFIG_DPT_LIST)[i];

				// get data path dpid
				if (not datapath.exists("dpid")) {
					continue; // as we do not know the data path dpid
				}
				cdpid dpid( (int)datapath["dpid"] );

				// get default port vid
				uint16_t default_pvid = 1;
				if (datapath.exists("default_pvid")) {
					default_pvid = (int)datapath["default_pvid"];
				}

				// this is the cethcore instance for this data path
				ethcore::cethcore& ethcore = ethcore::cethcore::set_core(dpid);

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

				rofcore::logging::debug << "after config:" << std::endl << cethcore::get_core(dpid);


			} catch (libconfig::SettingNotFoundException& e) {

			}
		}
	}

	/*
	 * prepare OpenFlow socket for listening
	 */
	rofl::openflow::cofhello_elem_versionbitmap versionbitmap;
	if (cconfig::get_instance().exists("ethcored.openflow.version")) {
		versionbitmap.add_ofp_version((int)cconfig::get_instance().lookup("ethcored.openflow.version"));
	} else {
		versionbitmap.add_ofp_version(rofl::openflow13::OFP_VERSION);
	}

	rofl::logging::notice << "[ethcored][main] using OpenFlow version-bitmap:" << std::endl << versionbitmap;

	ethcore::cethbase& ethbase = ethcore::cethbase::get_instance(versionbitmap);

	//base.init(/*port-table-id=*/0, /*fib-in-table-id=*/1, /*fib-out-table-id=*/2, /*default-vid=*/1);

	std::stringstream portno;
	if (cconfig::get_instance().exists("ethcored.openflow.port")) {
		portno << (int)cconfig::get_instance().lookup("ethcored.openflow.port");
	} else {
		portno << (int)6653;
	}

	std::stringstream bindaddr;
	if (cconfig::get_instance().exists("ethcored.openflow.bindaddr")) {
		bindaddr << (const char*)cconfig::get_instance().lookup("ethcored.openflow.bindaddr");
	} else {
		bindaddr << "::";
	}

	enum rofl::csocket::socket_type_t socket_type = rofl::csocket::SOCKET_TYPE_PLAIN;
	rofl::cparams socket_params = rofl::csocket::get_default_params(socket_type);
	socket_params.set_param(rofl::csocket::PARAM_KEY_LOCAL_PORT).set_string(portno.str());
	socket_params.set_param(rofl::csocket::PARAM_KEY_LOCAL_HOSTNAME).set_string(bindaddr.str());

	ethbase.rpc_listen_for_dpts(socket_type, socket_params);


	/*
	 * start main loop, does not return
	 */
	rofl::cioloop::run();

	rofl::cioloop::shutdown();

	return 0;
}
