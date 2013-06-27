/*
 * crtaddr.cc
 *
 *  Created on: 27.06.2013
 *      Author: andreas
 */


#include <crtaddr.h>


using namespace dptmap;


crtaddr::crtaddr() :
		ifindex(0),
		af(0),
		prefixlen(0),
		scope(0),
		flags(0),
		local(AF_INET),
		peer(AF_INET),
		bcast(AF_INET)
{

}



crtaddr::~crtaddr()
{

}



crtaddr::crtaddr(crtaddr const& rtaddr) :
		ifindex(0),
		af(0),
		prefixlen(0),
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
		scope(0),
		flags(0),
		local(AF_INET),
		peer(AF_INET),
		bcast(AF_INET)
{
	char s_buf[128];
	memset(s_buf, 0, sizeof(s_buf));

	nl_object_get((struct nl_object*)addr); // increment reference counter by one

	label.assign(rtnl_addr_get_label(addr));
	ifindex 	= rtnl_addr_get_ifindex(addr);
	af 			= rtnl_addr_get_family(addr);
	prefixlen 	= rtnl_addr_get_prefixlen(addr);
	scope 		= rtnl_addr_get_scope(addr);
	flags 		= rtnl_addr_get_flags(addr);
	local 		= rofl::caddress(af, nl_addr2str(rtnl_addr_get_local(addr), 	s_buf, sizeof(s_buf)));
	peer 		= rofl::caddress(af, nl_addr2str(rtnl_addr_get_peer(addr), 		s_buf, sizeof(s_buf)));
	bcast 		= rofl::caddress(af, nl_addr2str(rtnl_addr_get_broadcast(addr), s_buf, sizeof(s_buf)));

	nl_object_put((struct nl_object*)addr); // decrement reference counter by one
}


