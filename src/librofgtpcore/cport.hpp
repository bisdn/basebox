/*
 * cport.hpp
 *
 *  Created on: 19.08.2014
 *      Author: andreas
 */

#ifndef CPORT_HPP_
#define CPORT_HPP_

#include <sys/types.h>
#include <sys/socket.h>

#include <inttypes.h>
#include <ostream>
#include <exception>

namespace roflibs {
namespace gtp {

class ePortBase : public std::runtime_error {
public:
	ePortBase(const std::string& __arg) : std::runtime_error(__arg) {};
};
class ePortInval : public ePortBase {
public:
	ePortInval(const std::string& __arg) : ePortBase(__arg) {};
};

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
	explicit
	cport(struct sockaddr_in* sin, socklen_t salen) {
		if (salen < sizeof(struct sockaddr_in)) {
			throw ePortInval("cport::cport() invalid struct sockaddr_in");
		}
		port = be16toh(sin->sin_port);
	};

	/**
	 *
	 */
	explicit
	cport(struct sockaddr_in6* sin6, socklen_t salen) {
		if (salen < sizeof(struct sockaddr_in6)) {
			throw ePortInval("cport::cport() invalid struct sockaddr_in6");
		}
		port = be16toh(sin6->sin6_port);
	};

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

	/**
	 *
	 */
	std::string
	str() const {
		std::stringstream s_port; s_port << port;
		return s_port.str();
	};

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cport& port) {
		os << "<cport " << (unsigned int)port.get_value() << " >";
		return os;
	};

private:

	uint16_t port;
};

}; // end of namespace gtp
}; // end of namespace roflibs

#endif /* CPORT_HPP_ */
