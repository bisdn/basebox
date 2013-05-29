#include "cnetlink.h"
#include "vmcore.h"

int
main(int argc, char** argv)
{
	vmcore core;

	cnetlink netlink(&core);

	rofl::ciosrv::run();
}


