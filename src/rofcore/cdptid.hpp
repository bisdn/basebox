/*
 * cdptid.hpp
 *
 *  Created on: 19.04.2014
 *      Author: andreas
 */

#ifndef CDPTID_HPP_
#define CDPTID_HPP_

#include <inttypes.h>

#include <iostream>

#include "croflog.hpp"

namespace rofcore {

class cdptid {

	uint64_t dpid;

public:

	/**
	 *
	 */
	cdptid(
			uint64_t dpid = 0);

	/**
	 *
	 */
	virtual
	~cdptid();

	/**
	 *
	 */
	cdptid(
			cdptid const& dptid);

	/**
	 *
	 */
	cdptid&
	operator= (
			cdptid const& dptid);

	/**
	 *
	 */
	bool
	operator== (
			cdptid const& dptid) const {
		return (dpid == dptid.dpid);
	};

	/**
	 *
	 */
	bool
	operator< (
			cdptid const& dptid) const {
		return (dpid < dptid.dpid);
	};

public:

	/**
	 *
	 */
	uint64_t const&
	get_dpid() const { return dpid; };

public:

	friend std::ostream&
	operator<< (std::ostream& os, cdptid const& dptid) {
		os << rofcore::indent(0) << "<cdptid:" << (unsigned long long)dptid.dpid << " >" << std::endl;
		return os;
	};
};

}; // end of namespace

#endif /* CDPTID_HPP_ */
