#include <rofl/common/ciosrv.h>
#include <rofl/platform/unix/cdaemon.h>
#include <rofl/platform/unix/cunixenv.h>

#include "ethcore.h"
#ifdef AMQP_QMF_SUPPORT
#include "qmfagent.h"
#endif
#include "cconfig.h"

#define ETHCORE_LOG_FILE "/var/log/ethcored.log"
#define ETHCORE_PID_FILE "/var/run/ethcored.pid"
#define ETHCORE_CONF_FILE "/usr/local/etc/ethcored.conf"

int
main(int argc, char** argv)
{
	rofl::cunixenv env_parser(argc, argv);

#if 0
	/* update defaults */
	env_parser.update_default_option("logfile", ETHCORE_LOG_FILE);
	env_parser.update_default_option("config-file", ETHCORE_CONFIG_FILE);
	env_parser.add_option(rofl::coption(true, REQUIRED_ARGUMENT, 'i', "pidfile", "set pid-file", std::string(ETHCORE_PID_FILE)));
	env_parser.add_option(rofl::coption(true, REQUIRED_ARGUMENT, 'p', "port", "set port", ""+6653));
#endif

	/* update defaults */
	env_parser.add_option(rofl::coption(true, REQUIRED_ARGUMENT, 'l', "logfile", "set log-file", std::string(ETHCORE_LOG_FILE)));
	env_parser.add_option(rofl::coption(true, REQUIRED_ARGUMENT, 'p', "pidfile", "set pid-file", std::string(ETHCORE_PID_FILE)));
	env_parser.add_option(rofl::coption(true, REQUIRED_ARGUMENT, 'c', "conffile", "set conf-file", std::string(ETHCORE_CONF_FILE)));
	env_parser.add_option(rofl::coption(true, NO_ARGUMENT, 'D', "daemon", "daemonize", ""));
	env_parser.add_option(rofl::coption(true, NO_ARGUMENT, 'V', "version", "show version and build number", ""));


	// command line arguments
	env_parser.parse_args();

	// configuration file
	ethercore::cconfig::get_instance().open(env_parser.get_arg("conffile"));


	/*
	 * extract debug level
	 */
	int debug;
	if (env_parser.is_arg_set("debug")) {
		debug = atoi(env_parser.get_arg("debug").c_str());
	} else if (ethercore::cconfig::get_instance().exists("ethcored.daemon.debug")) {
		debug = (int)ethercore::cconfig::get_instance().lookup("ethcored.daemon.debug");
	} else {
		debug = 0; // default
	}

	/*
	 * extract log-file
	 */
	std::string logfile;
	if (env_parser.is_arg_set("logfile")) {
		logfile = env_parser.get_arg("logfile");
	} else if (ethercore::cconfig::get_instance().exists("ethcored.daemon.logfile")) {
		logfile = (const char*)ethercore::cconfig::get_instance().lookup("ethcored.daemon.logfile");
	} else {
		logfile = std::string(ETHCORE_LOG_FILE); // default
	}

	/*
	 * extract pid-file
	 */
	std::string pidfile;
	if (env_parser.is_arg_set("pidfile")) {
		pidfile = env_parser.get_arg("pidfile");
	} else if (ethercore::cconfig::get_instance().exists("ethcored.daemon.pidfile")) {
		pidfile = (const char*)ethercore::cconfig::get_instance().lookup("ethcored.daemon.pidfile");
	} else {
		pidfile = std::string(ETHCORE_PID_FILE); // default
	}

	/*
	 * extract daemonize flag
	 */
	bool daemonize;
	if (env_parser.is_arg_set("daemonize")) {
		daemonize = atoi(env_parser.get_arg("pidfile").c_str());
	} else if (ethercore::cconfig::get_instance().exists("ethcored.daemon.daemonize")) {
		daemonize = (bool)ethercore::cconfig::get_instance().lookup("ethcored.daemon.daemonize");
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




#ifdef AMQP_QMF_SUPPORT
	qmf::qmfagent::get_instance().init(argc, argv);
#endif

	cofhello_elem_versionbitmap versionbitmap;
	if (ethercore::cconfig::get_instance().exists("ethcored.openflow.version")) {
		versionbitmap.add_ofp_version((int)ethercore::cconfig::get_instance().lookup("ethcored.openflow.version"));
	} else {
		versionbitmap.add_ofp_version(rofl::openflow12::OFP_VERSION);
	}

	rofl::logging::notice << "[ethcore][main] using OpenFlow version-bitmap:" << std::endl << versionbitmap;

	ethercore::ethcore& core = ethercore::ethcore::get_instance(versionbitmap);

	core.init(/*port-table-id=*/0, /*fib-in-table-id=*/1, /*fib-out-table-id=*/2, /*default-vid=*/1);

	uint16_t portno = 6633;
	if (ethercore::cconfig::get_instance().exists("ethcored.openflow.port")) {
		portno = (int)ethercore::cconfig::get_instance().lookup("ethcored.openflow.port");
	}
	std::stringstream s_portno; s_portno << portno;

	enum rofl::csocket::socket_type_t socket_type = rofl::csocket::SOCKET_TYPE_PLAIN;
	rofl::cparams socket_params = rofl::csocket::get_default_params(socket_type);
	socket_params.set_param(rofl::csocket::PARAM_KEY_LOCAL_PORT).set_string(s_portno.str());

	core.rpc_listen_for_dpts(socket_type, socket_params);

	rofl::cioloop::run();

	rofl::cioloop::shutdown();

	return 0;
}

