/*
 * cdptxid.hpp
 *
 *  Created on: 21.04.2014
 *      Author: andreas
 */

#ifndef CDPTXID_HPP_
#define CDPTXID_HPP_

#include <inttypes.h>
#include <iostream>

#include "croflog.hpp"

namespace rofcore {

class cdptxid {

	uint32_t xid;

public:

	/**
	 *
	 */
	cdptxid(
			uint32_t xid = 0) :
				xid(xid) {};

	/**
	 *
	 */
	virtual
	~cdptxid() {};

	/**
	 *
	 */
	cdptxid(
			cdptxid const& dptxid) {
		*this = dptxid;
	};

	/**
	 *
	 */
	cdptxid&
	operator= (
			cdptxid const& dptxid) {
		if (this == &dptxid)
			return *this;
		xid = dptxid.xid;
		return *this;
	};

	/**
	 *
	 */
	bool
	operator< (
			cdptxid const& dptxid) const {
		return (xid < dptxid.xid);
	};

	/**
	 *
	 */
	bool
	operator== (
			cdptxid const& dptxid) const {
		return (xid == dptxid.xid);
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
	operator<< (std::ostream& os, cdptxid const& dptxid) {
		os << rofcore::indent(0) << "<cdptxid xid:" << (int)dptxid.get_xid() << " >" << std::endl;
		return os;
	};
};

}; // end of namespace

#endif /* CDPTXID_HPP_ */
