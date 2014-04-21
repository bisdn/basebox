/*
 * cctlxid.hpp
 *
 *  Created on: 21.04.2014
 *      Author: andreas
 */

#ifndef CCTLXID_HPP_
#define CCTLXID_HPP_

#include <inttypes.h>
#include <iostream>

#include "croflog.hpp"

namespace rofcore {

class cctlxid {

	uint32_t xid;

public:

	/**
	 *
	 */
	cctlxid(
			uint32_t xid = 0) :
				xid(xid) {};

	/**
	 *
	 */
	virtual
	~cctlxid() {};

	/**
	 *
	 */
	cctlxid(
			cctlxid const& ctlxid) {
		*this = ctlxid;
	};

	/**
	 *
	 */
	cctlxid&
	operator= (
			cctlxid const& ctlxid) {
		if (this == &ctlxid)
			return *this;
		xid = ctlxid.xid;
		return *this;
	};

	/**
	 *
	 */
	bool
	operator< (
			cctlxid const& ctlxid) const {
		return (xid < ctlxid.xid);
	};

	/**
	 *
	 */
	bool
	operator== (
			cctlxid const& ctlxid) const {
		return (xid == ctlxid.xid);
	};

public:

	/**
	 *
	 */
	uint32_t
	get_xid() const { return xid; };

	/**
	 *
	 */
	void
	set_xid(uint32_t xid) { this->xid = xid; };

public:

	friend std::ostream&
	operator<< (std::ostream& os, cctlxid const& ctlxid) {
		os << rofcore::indent(0) << "<cctlxid xid:" << (int)ctlxid.get_xid() << " >" << std::endl;
		return os;
	};
};

}; // end of namespace

#endif /* CCTLXID_HPP_ */
