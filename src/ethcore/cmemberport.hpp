/*
 * cmemberport.hpp
 *
 *  Created on: 04.08.2014
 *      Author: andreas
 */

#ifndef CMEMBERPORT_HPP_
#define CMEMBERPORT_HPP_

#include <inttypes.h>
#include <exception>
#include <iostream>
#include <rofl/common/cdptid.h>
#include "logging.h"
#include "cdpid.hpp"

namespace ethcore {

class eMemberPortBase : public std::runtime_error {
public:
	eMemberPortBase(const std::string& __arg) : std::runtime_error(__arg) {};
};
class eMemberPortNotFound : public eMemberPortBase {
public:
	eMemberPortNotFound(const std::string& __arg) : eMemberPortBase(__arg) {};
};

class cmemberport {
public:

	/**
	 *
	 */
	cmemberport() :
		portno(0xffffffff), vid(0xffff), tagged(false) {};

	/**
	 *
	 */
	cmemberport(
			const cdpid& dpid, uint32_t portno = 0xffffffff,
			uint16_t vid = 0xffff, bool tagged = true) :
		dpid(dpid), portno(portno), vid(vid), tagged(tagged) {};

	/**
	 *
	 */
	~cmemberport()
		{};

	/**
	 *
	 */
	cmemberport(const cmemberport& port)
		{ *this = port; };

	/**
	 *
	 */
	cmemberport&
	operator= (const cmemberport& port) {
		if (this == &port)
			return *this;
		dpid 	= port.dpid;
		portno 	= port.portno;
		vid 	= port.vid;
		tagged 	= port.tagged;
		return *this;
	};

public:

	/**
	 *
	 */
	uint32_t
	get_port_no() const { return portno; };

	/**
	 *
	 */
	bool
	get_tagged() const { return tagged; };

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cmemberport& port) {
		os << rofcore::indent(0) << "<cmemberport "
				<< "portno: " << (int)port.get_port_no() << " "
				<< "tagged: " << (int)port.get_tagged() << " "
				<< ">" << std::endl;
		return os;
	};

private:

	cdpid			dpid;
	uint32_t		portno;
	uint16_t 		vid;
	bool 			tagged;
};

}; // end of namespace ethcore

#endif /* CMEMBERPORT_HPP_ */
