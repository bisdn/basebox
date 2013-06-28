/*
 * crtroute.cc
 *
 *  Created on: 28.06.2013
 *      Author: andreas
 */


#include <crtroute.h>

using namespace dptmap;


crtroute::crtroute() :
		table_id(0),
		scope(0),
		tos(0),
		protocol(0),
		priority(0),
		family(0),
		dst(rofl::caddress(AF_INET)),
		src(rofl::caddress(AF_INET)),
		type(0),
		flags(0),
		metric(0),
		pref_src(rofl::caddress(AF_INET)),
		ifindex(0)
{

}



crtroute::~crtroute()
{

}



crtroute::crtroute(crtroute const& rtr) :
		table_id(0),
		scope(0),
		tos(0),
		protocol(0),
		priority(0),
		family(0),
		dst(rofl::caddress(AF_INET)),
		src(rofl::caddress(AF_INET)),
		type(0),
		flags(0),
		metric(0),
		pref_src(rofl::caddress(AF_INET)),
		ifindex(0){
	*this = rtr;
}



crtroute&
crtroute::operator= (crtroute const& rtr)
{
	if (this == &rtr)
		return *this;

	table_id 	= rtr.table_id;
	scope		= rtr.scope;
	tos			= rtr.tos;
	protocol	= rtr.protocol;
	priority	= rtr.priority;
	family		= rtr.family;
	dst			= rtr.dst;
	src			= rtr.src;
	type		= rtr.type;
	flags		= rtr.flags;
	metric		= rtr.metric;
	pref_src	= rtr.pref_src;
	ifindex		= rtr.ifindex;
	// TODO: next hops

	return *this;
}



crtroute::crtroute(struct rtnl_route *route) :
		table_id(0),
		scope(0),
		tos(0),
		protocol(0),
		priority(0),
		family(0),
		dst(rofl::caddress(AF_INET)),
		src(rofl::caddress(AF_INET)),
		type(0),
		flags(0),
		metric(0),
		pref_src(rofl::caddress(AF_INET)),
		ifindex(0)
{
	char s_buf[128];
	memset(s_buf, 0, sizeof(s_buf));

	rtnl_route_get(route); // increment reference count by one

	table_id 	= rtnl_route_get_table(route);
	scope		= rtnl_route_get_scope(route);
	tos			= rtnl_route_get_tos(route);
	protocol	= rtnl_route_get_protocol(route);
	priority	= rtnl_route_get_priority(route);
	family		= rtnl_route_get_family(route);
	type 		= rtnl_route_get_type(route);
	flags		= rtnl_route_get_flags(route);
	metric		= rtnl_route_get_metric(route, 0, NULL); // FIXME: check the integer value
	ifindex		= rtnl_route_get_iif(route);

	std::string s_dst(nl_addr2str(rtnl_route_get_dst(route), 			s_buf, sizeof(s_buf)));
	std::string s_src(nl_addr2str(rtnl_route_get_src(route), 			s_buf, sizeof(s_buf)));
	std::string s_pref_src(nl_addr2str(rtnl_route_get_pref_src(route), 	s_buf, sizeof(s_buf)));

	s_dst 		= s_dst.substr(0, s_dst.find_first_of("/", 0));
	s_src  		= s_src.substr(0,  s_src.find_first_of("/", 0));
	s_pref_src 	= s_pref_src.substr(0, s_pref_src.find_first_of("/", 0));

	dst 		= rofl::caddress(family, s_dst.c_str());
	src 		= rofl::caddress(family, s_src.c_str());
	pref_src	= rofl::caddress(family, s_pref_src.c_str());

	rtnl_route_put(route); // decrement reference count by one
}



bool
crtroute::operator== (crtroute const& rtr)
{
	// FIXME: anything else beyond this?
	return ((table_id 		== rtr.table_id) &&
			(scope 			== rtr.scope) &&
			(ifindex		== rtr.ifindex) &&
			(dst			== rtr.dst));
}


