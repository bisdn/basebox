/*
 * dhcpv6snoop.cc
 *
 *  Created on: 11.09.2013
 *      Author: andreas
 */

#include "cdhcpv6snoop.h"

using namespace dhcpv6snoop;


cdhcpv6snoop::~cdhcpv6snoop()
{
	for (std::map<std::string, cppcap*>::iterator
			it = captures.begin(); it != captures.end(); ++it) {
		delete it->second;
	}
	captures.clear();
}



void
cdhcpv6snoop::add_capture_device(const std::string& devname)
{
	if (captures.find(devname) != captures.end()) {
		throw eDhcpv6SnoopExists();
	}
	captures[devname] = new cppcap();

	captures[devname]->start(devname);
}



void
cdhcpv6snoop::del_capture_device(const std::string& devname)
{
	if (captures.find(devname) == captures.end()) {
		return;
	}
	captures[devname]->stop();

	delete captures[devname];

	captures.erase(devname);
}




