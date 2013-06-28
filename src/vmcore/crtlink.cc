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

	addrs.clear();

	for (std::map<uint16_t, crtaddr>::const_iterator
			it = rtlink.addrs.begin(); it != rtlink.addrs.end(); ++it) {
		addrs[it->first] = it->second;
	}

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
	char s_buf[128];
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



crtaddr&
crtlink::get_addr(uint16_t index)
{
	if (addrs.find(index) == addrs.end())
		throw eRtLinkNotFound();
	return addrs[index];
}



uint16_t
crtlink::set_addr(crtaddr const& rta)
{
	// address already exists
	std::map<uint16_t, crtaddr>::iterator it;
	if ((it = find_if(addrs.begin(), addrs.end(),
			crtaddr::crtaddr_find_by_local_addr(rta.get_local_addr()))) != addrs.end()) {

		//std::cerr << "crtlink::set_addr() route/addr: NL-ACT-CHG => index=" << it->first << " " << rta << std::endl;
		std::cerr << "crtlink::set_addr() route/addr: NL-ACT-CHG => " << *this << std::endl;
		it->second = rta;
		return it->first;

	} else {

		unsigned int index = 0;
		while (addrs.find(index) != addrs.end()) {
			index++;
		}
		//std::cerr << "crtlink::set_addr() route/addr: NL-ACT-NEW => index=" << index << " " << rta << std::endl;
		std::cerr << "crtlink::set_addr() route/addr: NL-ACT-NEW => " << *this << std::endl;
		addrs[index] = rta;
		return index;
	}
}



void
crtlink::del_addr(uint16_t index)
{
	if (CRTLINK_ADDR_ALL == index) {
		//std::cerr << "crtlink::del_addr() route/addr: NL-ACT-DEL => index=" << index << std::endl;
		addrs.clear();
	} else {
		//std::cerr << "crtlink::del_addr() route/addr: NL-ACT-DEL => index=" << index << std::endl;
		if (addrs.find(index) == addrs.end())
			return;
		addrs.erase(index);
	}
	std::cerr << "crtlink::set_addr() route/addr: NL-ACT-DEL => " << *this << std::endl;
}



void
crtlink::del_addr(crtaddr const& rta)
{
	// find address
	std::map<uint16_t, crtaddr>::iterator it;
	if ((it = find_if(addrs.begin(), addrs.end(),
			crtaddr::crtaddr_find_by_local_addr(rta.get_local_addr()))) == addrs.end()) {
		throw eRtLinkNotFound();
	}
	//std::cerr << "crtlink::del_addr() route/addr: NL-ACT-DEL => index=" << it->first << std::endl;
	addrs.erase(it);
}


