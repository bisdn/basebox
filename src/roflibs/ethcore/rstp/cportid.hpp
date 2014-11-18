/*
 * cportid.hpp
 *
 *  Created on: 22.09.2014
 *      Author: andreas
 */

#ifndef CPORTID_HPP_
#define CPORTID_HPP_

#include <inttypes.h>
#include <ostream>
#include <sstream>
#include <exception>

namespace roflibs {
namespace eth {
namespace rstp {

class cportid {
public:

	/**
	 *
	 */
	cportid() :
		pid(0)
	{};

	/**
	 *
	 */
	cportid(uint16_t pid) :
		pid(pid)
	{};

	/**
	 *
	 */
	~cportid()
	{};

	/**
	 *
	 */
	cportid(const cportid& portid) {
		*this = portid;
	};

	/**
	 *
	 */
	cportid&
	operator= (const cportid& portid) {
		if (this == &portid)
			return *this;
		pid = portid.pid;
		return *this;
	};

	/**
	 *
	 */
	bool
	operator< (const cportid& portid) const {
		return (pid < portid.pid);
	};

	/**
	 *
	 */
	bool
	operator== (const cportid& portid) const {
		return (pid == portid.pid);
	};

	/**
	 *
	 */
	bool
	operator!= (const cportid& portid) const {
		return (pid != portid.pid);
	};

public:

	const uint16_t&
	get_portid() const { return pid; };

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cportid& portid) {
		os << "<cportid port-id:" << (unsigned long long)portid.pid << " >";
		return os;
	};

	const std::string
	str() const {
		std::stringstream ss; ss << std::hex << "0x" << pid << std::dec; return ss.str();
		//std::stringstream ss; ss << *this; return ss.str();
	};

private:

	uint16_t	pid;	// root path cost
};

}; // end of namespace rstp
}; // end of namespace ethernet
}; // end of namespace roflibs

#endif /* CPORTID_HPP_ */
