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

#include <rofl/common/crofdpt.h>
#include <rofl/common/cdptid.h>

#include "clogging.h"
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
		state(STATE_IDLE), portno(0xffffffff), vid(0xffff), tagged(false) {};

	/**
	 *
	 */
	cmemberport(
			const cdpid& dpid, uint32_t portno = 0xffffffff,
			uint16_t vid = 0xffff, bool tagged = true) :
		state(STATE_IDLE), dpid(dpid), portno(portno), vid(vid), tagged(tagged) {};

	/**
	 *
	 */
	~cmemberport() {
		try {
			if (STATE_ATTACHED == state) {
				handle_dpt_close(rofl::crofdpt::get_dpt(dpid.get_dpid()));
			}
		} catch (rofl::eRofDptNotFound& e) {};
	};

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
		state	= port.state;
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

	/**
	 *
	 */
	void
	handle_dpt_open(
			rofl::crofdpt& dpt);

	/**
	 *
	 */
	void
	handle_dpt_close(
			rofl::crofdpt& dpt);

	/**
	 *
	 */
	void
	handle_packet_in(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_packet_in& msg);

	/**
	 *
	 */
	void
	handle_flow_removed(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_flow_removed& msg);

	/**
	 *
	 */
	void
	handle_port_status(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_port_status& msg);

	/**
	 *
	 */
	void
	handle_error_message(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_error& msg);

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

	enum dpt_state_t {
		STATE_IDLE = 1,
		STATE_DETACHED = 2,
		STATE_ATTACHED = 3,
	};

	dpt_state_t		state;

	cdpid			dpid;
	uint32_t		portno;
	uint16_t 		vid;
	bool 			tagged;
};

}; // end of namespace ethcore

#endif /* CMEMBERPORT_HPP_ */
