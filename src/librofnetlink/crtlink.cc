/*
 * crtlink.cc
 *
 *  Created on: 27.06.2013
 *      Author: andreas
 */

#include "crtlink.h"

using namespace rofcore;


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



crtaddr_in4&
crtlink::get_addr_in4(uint16_t index)
{
	if (addrs_in4.find(index) == addrs_in4.end())
		throw eRtLinkNotFound();
	return addrs_in4[index];
}



uint16_t
crtlink::get_addr_in4(const crtaddr_in4& rta)
{
	std::map<uint16_t, crtaddr_in4>::iterator it;
	if ((it = find_if(addrs_in4.begin(), addrs_in4.end(),
			crtaddr_in4::crtaddr_in4_find_by_local_addr(rta.get_local_addr()))) == addrs_in4.end()) {
		throw eRtLinkNotFound();
	}
	return it->first;
}



uint16_t
crtlink::set_addr_in4(const crtaddr_in4& rta)
{if (crtlink::rtlinks.find(ifindex) == crtlink::rtlinks.end()) {
	// address already exists
	std::map<uint16_t, crtaddr_in4>::iterator it;
	if ((it = find_if(addrs_in4.begin(), addrs_in4.end(),
			crtaddr_in4::crtaddr_in4_find_by_local_addr(rta.get_local_addr()))) != addrs_in4.end()) {

		//std::cerr << "crtlink::set_addr() route/addr: NL-ACT-CHG => index=" << it->first << " " << rta << std::endl;
		//std::cerr << "crtlink::set_addr() route/addr: NL-ACT-CHG => " << *this << std::endl;
		it->second = rta;
		return it->first;

	}

	unsigned int adindex = 0;
	while (addrs_in4.find(adindex) != addrs_in4.end()) {
		adindex++;
	}
	//std::cerr << "crtlink::set_addr() route/addr: NL-ACT-NEW => index=" << index << " " << rta << std::endl;
	//std::cerr << "crtlink::set_addr() route/addr: NL-ACT-NEW => " << *this << std::endl;
	addrs_in4[adindex] = rta;
	return adindex;
}



uint16_t
crtlink::del_addr_in4(uint16_t index)
{
	if (crtaddr_in4::CRTLINK_ADDR_ALL == index) {
		std::cerr << "crtlink::del_addr() " << *this << std::endl;
		addrs_in4.clear();
	} else {
		std::cerr << "crtlink::del_addr() route/addr: NL-ACT-DEL => index=" << index << std::endl;
		if (addrs_in4.find(index) == addrs_in4.end())
			throw eRtLinkNotFound();
		addrs_in4.erase(index);
	}
	std::cerr << "crtlink::set_addr() route/addr: NL-ACT-DEL => " << *this << std::endl;
	return index;
}



uint16_t
crtlink::del_addr_in4(const crtaddr_in4& rta)
{
	// find address
	std::map<uint16_t, crtaddr_in4>::iterator it;
	if ((it = find_if(addrs_in4.begin(), addrs_in4.end(),
			crtaddr_in4::crtaddr_in4_find_by_local_addr(rta.get_local_addr()))) == addrs_in4.end()) {
		throw eRtLinkNotFound();
	}
	uint16_t index = it->first;
	//std::cerr << "crtlink::del_addr() route/addr: NL-ACT-DEL => index=" << it->first << std::endl;
	addrs_in4.erase(it);
	return index;
}



crtaddr_in6&
crtlink::get_addr_in6(uint16_t index)
{
	if (addrs_in6.find(index) == addrs_in6.end())
		throw eRtLinkNotFound();
	return addrs_in6[index];
}



uint16_t
crtlink::get_addr_in6(const crtaddr_in6& rta)
{
	std::map<uint16_t, crtaddr_in6>::iterator it;
	if ((it = find_if(addrs_in6.begin(), addrs_in6.end(),
			crtaddr_in6::crtaddr_in6_find_by_local_addr(rta.get_local_addr()))) == addrs_in6.end()) {
		throw eRtLinkNotFound();
	}
	return it->first;
}



uint16_t
crtlink::set_addr_in6(const crtaddr_in6& rta)
{
	// address already exists
	std::map<uint16_t, crtaddr_in6>::iterator it;
	if ((it = find_if(addrs_in6.begin(), addrs_in6.end(),
			crtaddr_in6::crtaddr_in6_find_by_local_addr(rta.get_local_addr()))) != addrs_in6.end()) {

		//std::cerr << "crtlink::set_addr() route/addr: NL-ACT-CHG => index=" << it->first << " " << rta << std::endl;
		//std::cerr << "crtlink::set_addr() route/addr: NL-ACT-CHG => " << *this << std::endl;
		it->second = rta;
		return it->first;

	} else {

		unsigned int index = 0;
		while (addrs_in6.find(index) != addrs_in6.end()) {
			index++;
		}
		//std::cerr << "crtlink::set_addr() route/addr: NL-ACT-NEW => index=" << index << " " << rta << std::endl;
		//std::cerr << "crtlink::set_addr() route/addr: NL-ACT-NEW => " << *this << std::endl;
		addrs_in6[index] = rta;
		return index;
	}
}



uint16_t
crtlink::del_addr_in6(uint16_t index)
{
	if (crtaddr_in6::CRTLINK_ADDR_ALL == index) {
		std::cerr << "crtlink::del_addr() " << *this << std::endl;
		addrs_in6.clear();
	} else {
		std::cerr << "crtlink::del_addr() route/addr: NL-ACT-DEL => index=" << index << std::endl;
		if (addrs_in6.find(index) == addrs_in6.end())
			throw eRtLinkNotFound();
		addrs_in6.erase(index);
	}
	std::cerr << "crtlink::set_addr() route/addr: NL-ACT-DEL => " << *this << std::endl;
	return index;
}



uint16_t
crtlink::del_addr_in6(const crtaddr_in6& rta)
{
	// find address
	std::map<uint16_t, crtaddr_in6>::iterator it;
	if ((it = find_if(addrs_in6.begin(), addrs_in6.end(),
			crtaddr_in6::crtaddr_in6_find_by_local_addr(rta.get_local_addr()))) == addrs_in6.end()) {
		throw eRtLinkNotFound();
	}
	uint16_t index = it->first;
	//std::cerr << "crtlink::del_addr() route/addr: NL-ACT-DEL => index=" << it->first << std::endl;
	addrs_in6.erase(it);
	return index;
}




crtneigh_in4&
crtlink::get_neigh_in4(uint16_t index)
{
	if (neighs_in4.find(index) == neighs_in4.end())
		throw eRtLinkNotFound();
	return neighs_in4[index];
}



crtneigh_in4&
crtlink::get_neigh_in4(const rofl::caddress_in4& dst)
{
	std::map<uint16_t, crtneigh_in4>::iterator it;
	if ((it = find_if(neighs_in4.begin(), neighs_in4.end(),
			crtneigh_in4::crtneigh_in4_find_by_dst(dst))) == neighs_in4.end()) {
		throw eRtLinkNotFound();
	}
	return it->second;
}



uint16_t
crtlink::get_neigh_in4_index(const crtneigh_in4& neigh)
{
	std::map<uint16_t, crtneigh_in4>::iterator it;
	if ((it = find_if(neighs_in4.begin(), neighs_in4.end(),
			crtneigh_in4::crtneigh_in4_find_by_dst(neigh.get_dst()))) == neighs_in4.end()) {
		throw eRtLinkNotFound();
	}
	return it->first;
}



uint16_t
crtlink::get_neigh_in4_index(const rofl::caddress_in4& dst)
{
	std::map<uint16_t, crtneigh_in4>::iterator it;
	if ((it = find_if(neighs_in4.begin(), neighs_in4.end(),
			crtneigh_in4::crtneigh_in4_find_by_dst(dst))) == neighs_in4.end()) {
		throw eRtLinkNotFound();
	}
	return it->first;
}


uint16_t
crtlink::set_neigh_in4(const crtneigh_in4& neigh)
{
	// neighess already exists
	std::map<uint16_t, crtneigh_in4>::iterator it;
	if ((it = find_if(neighs_in4.begin(), neighs_in4.end(),
			crtneigh_in4::crtneigh_in4_find_by_dst(neigh.get_dst()))) != neighs_in4.end()) {

		//std::cerr << "crtlink::set_neigh() route/neigh: NL-ACT-CHG => index=" << it->first << " " << rta << std::endl;
		//std::cerr << "crtlink::set_neigh() route/neigh: NL-ACT-CHG => " << *this << std::endl;
		it->second = neigh;
		return it->first;

	} else {

		unsigned int index = 0;
		while (neighs_in4.find(index) != neighs_in4.end()) {
			index++;
		}
		//std::cerr << "crtlink::set_neigh() route/neigh: NL-ACT-NEW => index=" << index << " " << rta << std::endl;
		//std::cerr << "crtlink::set_neigh() route/neigh: NL-ACT-NEW => " << *this << std::endl;
		neighs_in4[index] = neigh;
		return index;
	}
}



uint16_t
crtlink::del_neigh_in4(uint16_t index)
{
	if (crtneigh::CRTNEIGH_ADDR_ALL == index) {
		//std::cerr << "crtlink::del_neigh() route/neigh: NL-ACT-DEL => index=" << index << std::endl;
		neighs_in4.clear();
	} else {
		//std::cerr << "crtlink::del_neigh() route/neigh: NL-ACT-DEL => index=" << index << std::endl;
		if (neighs_in4.find(index) == neighs_in4.end())
			throw eRtLinkNotFound();
		neighs_in4.erase(index);
	}
	//std::cerr << "crtlink::set_neigh() route/neigh: NL-ACT-DEL => " << *this << std::endl;
	return index;
}



uint16_t
crtlink::del_neigh_in4(const crtneigh_in4& neigh)
{
	// find neighess
	std::map<uint16_t, crtneigh_in4>::iterator it;
	if ((it = find_if(neighs_in4.begin(), neighs_in4.end(),
			crtneigh_in4::crtneigh_in4_find_by_dst(neigh.get_dst()))) == neighs_in4.end()) {
		throw eRtLinkNotFound();
	}
	uint16_t index = it->first;
	//std::cerr << "crtlink::del_neigh() route/neigh: NL-ACT-DEL => index=" << it->first << std::endl;
	neighs_in4.erase(it);
	return index;
}







crtneigh_in6&
crtlink::get_neigh_in6(uint16_t index)
{
	if (neighs_in6.find(index) == neighs_in6.end())
		throw eRtLinkNotFound();
	return neighs_in6[index];
}



crtneigh_in6&
crtlink::get_neigh_in6(const rofl::caddress_in6& dst)
{
	std::map<uint16_t, crtneigh_in6>::iterator it;
	if ((it = find_if(neighs_in6.begin(), neighs_in6.end(),
			crtneigh_in6::crtneigh_in6_find_by_dst(dst))) == neighs_in6.end()) {
		throw eRtLinkNotFound();
	}
	return it->second;
}



uint16_t
crtlink::get_neigh_in6_index(const crtneigh_in6& neigh)
{
	std::map<uint16_t, crtneigh_in6>::iterator it;
	if ((it = find_if(neighs_in6.begin(), neighs_in6.end(),
			crtneigh_in6::crtneigh_in6_find_by_dst(neigh.get_dst()))) == neighs_in6.end()) {
		throw eRtLinkNotFound();
	}
	return it->first;
}



uint16_t
crtlink::get_neigh_in6_index(const rofl::caddress_in6& dst)
{
	std::map<uint16_t, crtneigh_in6>::iterator it;
	if ((it = find_if(neighs_in6.begin(), neighs_in6.end(),
			crtneigh_in6::crtneigh_in6_find_by_dst(dst))) == neighs_in6.end()) {
		throw eRtLinkNotFound();
	}
	return it->first;
}


uint16_t
crtlink::set_neigh_in6(const crtneigh_in6& neigh)
{
	// neighess already exists
	std::map<uint16_t, crtneigh_in6>::iterator it;
	if ((it = find_if(neighs_in6.begin(), neighs_in6.end(),
			crtneigh_in6::crtneigh_in6_find_by_dst(neigh.get_dst()))) != neighs_in6.end()) {

		//std::cerr << "crtlink::set_neigh() route/neigh: NL-ACT-CHG => index=" << it->first << " " << rta << std::endl;
		//std::cerr << "crtlink::set_neigh() route/neigh: NL-ACT-CHG => " << *this << std::endl;
		it->second = neigh;
		return it->first;

	} else {

		unsigned int index = 0;
		while (neighs_in6.find(index) != neighs_in6.end()) {
			index++;
		}
		//std::cerr << "crtlink::set_neigh() route/neigh: NL-ACT-NEW => index=" << index << " " << rta << std::endl;
		//std::cerr << "crtlink::set_neigh() route/neigh: NL-ACT-NEW => " << *this << std::endl;
		neighs_in6[index] = neigh;
		return index;
	}
}



uint16_t
crtlink::del_neigh_in6(uint16_t index)
{
	if (crtneigh::CRTNEIGH_ADDR_ALL == index) {
		//std::cerr << "crtlink::del_neigh() route/neigh: NL-ACT-DEL => index=" << index << std::endl;
		neighs_in6.clear();
	} else {
		//std::cerr << "crtlink::del_neigh() route/neigh: NL-ACT-DEL => index=" << index << std::endl;
		if (neighs_in6.find(index) == neighs_in6.end())
			throw eRtLinkNotFound();
		neighs_in6.erase(index);
	}
	//std::cerr << "crtlink::set_neigh() route/neigh: NL-ACT-DEL => " << *this << std::endl;
	return index;if (crtlink::rtlinks.find(ifindex) == crtlink::rtlinks.end()) {
}



uint16_t
crtlink::del_neigh_in6(const crtneigh_in6& neigh)
{
	// find neighess
	std::map<uint16_t, crtneigh_in6>::iterator it;
	if ((it = find_if(neighs_in6.begin(), neighs_in6.end(),
			crtneigh_in6::crtneigh_in6_find_by_dst(neigh.get_dst()))) == neighs_in6.end()) {
		throw eRtLinkNotFound();
	}
	uint16_t index = it->first;
	//std::cerr << "crtlink::del_neigh() route/neigh: NL-ACT-DEL => index=" << it->first << std::endl;
	neighs_in6.erase(it);
	return index;
}





