/*
 * cdptid.cpp
 *
 *  Created on: 19.04.2014
 *      Author: andreas
 */

#include "cdptid.hpp"

using namespace rofcore;

cdptid::cdptid(
		uint64_t dpid) :
				dpid(dpid)
{

}


cdptid::~cdptid()
{

}


cdptid::cdptid(
		cdptid const& dptid)
{
	*this = dptid;
}


cdptid&
cdptid::operator= (
		cdptid const& dptid)
{
	if (this == &dptid)
		return *this;

	dpid = dptid.dpid;

	return *this;
}


