/*
 * dhcpv6snoop.h
 *
 *  Created on: 11.09.2013
 *      Author: andreas
 */

#ifndef DHCPV6SNOOP_H_
#define DHCPV6SNOOP_H_

#include <map>
#include <exception>

#include <rofl/common/ciosrv.h>

#include "cppcap.h"

using namespace rutils;

namespace dhcpv6snoop
{

class eDhcpv6SnoopBase : public std::exception {};
class eDhcpv6SnoopExists : public eDhcpv6SnoopBase {};
class eDhcpv6SnoopNotFound : public eDhcpv6SnoopBase {};

class cdhcpv6snoop :
		public rofl::ciosrv
{

	std::map<std::string, cppcap*> captures;

public:

	virtual
	~cdhcpv6snoop();

	void
	add_capture_device(const std::string& devname);

	void
	del_capture_device(const std::string& devname);
};

}; // end of namespace

#endif /* DHCPV6SNOOP_H_ */
