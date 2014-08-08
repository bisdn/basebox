/*
 * cdpid.hpp
 *
 *  Created on: 05.08.2014
 *      Author: andreas
 */

#ifndef CDPID_HPP_
#define CDPID_HPP_

#include <inttypes.h>
#include <iostream>

#include "clogging.h"

namespace ethcore {

class cdpid {
public:

	/**
	 *
	 */
	cdpid() :
		dpid(0) {};

	/**
	 *
	 */
	explicit cdpid(uint64_t dpid) :
			dpid(dpid) {};

	/**
	 *
	 */
	cdpid(const cdpid& dpid) { *this = dpid; };

	/**
	 *
	 */
	cdpid&
	operator= (const cdpid& dpid) {
		if (this == &dpid)
			return *this;
		this->dpid = dpid.dpid;
		return *this;
	};

	/**
	 *
	 */
	bool
	operator< (const cdpid& dpid) const {
		return (this->dpid < dpid.dpid);
	};

public:

	/**
	 *
	 */
	uint64_t
	get_dpid() const { return dpid; };

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cdpid& dpid) {
		os << rofcore::indent(0) << "<cdpid "
				<< (unsigned long long)dpid.get_dpid() << " >" << std::endl;
		return os;
	};

private:

	uint64_t 	dpid;
};

};

#endif /* CDPID_HPP_ */
