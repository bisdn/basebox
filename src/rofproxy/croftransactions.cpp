/*
 * croftransactions.cpp
 *
 *  Created on: 21.04.2014
 *      Author: andreas
 */

#include "croftransactions.hpp"

using namespace rofcore;

croftransactions::croftransactions()
{

}



croftransactions::~croftransactions()
{

}



croftransactions::croftransactions(
		croftransactions const& tas)
{
	*this = tas;
}



croftransactions&
croftransactions::operator= (
		croftransactions const& tas)
{
	if (this == &tas)
		return *this;

	// TODO

	return *this;
}



croftransaction&
croftransactions::add_ta(
		cctlxid const& ctlxid)
{
	std::set<croftransaction>::iterator it;
	if ((it = find_if(transactions.begin(), transactions.end(),
			croftransaction::find_by_cctlxid(ctlxid))) != transactions.end()) {
		transactions.erase(it);
	}
	return const_cast<croftransaction&>(*(transactions.insert(croftransaction()).first));
}



croftransaction&
croftransactions::set_ta(
		cctlxid const& ctlxid)
{
	std::set<croftransaction>::iterator it;
	if ((it = find_if(transactions.begin(), transactions.end(),
			croftransaction::find_by_cctlxid(ctlxid))) == transactions.end()) {
		it = transactions.insert(croftransaction()).first;
	}
	return const_cast<croftransaction&>(*(it));
}



croftransaction const&
croftransactions::get_ta(
		cctlxid const& ctlxid) const
{
	std::set<croftransaction>::const_iterator it;
	if ((it = find_if(transactions.begin(), transactions.end(),
			croftransaction::find_by_cctlxid(ctlxid))) == transactions.end()) {
		throw eRofTransactionNotFound();
	}
	return *(it);
}



void
croftransactions::drop_ta(
		cctlxid const& ctlxid)
{
	std::set<croftransaction>::iterator it;
	if ((it = find_if(transactions.begin(), transactions.end(),
			croftransaction::find_by_cctlxid(ctlxid))) == transactions.end()) {
		return;
	}
	transactions.erase(it);
}



bool
croftransactions::has_ta(
		cctlxid const& ctlxid) const
{
	return (not (find_if(transactions.begin(), transactions.end(),
				croftransaction::find_by_cctlxid(ctlxid)) == transactions.end()));
}



croftransaction&
croftransactions::add_ta(
		cdptxid const& dptxid)
{
	std::set<croftransaction>::iterator it;
	if ((it = find_if(transactions.begin(), transactions.end(),
			croftransaction::find_by_cdptxid(dptxid))) != transactions.end()) {
		transactions.erase(it);
	}
	return const_cast<croftransaction&>(*(transactions.insert(croftransaction()).first));
}



croftransaction&
croftransactions::set_ta(
		cdptxid const& dptxid)
{
	std::set<croftransaction>::iterator it;
	if ((it = find_if(transactions.begin(), transactions.end(),
			croftransaction::find_by_cdptxid(dptxid))) == transactions.end()) {
		it = transactions.insert(croftransaction()).first;
	}
	return const_cast<croftransaction&>(*(it));
}



croftransaction const&
croftransactions::get_ta(
		cdptxid const& dptxid) const
{
	std::set<croftransaction>::const_iterator it;
	if ((it = find_if(transactions.begin(), transactions.end(),
			croftransaction::find_by_cdptxid(dptxid))) == transactions.end()) {
		throw eRofTransactionNotFound();
	}
	return *(it);
}



void
croftransactions::drop_ta(
		cdptxid const& dptxid)
{
	std::set<croftransaction>::iterator it;
	if ((it = find_if(transactions.begin(), transactions.end(),
			croftransaction::find_by_cdptxid(dptxid))) == transactions.end()) {
		return;
	}
	transactions.erase(it);
}



bool
croftransactions::has_ta(
		cdptxid const& dptxid) const
{
	return (not (find_if(transactions.begin(), transactions.end(),
				croftransaction::find_by_cdptxid(dptxid)) == transactions.end()));
}


