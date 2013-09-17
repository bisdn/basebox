/*
 * cprefix.cc
 *
 *  Created on: 17.09.2013
 *      Author: andreas
 */

#include "cprefix.h"

using namespace dhcpv6snoop;


cprefix::cprefix() :
		preflen(0)
{

}



cprefix::~cprefix()
{

}



cprefix::cprefix(
			cprefix const& rt)
{
	*this = rt;
}



cprefix&
cprefix::operator= (
			cprefix const& rt)
{
	if (this == &rt)
		return *this;

	devname		= rt.devname;
	serverid	= rt.serverid;
	pref		= rt.pref;
	preflen		= rt.preflen;
	via			= rt.via;

	return *this;
}



cprefix::cprefix(
			std::string const& devname,
			std::string const& serverid,
			rofl::caddress const& pref,
			unsigned int preflen,
			rofl::caddress const& via) :
		devname(devname),
		serverid(serverid),
		pref(pref),
		preflen(preflen),
		via(via)
{

}



bool
cprefix::operator< (cprefix const& pr)
{
	return ((preflen < pr.preflen) && (pref < pr.pref));
}



void
cprefix::route_add()
{
	// TODO: set route into kernel
	std::cerr << "cprefix::route_add() " << *this << std::endl;
}



void
cprefix::route_del()
{
	// TODO: remove route from kernel
	std::cerr << "cprefix::route_del() " << *this << std::endl;
}






