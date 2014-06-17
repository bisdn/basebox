#include "crtnexthop.h"

using namespace rofcore;


crtnexthop::crtnexthop() :
		family(0),
		weight(0),
		ifindex(0),
		flags(0),
		realms(0)
{

}



crtnexthop::~crtnexthop()
{

}



crtnexthop::crtnexthop(
		crtnexthop const& nxthop) :
		family(0),
		weight(0),
		ifindex(0),
		flags(0),
		realms(0)
{
	*this = nxthop;
}



crtnexthop&
crtnexthop::operator= (crtnexthop const& nxthop)
{
	if (this == &nxthop)
		return *this;

	family			= nxthop.family;
	weight 			= nxthop.weight;
	ifindex			= nxthop.ifindex;
	flags			= nxthop.flags;
	realms			= nxthop.realms;

	return *this;
}



crtnexthop::crtnexthop(
		struct rtnl_route *route,
		struct rtnl_nexthop *nxthop) :
		family(0),
		weight(0),
		ifindex(0),
		flags(0),
		realms(0)
{
	rtnl_route_get(route);

	family		= rtnl_route_get_family(route);
	weight		= rtnl_route_nh_get_weight(nxthop);
	ifindex		= rtnl_route_nh_get_ifindex(nxthop);
	flags		= rtnl_route_nh_get_flags(nxthop);
	realms		= rtnl_route_nh_get_realms(nxthop);

	rtnl_route_put(route);
}



