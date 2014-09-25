/*
 * cport.hpp
 *
 *  Created on: 22.09.2014
 *      Author: andreas
 */

#ifndef CPORT_HPP_
#define CPORT_HPP_

#include <ostream>
#include <exception>

#include <rofl/common/cpacket.h>

#include "cportid.hpp"

namespace roflibs {
namespace ethernet {
namespace rstp {

class ePortBase : public std::runtime_error {
public:
	ePortBase(const std::string& __arg) : std::runtime_error(__arg) {};
};
class ePortNotFound : public ePortBase {
public:
	ePortNotFound(const std::string& __arg) : ePortBase(__arg) {};
};

class cport_env {
public:
	virtual ~cport_env() {};
	virtual void send_bpdu_message(const cportid& portid, const rofl::cpacket& bpdu) = 0;
};

class cport {
public:

	/**
	 *
	 */
	cport() :
		env(NULL)
	{};

	/**
	 *
	 */
	cport(cport_env* env, const cportid& pid) :
		env(env),
		pid(pid)
	{};

	/**
	 *
	 */
	~cport()
	{};

	/**
	 *
	 */
	cport(const cport& port) {
		*this = port;
	};

	/**
	 *
	 */
	cport&
	operator= (const cport& port) {
		if (this == &port)
			return *this;
		env = port.env;
		pid = port.pid;
		return *this;
	};

public:

	/**
	 *
	 */
	void
	send_bpdu_conf();

	/**
	 *
	 */
	void
	send_bpdu_tcn();

	/**
	 *
	 */
	void
	send_bpdu_rst();

private:

	cport_env*	env;
	cportid		pid;
};

}; // end of namespace rstp
}; // end of namespace ethernet
}; // end of namespace roflibs

#endif /* CPORT_HPP_ */
