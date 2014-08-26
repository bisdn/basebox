/*
 * cprefix.hpp
 *
 *  Created on: 26.08.2014
 *      Author: andreas
 */

#ifndef CPREFIX_HPP_
#define CPREFIX_HPP_

#include <iostream>
#include <rofl/common/caddress.h>
#include "clogging.h"

namespace rofcore {

class cprefix {
public:

	/**
	 *
	 */
	cprefix() {};

	/**
	 *
	 */
	virtual
	~cprefix() {};

	/**
	 *
	 */
	cprefix(const cprefix& prefix) { *this = prefix; };

	/**
	 *
	 */
	cprefix&
	operator= (const cprefix& prefix) {
		if (this == &prefix)
			return *this;
		// do nothing (so far)
		return *this;
	};

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cprefix& prefix) {
		os << rofcore::indent(0) << "<cprefix >" << std::endl;
		return os;
	};

protected:


};



class cprefix_in4 : public cprefix {
public:

	/**
	 *
	 */
	cprefix_in4() {};

	/**
	 *
	 */
	virtual
	~cprefix_in4() {};

	/**
	 *
	 */
	cprefix_in4(const rofl::caddress_in4& addr, const rofl::caddress_in4& mask) :
		addr(addr), mask(mask) {};

	/**
	 *
	 */
	cprefix_in4(const cprefix_in4& prefix) { *this = prefix; };

	/**
	 *
	 */
	cprefix_in4&
	operator= (const cprefix_in4& prefix) {
		if (this == &prefix)
			return *this;
		addr = prefix.addr;
		mask = prefix.mask;
		return *this;
	};

	/**
	 *
	 */
	bool
	operator< (const cprefix_in4& prefix) const {
		return ((prefix.mask < mask) || (addr < prefix.addr));
	};

public:

	/**
	 *
	 */
	const rofl::caddress_in4&
	get_addr() const { return addr; };

	/**
	 *
	 */
	const rofl::caddress_in4&
	get_mask() const { return mask; };

public:

	/**
	 *
	 */
	friend std::ostream&
	operator<< (std::ostream& os, const cprefix_in4& prefix) {
		os << rofcore::indent(0) << "<cprefix_in4 >" << std::endl;
		os << rofcore::indent(2) << "<addr: >" << std::endl;
		{ rofcore::indent i(2); os << prefix.addr; };
		os << rofcore::indent(2) << "<mask: >" << std::endl;
		{ rofcore::indent i(2); os << prefix.mask; };
		return os;
	};

private:

	rofl::caddress_in4 addr;
	rofl::caddress_in4 mask;
};



class cprefix_in6 : public cprefix {
public:

	/**
	 *
	 */
	cprefix_in6() {};

	/**
	 *
	 */
	virtual
	~cprefix_in6() {};

	/**
	 *
	 */
	cprefix_in6(const rofl::caddress_in6& addr, const rofl::caddress_in6& mask) :
		addr(addr), mask(mask) {};

	/**
	 *
	 */
	cprefix_in6(const cprefix_in6& prefix) { *this = prefix; };

	/**
	 *
	 */
	cprefix_in6&
	operator= (const cprefix_in6& prefix) {
		if (this == &prefix)
			return *this;
		addr = prefix.addr;
		mask = prefix.mask;
		return *this;
	};

	/**
	 *
	 */
	bool
	operator< (const cprefix_in6& prefix) const {
		return ((prefix.mask < mask) || (addr < prefix.addr));
	};

public:

	/**
	 *
	 */
	const rofl::caddress_in6&
	get_addr() const { return addr; };

	/**
	 *
	 */
	const rofl::caddress_in6&
	get_mask() const { return mask; };

public:

	/**
	 *
	 */
	friend std::ostream&
	operator<< (std::ostream& os, const cprefix_in6& prefix) {
		os << rofcore::indent(0) << "<cprefix_in6 >" << std::endl;
		os << rofcore::indent(2) << "<addr: >" << std::endl;
		{ rofcore::indent i(2); os << prefix.addr; };
		os << rofcore::indent(2) << "<mask: >" << std::endl;
		{ rofcore::indent i(2); os << prefix.mask; };
		return os;
	};

private:

	rofl::caddress_in6 addr;
	rofl::caddress_in6 mask;
};

}; // end of namespace rofcore

#endif /* CPREFIX_HPP_ */
