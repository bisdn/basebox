/*
 * crtneigh.cc
 *
 *  Created on: 03.07.2013
 *      Author: andreas
 */


#include <crtneigh.h>

using namespace dptmap;


crtneigh::crtneigh() :
		state(0),
		flags(0),
		ifindex(0),
		lladdr(rofl::cmacaddr("00:00:00:00:00:00")),
		dst(rofl::caddress(AF_INET)),
		family(0),
		type(0)
{

}



crtneigh::~crtneigh()
{

}



crtneigh::crtneigh(
		crtneigh const& neigh) :
		state(0),
		flags(0),
		ifindex(0),
		lladdr(rofl::cmacaddr("00:00:00:00:00:00")),
		dst(rofl::caddress(AF_INET)),
		family(0),
		type(0)
{
	*this = neigh;
}



crtneigh&
crtneigh::operator= (
		crtneigh const& neigh)
{
	if (this == &neigh)
		return *this;

	state		= neigh.state;
	flags		= neigh.flags;
	ifindex		= neigh.ifindex;
	lladdr		= neigh.lladdr;
	dst			= neigh.dst;
	family 		= neigh.family;
	type		= neigh.type;

	return *this;
}



crtneigh::crtneigh(
		struct rtnl_neigh* neigh) :
		state(0),
		flags(0),
		ifindex(0),
		lladdr(rofl::cmacaddr("00:00:00:00:00:00")),
		dst(rofl::caddress(AF_INET)),
		family(0),
		type(0)
{
	char s_buf[128];

	nl_object_get((struct nl_object*)neigh); // increment reference counter by one

	state	= rtnl_neigh_get_state(neigh);
	flags	= rtnl_neigh_get_flags(neigh);
	ifindex	= rtnl_neigh_get_ifindex(neigh);
	family	= rtnl_neigh_get_family(neigh);
	type	= rtnl_neigh_get_type(neigh);

	memset(s_buf, 0, sizeof(s_buf));
	lladdr 	= rofl::cmacaddr(nl_addr2str(rtnl_neigh_get_lladdr(neigh), s_buf, sizeof(s_buf)));

	memset(s_buf, 0, sizeof(s_buf));
	std::string s_dst(nl_addr2str(rtnl_neigh_get_dst(neigh), s_buf, sizeof(s_buf)));
	s_dst 	= s_dst.substr(0, s_dst.find_first_of("/", 0));
	dst 	= rofl::caddress(family, s_dst.c_str());

	nl_object_put((struct nl_object*)neigh); // decrement reference counter by one
}



