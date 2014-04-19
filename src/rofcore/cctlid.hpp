/*
 * cctlid.hpp
 *
 *  Created on: 19.04.2014
 *      Author: andreas
 */

#ifndef CCTLID_HPP_
#define CCTLID_HPP_

#include <inttypes.h>

#include <iostream>

#include "croflog.hpp"

namespace rofcore {

class cctlid {

	uint64_t ctlid;

public:

	/**
	 *
	 */
	cctlid(
			uint64_t ctlid = 0);

	/**
	 *
	 */
	virtual
	~cctlid();

	/**
	 *
	 */
	cctlid(
			cctlid const& dptid);

	/**
	 *
	 */
	cctlid&
	operator= (
			cctlid const& dptid);

	/**
	 *
	 */
	bool
	operator== (
			cctlid const& c) const {
		return (ctlid == c.ctlid);
	};

	/**
	 *
	 */
	bool
	operator< (
			cctlid const& c) const {
		return (ctlid < c.ctlid);
	};

public:

	/**
	 *
	 */
	uint64_t const&
	get_ctlid() const { return ctlid; };

public:

	friend std::ostream&
	operator<< (std::ostream& os, cctlid const& dptid) {
		os << rofcore::indent(0) << "<cctlid:" << (unsigned long long)dptid.ctlid << " >" << std::endl;
		return os;
	};
};

}; // end of namespace

#endif /* CDPTID_HPP_ */
