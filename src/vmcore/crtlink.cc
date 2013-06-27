/*
 * crtlink.cc
 *
 *  Created on: 27.06.2013
 *      Author: andreas
 */

#include <crtlink.h>

using namespace dptmap;


crtlink::crtlink() :
		flags(0),
		af(0),
		arptype(0),
		ifindex(0),
		mtu(0)
{

}


crtlink::~crtlink()
{

}


crtlink::crtlink(crtlink const& rtlink) :
		flags(0),
		af(0),
		arptype(0),
		ifindex(0),
		mtu(0)
{
	*this = rtlink;
}



crtlink&
crtlink::operator= (crtlink const& rtlink)
{
	if (this == &rtlink)
		return *this;

	devname		= rtlink.devname;
	maddr		= rtlink.maddr;
	bcast		= rtlink.bcast;
	flags		= rtlink.flags;
	af			= rtlink.af;
	arptype		= rtlink.arptype;
	ifindex		= rtlink.ifindex;
	mtu			= rtlink.mtu;

	return *this;
}



crtlink::crtlink(struct rtnl_link *link) :
		flags(0),
		af(0),
		arptype(0),
		ifindex(0),
		mtu(0)
{
	char s_buf[18];
	memset(s_buf, 0, sizeof(s_buf));

	nl_object_get((struct nl_object*)link); // increment reference counter by one

	devname.assign(rtnl_link_get_name(link));
	maddr 	= rofl::cmacaddr(nl_addr2str(rtnl_link_get_addr(link), 		s_buf, sizeof(s_buf)));
	bcast 	= rofl::cmacaddr(nl_addr2str(rtnl_link_get_broadcast(link), s_buf, sizeof(s_buf)));
	flags 	= rtnl_link_get_flags(link);
	af 		= rtnl_link_get_family(link);
	arptype = rtnl_link_get_arptype(link);
	ifindex	= rtnl_link_get_ifindex(link);
	mtu 	= rtnl_link_get_mtu(link);

	nl_object_put((struct nl_object*)link); // decrement reference counter by one
}


