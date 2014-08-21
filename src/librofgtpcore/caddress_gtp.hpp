/*
 * ctrspaddr.hpp
 *
 *  Created on: 19.08.2014
 *      Author: andreas
 */

#ifndef CTRSPADDR_HPP_
#define CTRSPADDR_HPP_

#include <inttypes.h>
#include <rofl/common/caddress.h>

#include "cport.hpp"
#include "clogging.h"

namespace rofgtp {

class caddress_gtp {
public:

	/**
	 *
	 */
	caddress_gtp() {};

	/**
	 *
	 */
	~caddress_gtp() {};

	/**
	 *
	 */
	caddress_gtp(const cport& port) :
		port(port) {};

	/**
	 *
	 */
	caddress_gtp(const caddress_gtp& addr) { *this = addr; };

	/**
	 *
	 */
	caddress_gtp&
	operator= (const caddress_gtp& addr) {
		if (this == &addr)
			return *this;
		this->port = addr.port;
		return *this;
	};

	/**
	 *
	 */
	bool
	operator< (const caddress_gtp& addr) const { return (this->port < addr.port); };

public:

	/**
	 *
	 */
	const cport&
	get_port() const { return port; };

public:

	friend std::ostream&
	operator<< (std::ostream& os, const caddress_gtp& addr) {
		os << "<caddress_gtp port:" << addr.get_port() << " >";
		return os;
	};

private:

	cport port;
};



class caddress_gtp_in4 : public caddress_gtp {
public:

	/**
	 *
	 */
	caddress_gtp_in4() {};

	/**
	 *
	 */
	~caddress_gtp_in4() {};

	/**
	 *
	 */
	caddress_gtp_in4(const rofl::caddress_in4& addr, const cport& port) :
		caddress_gtp(port), addr(addr) {};

	/**
	 *
	 */
	caddress_gtp_in4(const caddress_gtp_in4& addr) { *this = addr; };

	/**
	 *
	 */
	caddress_gtp_in4&
	operator= (const caddress_gtp_in4& addr) {
		if (this == &addr)
			return *this;
		caddress_gtp::operator= (addr);
		this->addr = addr.addr;
		return *this;
	};

	/**
	 *
	 */
	bool
	operator< (const caddress_gtp_in4& addr) const {
		return ((this->addr < addr.addr) || (caddress_gtp::operator< (addr)));
	};

public:

	/**
	 *
	 */
	const rofl::caddress_in4&
	get_addr() const { return addr; };

public:

	friend std::ostream&
	operator<< (std::ostream& os, const caddress_gtp_in4& addr) {
		os << rofcore::indent(0) << "<caddress_gtp_in4 >" << std::endl;
		os << rofcore::indent(2) << addr.get_port();
		os << addr.get_addr();
		return os;
	};

private:

	rofl::caddress_in4		addr;
};



class caddress_gtp_in6 : public caddress_gtp {
public:

	/**
	 *
	 */
	caddress_gtp_in6() {};

	/**
	 *
	 */
	~caddress_gtp_in6() {};

	/**
	 *
	 */
	caddress_gtp_in6(const rofl::caddress_in6& addr, const cport& port) :
		caddress_gtp(port), addr(addr) {};

	/**
	 *
	 */
	caddress_gtp_in6(const caddress_gtp_in6& addr) { *this = addr; };

	/**
	 *
	 */
	caddress_gtp_in6&
	operator= (const caddress_gtp_in6& addr) {
		if (this == &addr)
			return *this;
		caddress_gtp::operator= (addr);
		this->addr = addr.addr;
		return *this;
	};

	/**
	 *
	 */
	bool
	operator< (const caddress_gtp_in6& addr) const {
		return ((this->addr < addr.addr) || (caddress_gtp::operator< (addr)));
	};

public:

	/**
	 *
	 */
	const rofl::caddress_in6&
	get_addr() const { return addr; };

public:

	friend std::ostream&
	operator<< (std::ostream& os, const caddress_gtp_in6& addr) {
		os << rofcore::indent(0) << "<caddress_gtp_in6 >" << std::endl;
		os << rofcore::indent(2) << addr.get_port();
		os << addr.get_addr();
		return os;
	};

private:

	rofl::caddress_in6		addr;
};

}; // end of namespace rofgtp

#endif /* CTRSPADDR_HPP_ */
