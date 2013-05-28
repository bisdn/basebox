#include "cnetlink.h"

int
main(int argc, char** argv)
{
	cnetlink_owner *owner = 0;

	cnetlink netlink(owner);

	netlink.get_route_notifications();

	rofl::ciosrv::run();
}


