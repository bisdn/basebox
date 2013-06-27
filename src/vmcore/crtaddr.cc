/*
 * crtaddr.cc
 *
 *  Created on: 27.06.2013
 *      Author: andreas
 */


#include <crtaddr.h>


using namespace dptmap;


crtaddr::crtaddr()
{

}



crtaddr::~crtaddr()
{

}



crtaddr::crtaddr(crtaddr const& rtaddr)
{
	*this = rtaddr;
}



crtaddr&
crtaddr::operator= (crtaddr const& rtaddr)
{
	if (this == &rtaddr)
		return *this;


	return *this;
}



