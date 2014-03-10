#include <crtnexthop.h>

using namespace dptmap;


crtnexthop::crtnexthop() :
		family(0),
		weight(0),
		ifindex(0),
		gateway(rofl::caddress(AF_INET, "0.0.0.0")),
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
		gateway(rofl::caddress(AF_INET, "0.0.0.0")),
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
	gateway			= nxthop.gateway;
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
		gateway(rofl::caddress(AF_INET, "0.0.0.0")),
		flags(0),
		realms(0)
{
	char s_buf[128];
	memset(s_buf, 0, sizeof(s_buf));

	rtnl_route_get(route);

	family		= rtnl_route_get_family(route);
	weight		= rtnl_route_nh_get_weight(nxthop);
	ifindex		= rtnl_route_nh_get_ifindex(nxthop);
	flags		= rtnl_route_nh_get_flags(nxthop);
	realms		= rtnl_route_nh_get_realms(nxthop);

	std::string s_gw(nl_addr2str(rtnl_route_nh_get_gateway(nxthop), s_buf, sizeof(s_buf)));
	s_gw 		= s_gw.substr(0, s_gw.find_first_of("/", 0));
	gateway 	= rofl::caddress(family, s_gw.c_str());

	rtnl_route_put(route);
}



