/*
 * cctlid.cpp
 *
 *  Created on: 19.04.2014
 *      Author: andreas
 */

#include "cctlid.hpp"

using namespace rofcore;

cctlid::cctlid(
		uint64_t ctlid) :
				ctlid(ctlid)
{

}


cctlid::~cctlid()
{

}


cctlid::cctlid(
		cctlid const& dptid)
{
	*this = dptid;
}


cctlid&
cctlid::operator= (
		cctlid const& dptid)
{
	if (this == &dptid)
		return *this;

	ctlid = dptid.ctlid;

	return *this;
}


