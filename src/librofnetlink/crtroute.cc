/*
 * crtroute.cc
 *
 *  Created on: 28.06.2013
 *      Author: andreas
 */


#include "crtroute.h"

using namespace rofcore;

crtroute::crtroute() :
		table_id(0),
		scope(0),
		tos(0),
		protocol(0),
		priority(0),
		family(0),
		prefixlen(0),
		type(0),
		flags(0),
		metric(0),
		iif(0)
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
		prefixlen(0),
		type(0),
		flags(0),
		metric(0),
		iif(0){
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
	prefixlen	= rtr.prefixlen;
	type		= rtr.type;
	flags		= rtr.flags;
	metric		= rtr.metric;
	iif			= rtr.iif;

	return *this;
}



crtroute::crtroute(struct rtnl_route *route) :
		table_id(0),
		scope(0),
		tos(0),
		protocol(0),
		priority(0),
		family(0),
		prefixlen(0),
		type(0),
		flags(0),
		metric(0),
		iif(0)
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
	iif		= rtnl_route_get_iif(route);
	prefixlen	= nl_addr_get_prefixlen(rtnl_route_get_dst(route));

	rtnl_route_put(route); // decrement reference count by one
}



bool
crtroute::operator== (crtroute const& rtr)
{
	// FIXME: anything else beyond this?
	return ((table_id 		== rtr.table_id) &&
			(scope 			== rtr.scope) &&
			(iif		== rtr.iif));
}



std::string
crtroute::get_table_id_s() const
{
	std::string str;

	switch (table_id) {
	/*255*/case RT_TABLE_LOCAL:		str = std::string("local");		break;
	/*254*/case RT_TABLE_MAIN:		str = std::string("main");		break;
	/*253*/case RT_TABLE_DEFAULT:	str = std::string("default");	break;
	/*252*/case RT_TABLE_COMPAT:	str = std::string("compat");	break;
	default:						str = std::string("unknown");	break;
	}

	return str;
}



std::string
crtroute::get_scope_s() const
{
	std::string str;

	switch (scope) {
	/*255*/case RT_SCOPE_NOWHERE:	str = std::string("nowhere");	break;
	/*254*/case RT_SCOPE_HOST:		str = std::string("host");		break;
	/*253*/case RT_SCOPE_LINK:		str = std::string("link");		break;
	/*200*/case RT_SCOPE_SITE:		str = std::string("site");		break;
	/*000*/case RT_SCOPE_UNIVERSE:	str = std::string("universe");	break;
	default:						str = std::string("unknown");	break;
	}

	return str;
}



crtnexthop_in4&
crtroute_in4::add_nexthop_in4(
		unsigned int nhindex)
{
	if (nexthops.find(nhindex) != nexthops.end()) {
		nexthops.erase(nhindex);
	}
	return nexthops[nhindex];
}



crtnexthop_in4&
crtroute_in4::set_nexthop_in4(
		unsigned int nhindex)
{
	if (nexthops.find(nhindex) == nexthops.end()) {
		(void)nexthops[nhindex];
	}
	return nexthops[nhindex];
}



const crtnexthop_in4&
crtroute_in4::get_nexthop_in4(
		unsigned int nhindex) const
{
	if (nexthops.find(nhindex) == nexthops.end()) {
		throw eRtRouteNotFound();
	}
	return nexthops.at(nhindex);
}



void
crtroute_in4::drop_nexthop_in4(
		unsigned int nhindex)
{
	if (nexthops.find(nhindex) == nexthops.end()) {
		return;
	}
	nexthops.erase(nhindex);
}



bool
crtroute_in4::has_nexthop_in4(
		unsigned int nhindex) const
{
	return (not (nexthops.find(nhindex) == nexthops.end()));
}



crtnexthop_in6&
crtroute_in6::add_nexthop_in6(
		unsigned int nhindex)
{
	if (nexthops.find(nhindex) != nexthops.end()) {
		nexthops.erase(nhindex);
	}
	return nexthops[nhindex];
}



crtnexthop_in6&
crtroute_in6::set_nexthop_in6(
		unsigned int nhindex)
{
	if (nexthops.find(nhindex) == nexthops.end()) {
		(void)nexthops[nhindex];
	}
	return nexthops[nhindex];
}



const crtnexthop_in6&
crtroute_in6::get_nexthop_in6(
		unsigned int nhindex) const
{
	if (nexthops.find(nhindex) == nexthops.end()) {
		throw eRtRouteNotFound();
	}
	return nexthops.at(nhindex);
}



void
crtroute_in6::drop_nexthop_in6(
		unsigned int nhindex)
{
	if (nexthops.find(nhindex) == nexthops.end()) {
		return;
	}
	nexthops.erase(nhindex);
}



bool
crtroute_in6::has_nexthop_in6(
		unsigned int nhindex) const
{
	return (not (nexthops.find(nhindex) == nexthops.end()));
}



