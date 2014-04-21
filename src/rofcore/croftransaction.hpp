/*
 * croftransaction.hpp
 *
 *  Created on: 21.04.2014
 *      Author: andreas
 */

#ifndef CROFTRANSACTION_HPP_
#define CRoFTRANSACTION_HPP_

#include <inttypes.h>
#include <iostream>

#include "croflog.hpp"
#include "cctlxid.hpp"
#include "cdptxid.hpp"
#include "erofcorexcp.hpp"

namespace rofcore {

class eRofTransactionBase		: public eRofCoreXcp {};
class eRofTransactionNotFound	: public eRofTransactionBase {};

class croftransaction {

	uint8_t		msg_type;
	cctlxid		ctlxid;
	cdptxid		dptxid;

public:

	/**
	 *
	 */
	croftransaction();

	/**
	 *
	 */
	croftransaction(
			uint8_t msg_type, cctlxid const& ctlxid, cdptxid const& dptxid);

	/**
	 *
	 */
	virtual
	~croftransaction();

	/**
	 *
	 */
	croftransaction(
			croftransaction const& ta);

	/**
	 *
	 */
	croftransaction&
	operator= (
			croftransaction const& ta);

	/**
	 *
	 */
	bool
	operator< (
			croftransaction const& ta) const;

public:

	/**
	 *
	 */
	uint8_t
	get_msg_type() const { return msg_type; };

	/**
	 *
	 */
	void
	set_msg_type(uint8_t msg_type) { this->msg_type = msg_type; };

	/**
	 *
	 */
	cctlxid const&
	get_ctlxid() const { return ctlxid; };

	/**
	 *
	 */
	cctlxid&
	set_ctlxid() { return ctlxid; };

	/**
	 *
	 */
	cdptxid const&
	get_dptxid() const { return dptxid; };

	/**
	 *
	 */
	cdptxid&
	set_dptxid() { return dptxid; };

public:

	friend std::ostream&
	operator<< (std::ostream& os, croftransaction const& ta) {
		os << rofcore::indent(0) << "<croftransaction "
				<< "msg-type:" << (int)ta.get_msg_type() << " "
				<< "ctlxid:" << ta.get_ctlxid() << " "
				<< "dptxid:" << ta.get_dptxid() << " "
				<< ">" << std::endl;
		return os;
	};

public:

	class find_by_cctlxid {
		cctlxid ctlxid;
	public:
		find_by_cctlxid(cctlxid const& ctlxid) :
			ctlxid(ctlxid) {};
		bool operator() (croftransaction const& ta) {
			return (ta.ctlxid == ctlxid);
		};
	};

	class find_by_cdptxid {
		cdptxid dptxid;
	public:
		find_by_cdptxid(cdptxid const& dptxid) :
			dptxid(dptxid) {};
		bool operator() (croftransaction const& ta) {
			return (ta.dptxid == dptxid);
		};
	};
};

}; // end of namespace rofcore

#endif /* CROFTRANSACTION_HPP_ */
