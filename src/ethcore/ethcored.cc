
#include <rofl/common/ciosrv.h>
#include <rofl/platform/unix/cdaemon.h>
#include <rofl/platform/unix/cunixenv.h>

#include "ethcore.h"
#ifdef AMQP_QMF_SUPPORT
//#include "qmfagent.h"
#endif

#define ETHCORE_LOG_FILE "/var/log/ethcored.log"
#define ETHCORE_PID_FILE "/var/run/ethcored.pid"

int
main(int argc, char** argv)
{
	rofl::cunixenv env_parser(argc, argv);

	/* update defaults */
	env_parser.update_default_option("logfile", ETHCORE_LOG_FILE);
	env_parser.add_option(rofl::coption(true, REQUIRED_ARGUMENT, 'i', "pidfile", "set pid-file", std::string(ETHCORE_PID_FILE)));
	env_parser.add_option(rofl::coption(true, REQUIRED_ARGUMENT, 'p', "port", "set port", ""+6653));

	//Parse
	env_parser.parse_args();

	if (not env_parser.is_arg_set("daemonize")) {

		rofl::logging::set_debug_level(atoi(env_parser.get_arg("debug").c_str()));

	} else {

		rofl::cdaemon::daemonize(env_parser.get_arg("pidfile"), env_parser.get_arg("logfile"));
		rofl::logging::set_debug_level(atoi(env_parser.get_arg("debug").c_str()));
		rofl::logging::notice << "[ethcore][main] daemonizing successful" << std::endl;
	}

#ifdef AMQP_QMF_SUPPORT
	qmf::qmfagent::get_instance().init(argc, argv);
#endif

	cofhello_elem_versionbitmap versionbitmap;
	versionbitmap.add_ofp_version(rofl::openflow12::OFP_VERSION);
	ethercore::ethcore& core = ethercore::ethcore::get_instance(versionbitmap);

	core.init(/*port-table-id=*/0, /*fib-in-table-id=*/1, /*fib-out-table-id=*/2, /*default-vid=*/1);

	core.rpc_listen_for_dpts(rofl::caddress(AF_INET, "0.0.0.0", atoi(env_parser.get_arg("port").c_str())));

	rofl::cioloop::run();

	rofl::cioloop::shutdown();

	return 0;
}

