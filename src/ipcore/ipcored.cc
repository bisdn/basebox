//#include "cnetlink.h"
#include "cipcore.h"
#include "cconfig.h"

#include <rofl/common/ciosrv.h>
#include <rofl/platform/unix/cdaemon.h>
#include <rofl/platform/unix/cunixenv.h>

#define IPCORE_LOG_FILE "/var/log/ipcored.log"
#define IPCORE_PID_FILE "/var/run/ipcored.pid"

int
main(int argc, char** argv)
{
	rofl::cunixenv env_parser(argc, argv);

	/* update defaults */
	env_parser.update_default_option("logfile", IPCORE_LOG_FILE);
	env_parser.add_option(rofl::coption(true, REQUIRED_ARGUMENT, 'i', "pidfile", "set pid-file", std::string(IPCORE_PID_FILE)));
	env_parser.add_option(rofl::coption(true, REQUIRED_ARGUMENT, 'p', "port", "set port", ""+6653));

	//Parse
	env_parser.parse_args();


	// daemonize or stay on console
	if (not env_parser.is_arg_set("daemonize")) {

		rofl::logging::set_debug_level(atoi(env_parser.get_arg("debug").c_str()));

	} else {

		rofl::cdaemon::daemonize(env_parser.get_arg("pidfile"), env_parser.get_arg("logfile"));
		rofl::logging::set_debug_level(atoi(env_parser.get_arg("debug").c_str()));
		rofl::logging::notice << "[ipcore][main] daemonizing successful" << std::endl;
	}


	// create cipcore instance with defined OpenFlow version
	rofl::openflow::cofhello_elem_versionbitmap versionbitmap;
	if (rofcore::cconfig::get_instance().exists("ipcored.openflow.version")) {
		versionbitmap.add_ofp_version((int)rofcore::cconfig::get_instance().lookup("ipcored.openflow.version"));
	} else {
		versionbitmap.add_ofp_version(rofl::openflow12::OFP_VERSION);
	}
	ipcore::cipcore core(versionbitmap);


	// prepare control socket and listen for incoming control connections
	enum rofl::csocket::socket_type_t socket_type = rofl::csocket::SOCKET_TYPE_PLAIN;
	rofl::cparams socket_params = rofl::csocket::get_default_params(socket_type);
	socket_params.set_param(rofl::csocket::PARAM_KEY_LOCAL_PORT).set_string() = env_parser.get_arg("port");
	core.rpc_listen_for_dpts(socket_type, socket_params);


	// enter main loop
	rofl::cioloop::run();

	// clean-up and terminate
	rofl::cioloop::shutdown();

	return 0;
}


