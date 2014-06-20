//#include "cnetlink.h"
#include "ipcore.h"
#include "cconfig.h"

#include <rofl/common/ciosrv.h>
#include <rofl/platform/unix/cdaemon.h>
#include <rofl/platform/unix/cunixenv.h>

#define IPCORE_LOG_FILE "/var/log/ipcored.log"
#define IPCORE_PID_FILE "/var/run/ipcored.pid"
#define IPCORE_CONF_FILE "/usr/local/etc/ipcored.conf"

int
main(int argc, char** argv)
{
	rofl::cunixenv env_parser(argc, argv);

#if 0
	/* update defaults */
	env_parser.update_default_option("logfile", IPCORE_LOG_FILE);
	env_parser.add_option(rofl::coption(true, REQUIRED_ARGUMENT, 'i', "pidfile", "set pid-file", std::string(IPCORE_PID_FILE)));
	env_parser.add_option(rofl::coption(true, REQUIRED_ARGUMENT, 'p', "port", "set port", ""+6653));
#endif

	/* update defaults */
	env_parser.add_option(rofl::coption(true, REQUIRED_ARGUMENT, 'l', "logfile", "set log-file", std::string(IPCORE_LOG_FILE)));
	env_parser.add_option(rofl::coption(true, REQUIRED_ARGUMENT, 'p', "pidfile", "set pid-file", std::string(IPCORE_PID_FILE)));
	env_parser.add_option(rofl::coption(true, REQUIRED_ARGUMENT, 'c', "conffile", "set conf-file", std::string(IPCORE_CONF_FILE)));
	env_parser.add_option(rofl::coption(true, NO_ARGUMENT, 'D', "daemon", "daemonize", ""));
	env_parser.add_option(rofl::coption(true, NO_ARGUMENT, 'V', "version", "show version and build number", ""));

	//Parse
	env_parser.parse_args();

	// configuration file
	iprotcore::cconfig::get_instance().open(env_parser.get_arg("conffile"));


	if (not env_parser.is_arg_set("daemonize")) {

		rofl::logging::set_debug_level(atoi(env_parser.get_arg("debug").c_str()));

	} else {

		rofl::cdaemon::daemonize(env_parser.get_arg("pidfile"), env_parser.get_arg("logfile"));
		rofl::logging::set_debug_level(atoi(env_parser.get_arg("debug").c_str()));
		rofl::logging::notice << "[ipcore][main] daemonizing successful" << std::endl;
	}

	rofl::openflow::cofhello_elem_versionbitmap versionbitmap;
	if (iprotcore::cconfig::get_instance().exists("ipcored.openflow.version")) {
		versionbitmap.add_ofp_version((int)iprotcore::cconfig::get_instance().lookup("ipcored.openflow.version"));
	} else {
		versionbitmap.add_ofp_version(rofl::openflow12::OFP_VERSION);
	}
	dptmap::ipcore core(versionbitmap);

	uint16_t portno = 6633;
	if (iprotcore::cconfig::get_instance().exists("ipcored.openflow.port")) {
		portno = (int)iprotcore::cconfig::get_instance().lookup("ipcored.openflow.port");
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


