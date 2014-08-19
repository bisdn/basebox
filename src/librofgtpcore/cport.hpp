/*
 * cport.hpp
 *
 *  Created on: 19.08.2014
 *      Author: andreas
 */

#ifndef CPORT_HPP_
#define CPORT_HPP_

#include <inttypes.h>
#include <ostream>

namespace rofgtp {

class cport {
public:

	/**
	 *
	 */
	cport() :
		port(0) {};

	/**
	 *
	 */
	~cport() {};

	/**
	 *
	 */
	explicit
	cport(uint16_t port) :
		port(port) {};

	/**
	 *
	 */
	cport(const cport& port) { *this = port; };

	/**
	 *
	 */
	cport&
	operator= (const cport& port) {
		if (this == &port)
			return *this;
		this->port = port.port;
		return *this;
	};

	/**
	 *
	 */
	bool
	operator< (const cport& port) const { return (this->port < port.port); };

public:

	/**
	 *
	 */
	uint16_t
	get_value() const { return port; };

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cport& port) {
		os << "<cport " << (unsigned int)port.get_value() << " >";
		return os;
	};

private:

	uint16_t port;
};

}; // end of namespace rofgtp

#endif /* CPORT_HPP_ */
