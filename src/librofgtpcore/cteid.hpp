/*
 * cteid.hpp
 *
 *  Created on: 19.08.2014
 *      Author: andreas
 */

#ifndef CTEID_HPP_
#define CTEID_HPP_

#include <ostream>
#include <inttypes.h>

#include "clogging.h"

namespace rofgtp {

class cteid {
public:

	/**
	 *
	 */
	cteid() :
		teid(0) {};

	/**
	 *
	 */
	explicit
	cteid(uint32_t teid) :
		teid(teid) {};

	/**
	 *
	 */
	~cteid() {};

	/**
	 *
	 */
	cteid(const cteid& teid) { *this = teid; };

	/**
	 *
	 */
	cteid&
	operator= (const cteid& teid) {
		if (this == &teid)
			return *this;
		this->teid = teid.teid;
		return *this;
	};

	/**
	 *
	 */
	bool
	operator< (const cteid& teid) const {
		return (this->teid < teid.teid);
	};

public:

	/**
	 *
	 */
	uint32_t
	get_value() const { return teid; };

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cteid& teid) {
		os << "<cteid " << (unsigned int)teid.get_value() << " >";
		return os;
	};

private:

	uint32_t teid;
};

}; // end of namespace rofgtp

#endif /* CTEID_HPP_ */
