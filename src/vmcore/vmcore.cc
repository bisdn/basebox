#include "vmcore.h"


vmcore::vmcore()
{

}


vmcore::~vmcore()
{

}


void
vmcore::nl_route_new(cnetlink const* netlink, croute const& route)
{
	croute rt(route);
	fprintf(stderr, "vmcore::nl_route_new() %s\n", rt.c_str());
}


void
vmcore::nl_route_del(cnetlink const* netlink, croute const& route)
{
	croute rt(route);
	fprintf(stderr, "vmcore::nl_route_del() %s\n", rt.c_str());
}


void
vmcore::nl_route_change(cnetlink const* netlink, croute const& route)
{
	croute rt(route);
	fprintf(stderr, "vmcore::nl_route_change() %s\n", rt.c_str());
}


