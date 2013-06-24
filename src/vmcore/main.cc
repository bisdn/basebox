//#include "cnetlink.h"
#include <vmcore.h>

int
main(int argc, char** argv)
{
	dptmap::vmcore core;

	core.rpc_listen_for_dpts(rofl::caddress(AF_INET, "0.0.0.0", 6633));

	//cnetlink netlink(&core);

	rofl::ciosrv::run();
}


