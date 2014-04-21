/*
 * croftransaction.cpp
 *
 *  Created on: 21.04.2014
 *      Author: andreas
 */

#include "croftransaction.hpp"

using namespace rofcore;

croftransaction::croftransaction() :
		msg_type(0),
		ctlxid(),
		dptxid()
{

}



croftransaction::croftransaction(
		uint8_t msg_type, cctlxid const& ctlxid, cdptxid const& dptxid) :
				msg_type(msg_type),
				ctlxid(ctlxid),
				dptxid(dptxid)
{

}



croftransaction::~croftransaction()
{

}



croftransaction::croftransaction(
		croftransaction const& ta)
{
	*this = ta;
}



croftransaction&
croftransaction::operator= (
		croftransaction const& ta)
{
	if (this == &ta)
		return *this;

	msg_type	= ta.msg_type;
	ctlxid		= ta.ctlxid;
	dptxid		= ta.dptxid;

	return *this;
}



bool
croftransaction::operator< (
		croftransaction const& ta) const
{
	return ((msg_type < ta.msg_type) && (ctlxid < ta.ctlxid) && (dptxid < ta.dptxid));
}



