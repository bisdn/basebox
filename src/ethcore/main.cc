#include <rofl/platform/unix/cunixenv.h>
#include "ethcore.h"

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

	ethercore::ethcore sw;

	sw.rpc_listen_for_dpts(caddress(AF_INET, "0.0.0.0", 6633));
	sw.rpc_listen_for_dpts(caddress(AF_INET, "0.0.0.0", 6632));

	sw.run();

	return 0;
}

