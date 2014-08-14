/*
 * cipbase.cpp
 *
 *  Created on: 05.08.2014
 *      Author: andreas
 */

#include "cipbase.hpp"

#include <rofl/common/ciosrv.h>
#include <rofl/platform/unix/cdaemon.h>
#include <rofl/platform/unix/cunixenv.h>

#define IPCORE_LOG_FILE "/var/log/ipcored.log"
#define IPCORE_PID_FILE "/var/run/ipcored.pid"
#define IPCORE_CONFIG_FILE "/usr/local/etc/ipcored.conf"

#include "cipbase.hpp"

using namespace ipcore;

/*static*/cipbase* cipbase::ipbase = (cipbase*)0;


/*static*/int
cipbase::run(int argc, char** argv)
{
	rofl::cunixenv env_parser(argc, argv);

	/* update defaults */
	env_parser.add_option(rofl::coption(true, NO_ARGUMENT,'D',"daemonize","daemonize",""));
	env_parser.add_option(rofl::coption(true, REQUIRED_ARGUMENT, 'l', "logfile", "set log-file", std::string(IPCORE_LOG_FILE)));
	env_parser.add_option(rofl::coption(true, REQUIRED_ARGUMENT, 'c', "config-file", "set config-file", std::string(IPCORE_CONFIG_FILE)));
	env_parser.add_option(rofl::coption(true, REQUIRED_ARGUMENT, 'i', "pidfile", "set pid-file", std::string(IPCORE_PID_FILE)));
	env_parser.add_option(rofl::coption(true, REQUIRED_ARGUMENT, 'p', "port", "set port", ""+6653));

	// command line arguments
	env_parser.parse_args();

	// configuration file
	cconfig::get_instance().open(env_parser.get_arg("config-file"));


	/*
	 * extract debug level
	 */
	int rofl_debug = 0;
	if (cconfig::get_instance().exists("ipcored.daemon.logging.rofl.debug")) {
		rofl_debug = (int)cconfig::get_instance().lookup("ipcored.daemon.logging.rofl.debug");
	}
	int core_debug = 0;
	if (cconfig::get_instance().exists("ipcored.daemon.logging.core.debug")) {
		core_debug = (int)cconfig::get_instance().lookup("ipcored.daemon.logging.core.debug");
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
	} else if (cconfig::get_instance().exists("ipcored.daemon.logfile")) {
		logfile = (const char*)cconfig::get_instance().lookup("ipcored.daemon.logfile");
	} else {
		logfile = std::string(IPCORE_LOG_FILE); // default
	}

	/*ETH
	 * extract pid-file
	 */
	std::string pidfile;
	if (env_parser.is_arg_set("pidfile")) {
		pidfile = env_parser.get_arg("pidfile");
	} else if (cconfig::get_instance().exists("ipcored.daemon.pidfile")) {
		pidfile = (const char*)cconfig::get_instance().lookup("ipcored.daemon.pidfile");
	} else {
		pidfile = std::string(IPCORE_PID_FILE); // default
	}

	/*
	 * extract daemonize flag
	 */
	bool daemonize = true;
	if (env_parser.is_arg_set("daemonize")) {
		daemonize = atoi(env_parser.get_arg("pidfile").c_str());
	} else if (cconfig::get_instance().exists("ipcored.daemon.daemonize")) {
		daemonize = (bool)cconfig::get_instance().lookup("ipcored.daemon.daemonize");
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
		rofcore::logging::notice << "[ipcored][main] daemonizing successful" << std::endl;
	}





	// create cipcore instance with defined OpenFlow version
	rofl::openflow::cofhello_elem_versionbitmap versionbitmap;
	if (cconfig::get_instance().exists("ipcored.openflow.version")) {
		versionbitmap.add_ofp_version((int)cconfig::get_instance().lookup("ipcored.openflow.version"));
	} else {
		versionbitmap.add_ofp_version(rofl::openflow13::OFP_VERSION);
	}


	std::stringstream portno;
	if (cconfig::get_instance().exists("ipcored.openflow.bindport")) {
		portno << (int)cconfig::get_instance().lookup("ipcored.openflow.bindport");
	} else {
		portno << (int)6653;
	}

	std::stringstream bindaddr;
	if (cconfig::get_instance().exists("ipcored.openflow.bindaddr")) {
		bindaddr << (const char*)cconfig::get_instance().lookup("ipcored.openflow.bindaddr");
	} else {
		bindaddr << "::";
	}

	enum rofl::csocket::socket_type_t socket_type = rofl::csocket::SOCKET_TYPE_PLAIN;
	rofl::cparams socket_params = rofl::csocket::get_default_params(socket_type);
	socket_params.set_param(rofl::csocket::PARAM_KEY_LOCAL_PORT).set_string(portno.str());
	socket_params.set_param(rofl::csocket::PARAM_KEY_LOCAL_HOSTNAME).set_string(bindaddr.str());

	ipcore::cipbase::get_instance(versionbitmap).rpc_listen_for_dpts(socket_type, socket_params);



	// enter main loop
	rofl::cioloop::run();

	// clean-up and terminate
	rofl::cioloop::shutdown();

	return 0;
}
