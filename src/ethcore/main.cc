#include <rofl/platform/unix/cunixenv.h>
#include "ethcore.h"
#include "qmfagent.h"

int
main(int argc, char** argv)
{
	/* update defaults */
	rofl::csyslog::initlog(
			rofl::csyslog::LOGTYPE_STDERR,
			rofl::csyslog::EMERGENCY,
			std::string("ethcore.log"),
			"an example: ");

	rofl::csyslog::set_debug_level("ciosrv", "emergency");
	rofl::csyslog::set_debug_level("cthread", "emergency");


	qmf::qmfagent::get_instance().init(argc, argv);

	ethercore::ethcore& core = ethercore::ethcore::get_instance();
	core.init(/*port-table-id=*/0, /*fib-in-table-id=*/1, /*fib-out-table-id=*/2, /*default-vid=*/1);
	core.rpc_listen_for_dpts(caddress(AF_INET, "0.0.0.0", 6633));
	core.rpc_listen_for_dpts(caddress(AF_INET, "0.0.0.0", 6632));

	rofl::ciosrv::run();

	return 0;
}

