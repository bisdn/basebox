/*
 * crtaddr.cc
 *
 *  Created on: 27.06.2013
 *      Author: andreas
 */


#include "crtaddr.h"

using namespace rofcore;

crtaddr::crtaddr() :
		ifindex(0),
		af(0),
		prefixlen(0),
		scope(0),
		flags(0)
{
	//std::cerr << "crtaddr::crtaddr() " << (std::hex) << (int*)this << (std::dec) << std::endl;
}



crtaddr::~crtaddr()
{
	//std::cerr << "crtaddr::~crtaddr() " << (std::hex) << (int*)this << (std::dec) << std::endl;
}



crtaddr::crtaddr(crtaddr const& rtaddr) :
		ifindex(0),
		af(0),
		prefixlen(0),
		scope(0),
		flags(0)
{
	*this = rtaddr;
}



crtaddr&
crtaddr::operator= (crtaddr const& rtaddr)
{
	if (this == &rtaddr)
		return *this;

	label		= rtaddr.label;
	ifindex		= rtaddr.ifindex;
	af			= rtaddr.af;
	prefixlen	= rtaddr.prefixlen;
	scope		= rtaddr.scope;
	flags		= rtaddr.flags;

	return *this;
}



crtaddr::crtaddr(struct rtnl_addr* addr) :
		ifindex(0),
		af(0),
		prefixlen(0),
		scope(0),
		flags(0)
{
	char s_buf[128];
	memset(s_buf, 0, sizeof(s_buf));

	nl_object_get((struct nl_object*)addr); // increment reference counter by one

	if (rtnl_addr_get_label(addr)) label.assign(rtnl_addr_get_label(addr)); // label might be null in case of IPv6 addresses
	ifindex 	= rtnl_addr_get_ifindex(addr);
	af 			= rtnl_addr_get_family(addr);
	prefixlen 	= rtnl_addr_get_prefixlen(addr);
	scope 		= rtnl_addr_get_scope(addr);
	flags 		= rtnl_addr_get_flags(addr);

	nl_object_put((struct nl_object*)addr); // decrement reference counter by one
}



std::string
crtaddr::get_family_s() const
{
	std::string str;

	switch (af) {
	case AF_INET:	str = std::string("inet"); 		break;
	case AF_INET6:	str = std::string("inet6");		break;
	default:		str = std::string("unknown"); 	break;
	}

	return str;
}


