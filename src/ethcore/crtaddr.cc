/*
 * crtaddr.cc
 *
 *  Created on: 27.06.2013
 *      Author: andreas
 */


#include "crtaddr.h"


using namespace ethercore;


crtaddr::crtaddr() :
		ifindex(0),
		af(0),
		prefixlen(0),
		mask(AF_INET),
		scope(0),
		flags(0),
		local(AF_INET),
		peer(AF_INET),
		bcast(AF_INET)
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
		mask(AF_INET),
		scope(0),
		flags(0),
		local(AF_INET),
		peer(AF_INET),
		bcast(AF_INET)
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
	mask		= rtaddr.mask;
	scope		= rtaddr.scope;
	flags		= rtaddr.flags;
	local		= rtaddr.local;
	peer		= rtaddr.peer;
	bcast		= rtaddr.bcast;

	return *this;
}



crtaddr::crtaddr(struct rtnl_addr* addr) :
		ifindex(0),
		af(0),
		prefixlen(0),
		mask(AF_INET),
		scope(0),
		flags(0),
		local(AF_INET),
		peer(AF_INET),
		bcast(AF_INET)
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

	switch (af) {
	case AF_INET: {
		mask = rofl::caddress(AF_INET);
		mask.ca_s4addr->sin_addr.s_addr = htobe32(~((1 << (32 - prefixlen)) - 1));
	} break;
	case AF_INET6: {
		mask = rofl::caddress(AF_INET6);
		int segment 	= prefixlen / 32;
		int t_prefixlen = prefixlen % 32;
		for (int i = 0; i < 4; i++) {
			if (segment == i) {
				((uint32_t*)(mask.ca_s6addr->sin6_addr.s6_addr))[i] = htobe32(~((1 << (32 - t_prefixlen)) - 1));
			} else if (i > segment) {
				((uint32_t*)(mask.ca_s6addr->sin6_addr.s6_addr))[i] = htobe32(0x00000000);
			} else if (i < segment) {
				((uint32_t*)(mask.ca_s6addr->sin6_addr.s6_addr))[i] = htobe32(0xffffffff);
			}
		}
	} break;
	}

	std::string s_local(nl_addr2str(rtnl_addr_get_local(addr), 		s_buf, sizeof(s_buf)));
	std::string s_peer (nl_addr2str(rtnl_addr_get_peer(addr), 		s_buf, sizeof(s_buf)));
	std::string s_bcast(nl_addr2str(rtnl_addr_get_broadcast(addr),  s_buf, sizeof(s_buf)));

	s_local = s_local.substr(0, s_local.find_first_of("/", 0));
	s_peer  = s_peer .substr(0,  s_peer.find_first_of("/", 0));
	s_bcast = s_bcast.substr(0, s_bcast.find_first_of("/", 0));

	local 		= rofl::caddress(af, s_local.c_str());
	peer 		= rofl::caddress(af, s_peer.c_str());
	bcast 		= rofl::caddress(af, s_bcast.c_str());

	nl_object_put((struct nl_object*)addr); // decrement reference counter by one
}



bool
crtaddr::operator< (crtaddr const& rtaddr)
{
	return (local < rtaddr.local);
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


