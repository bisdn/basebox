//#include "cnetlink.h"
#include "ipcore.h"
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

	if (not env_parser.is_arg_set("daemonize")) {

		rofl::logging::set_debug_level(atoi(env_parser.get_arg("debug").c_str()));

	} else {

		rofl::cdaemon::daemonize(env_parser.get_arg("pidfile"), env_parser.get_arg("logfile"));
		rofl::logging::set_debug_level(atoi(env_parser.get_arg("debug").c_str()));
		rofl::logging::notice << "[vmcore][main] daemonizing successful" << std::endl;
	}

	cofhello_elem_versionbitmap versionbitmap;
	if (iprotcore::cconfig::get_instance().exists("ipcored.openflow.version")) {
		versionbitmap.add_ofp_version((int)iprotcore::cconfig::get_instance().lookup("ipcored.openflow.version"));
	} else {
		versionbitmap.add_ofp_version(rofl::openflow12::OFP_VERSION);
	}
	dptmap::ipcore core(versionbitmap);

	core.rpc_listen_for_dpts(rofl::caddress(AF_INET, "0.0.0.0", atoi(env_parser.get_arg("port").c_str())));

	rofl::cioloop::run();

	rofl::cioloop::shutdown();

	return 0;
}


