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
#include <roflibs/netlink/clogging.hpp>

namespace rofcore {

class cprefix {
public:

	/**
	 *
	 */
	cprefix(int prefixlen = 0) : prefixlen(prefixlen) {};

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
		prefixlen = prefix.prefixlen;
		return *this;
	};

public:

	/**
	 *
	 */
	int
	get_prefixlen() const { return prefixlen; };

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cprefix& prefix) {
		os << rofcore::indent(0) << "<cprefix >" << std::endl;
		return os;
	};

protected:

	int prefixlen;
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
		cprefix(0), addr(addr), mask(mask) {
		uint32_t m = mask.get_addr_hbo();
		while (0 == (m % 2)) {
			prefixlen++;
			m = m >> 1;
		}
		prefixlen = 32-prefixlen;
	};

	/**
	 *
	 */
	cprefix_in4(const rofl::caddress_in4& addr, int prefixlen) :
		cprefix(prefixlen), addr(addr) {
		mask.set_addr_hbo(~((1 << (32 - prefixlen)) - 1));
	};

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
		cprefix::operator= (prefix);
		addr = prefix.addr;
		mask = prefix.mask;
		return *this;
	};

	/**
	 *
	 */
	bool
	operator< (const cprefix_in4& prefix) const {
		return ((prefix.prefixlen < prefixlen) || (addr < prefix.addr));
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
		os << rofcore::indent(0) << "<cprefix_in4 prefixlen: " << prefix.get_prefixlen() << " >" << std::endl;
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
		cprefix(0), addr(addr), mask(mask) {
		// TODO
		assert(0);
#if 0
		for (int i = 0; i < 16; i++) {
			uint8_t byte = ((uint8_t*)&mask)[15-i];
			while (0 == (byte % 2)) {
				prefixlen++;
				byte = byte >> 1;
			}
		}
#endif
	};

	/**
	 *
	 */
	cprefix_in6(const rofl::caddress_in6& addr, int prefixlen) :
		cprefix(prefixlen), addr(addr) {
		int segment 	= prefixlen / 8;
		int t_prefixlen = prefixlen % 8;
		for (int i = 0; i < 16; i++) {
			if (segment == i) {
				mask[i] = ~((1 << (8 - t_prefixlen)) - 1);
			} else if (i > segment) {
				mask[i] = 0;
			} else if (i < segment) {
				mask[i] = 0xff;
			}
		}
	};

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
		cprefix::operator= (prefix);
		addr = prefix.addr;
		mask = prefix.mask;
		return *this;
	};

	/**
	 *
	 */
	bool
	operator< (const cprefix_in6& prefix) const {
		return ((prefix.prefixlen < prefixlen) || (addr < prefix.addr));
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
		os << rofcore::indent(0) << "<cprefix_in6 prefixlen: " << prefix.get_prefixlen() << " >" << std::endl;
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
