/*
 * crtroute.cc
 *
 *  Created on: 28.06.2013
 *      Author: andreas
 */


#include <crtroute.h>

using namespace dptmap;


crtroute::crtroute()
{

}



crtroute::~crtroute()
{

}



crtroute::crtroute(crtroute const& rtr)
{
	*this = rtr;
}



crtroute&
crtroute::operator= (crtroute const& rtr)
{
	if (this == &rtr)
		return *this;

	return *this;
}



crtroute::crtroute(struct rtnl_route *route)
{

}
