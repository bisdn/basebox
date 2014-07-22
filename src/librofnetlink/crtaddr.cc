/*
 * crtaddr.cc
 *
 *  Created on: 27.06.2013
 *      Author: andreas
 */


#include "crtaddr.h"

using namespace rofcore;

/*static*/
unsigned int crtaddr::nextindex = 0;

/*static*/
std::map<unsigned int, crtaddr*> crtaddr::rtaddrs;

/*static*/
crtaddr&
crtaddr::get_addr(unsigned int adindex)
{
	std::map<unsigned int, crtaddr*>::iterator it;
	if ((it = find_if(crtaddr::rtaddrs.begin(), crtaddr::rtaddrs.end(),
			crtaddr_find_by_adindex(adindex))) == crtaddr::rtaddrs.end()) {
		throw eRtAddrNotFound();
	}
	return *(crtaddr::rtaddrs[adindex]);
}



/*static*/
crtaddr_in4&
crtaddr_in4::get_addr_in4(unsigned int adindex)
{
	std::map<unsigned int, crtaddr*>::iterator it;
	if ((it = find_if(crtaddr::rtaddrs.begin(), crtaddr::rtaddrs.end(),
			crtaddr_find_by_adindex(adindex))) == crtaddr::rtaddrs.end()) {
		throw eRtAddrNotFound("crtaddr_in4::get_addr_in4() / error: adindex not found");
	}
	if (!dynamic_cast<crtaddr_in4*>( crtaddr::rtaddrs[adindex] )) {
		throw eRtAddrNotFound("crtaddr_in4::get_addr_in4() / error: adindex found, but is IPv6 address");
	}
	return dynamic_cast<crtaddr_in4&>(*(crtaddr::rtaddrs[adindex]));
}



/*static*/
crtaddr_in6&
crtaddr_in6::get_addr_in6(unsigned int adindex)
{
	std::map<unsigned int, crtaddr*>::iterator it;
	if ((it = find_if(crtaddr::rtaddrs.begin(), crtaddr::rtaddrs.end(),
			crtaddr_find_by_adindex(adindex))) == crtaddr::rtaddrs.end()) {
		throw eRtAddrNotFound("crtaddr_in6::get_addr_in6() / error: adindex not found");
	}
	if (!dynamic_cast<crtaddr_in6*>( crtaddr::rtaddrs[adindex] )) {
		throw eRtAddrNotFound("crtaddr_in6::get_addr_in6() / error: adindex found, but is IPv4 address");
	}
	return dynamic_cast<crtaddr_in6&>(*(crtaddr::rtaddrs[adindex]));
}



crtaddr::crtaddr(struct rtnl_addr* addr) :
		adindex(crtaddr::nextindex++),
		ifindex(0),
		af(0),
		prefixlen(0),
		scope(0),
		flags(0)
{
	if (crtaddr::rtaddrs.find(adindex) != crtaddr::rtaddrs.end()) {
		throw eRtAddrExists();
	}
	crtaddr::rtaddrs[adindex] = this;

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



crtaddr::~crtaddr()
{
	crtaddr::rtaddrs.erase(adindex);
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


