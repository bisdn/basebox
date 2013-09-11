/*
 * main.cc
 *
 *  Created on: 11.09.2013
 *      Author: andreas
 */

#include "cdhcpv6snoop.h"

int
main(int argc, char** argv)
{
	dhcpv6snoop::cdhcpv6snoop snoop;

	snoop.add_capture_device("ge1");

	rofl::ciosrv::run();

	return 0;
}



