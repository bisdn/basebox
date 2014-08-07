/*
 * cfibentry.hpp
 *
 *  Created on: 15.07.2013
 *      Author: andreas
 */

#ifndef CFIBENTRY_HPP_
#define CFIBENTRY_HPP_ 1

#include <inttypes.h>
#include <iostream>
#include <exception>

#include <rofl/common/ciosrv.h>
#include <rofl/common/caddress.h>
#include <rofl/common/crofbase.h>
#include <rofl/common/crofdpt.h>
#include <rofl/common/logging.h>

#include "logging.h"
#include "cdpid.hpp"

namespace ethcore {

class eFibEntryBase : public std::runtime_error {
public:
	eFibEntryBase(const std::string& __arg) : std::runtime_error(__arg) {};
};
class eFibEntryNotFound : public eFibEntryBase {
public:
	eFibEntryNotFound(const std::string& __arg) : eFibEntryBase(__arg) {};
};

class cfibentry; // forward declaration, see below

class cfibentry_owner {
public:
	virtual ~cfibentry_owner() {};
	virtual void fib_expired(cfibentry& entry) = 0;
};

class cfibentry : public rofl::ciosrv {
public:

	/**
	 *
	 */
	cfibentry() :
		state(STATE_IDLE), fib(0), vid(0), portno(0), tagged(true),
		entry_timeout(0), dst_stage_table_id(2), src_stage_table_id(1) {};

	/**
	 *
	 */
	cfibentry(
			cfibentry_owner *fib,
			cdpid dpid,
			uint16_t vid,
			uint32_t portno,
			bool tagged,
			const rofl::caddress_ll& lladdr) :
				state(STATE_IDLE), fib(fib), dpid(dpid), vid(vid), portno(portno), tagged(tagged), lladdr(lladdr),
				entry_timeout(FIB_ENTRY_TIMEOUT), dst_stage_table_id(2), src_stage_table_id(1) {};

	/**
	 *
	 */
	virtual
	~cfibentry() {};

	/**
	 *
	 */
	cfibentry(const cfibentry& entry) { *this = entry; };

	/**
	 *
	 */
	cfibentry&
	operator= (const cfibentry& entry) {
		if (this == &entry)
			return *this;
		fib 				= entry.fib;
		dpid 				= entry.dpid;
		vid 				= entry.vid;
		portno 				= entry.portno;
		tagged 				= entry.tagged;
		lladdr 				= entry.lladdr;
		src_stage_table_id 	= entry.src_stage_table_id;
		dst_stage_table_id 	= entry.dst_stage_table_id;
		entry_timeout 		= entry.entry_timeout;
		return *this;
	};

public:

	/**
	 *
	 */
	const cdpid&
	get_dpid() const { return dpid; };

	/**
	 *
	 */
	uint16_t
	get_vid() const { return vid; };

	/**
	 *
	 */
	uint32_t
	get_portno() const { return portno; };

	/**
	 *
	 */
	bool
	get_tagged() const { return tagged; };

	/**
	 *
	 */
	rofl::cmacaddr const&
	get_lladdr() const { return lladdr; };

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


private:

	/**
	 *
	 */
	virtual void
	handle_timeout(int opaque, void *data = (void*)0) {
		switch (opaque) {
		case TIMER_ENTRY_EXPIRED: {
			if (fib) fib->fib_expired(*this);
		} return; // immediate return, this instance might have been deleted here already
		}
	};

	/**
	 *
	 */
	void
	drop_buffer(
			const rofl::cauxid& auxid, uint32_t buffer_id);

public:

	/**
	 *
	 */
	friend std::ostream&
	operator<< (std::ostream& os, const cfibentry& entry) {
		os << rofcore::indent(0) << "<fibentry ";
			os << "dpid:" 		<< entry.get_dpid() << " ";
			os << "vid:" 		<< (int)entry.get_vid() << " ";
			os << "lladdr: " 	<< entry.get_lladdr() << " ";
			os << "portno: " 	<< entry.get_portno() << " ";
			os << "tagged:" 	<< (entry.get_tagged() ? "true":"false") << " ";
		os << ">" << std::endl;
		return os;
	};

private:

	enum dpt_state_t {
		STATE_IDLE = 1,
		STATE_DETACHED = 2,
		STATE_ATTACHED = 3,
	};

	dpt_state_t			state;

	enum cfibentry_timer_type_t {
		TIMER_ENTRY_EXPIRED = 1,
	};

	static const int FIB_ENTRY_TIMEOUT = 20; // seconds

	cfibentry_owner				*fib;
	cdpid						dpid;
	uint16_t					vid;
	uint32_t					portno;
	bool						tagged;
	rofl::cmacaddr				lladdr;

	uint8_t						src_stage_table_id;
	uint8_t						dst_stage_table_id;
	int							entry_timeout;
	rofl::ctimerid				expiration_timer_id;
};

}; // end of namespace ethcore

#endif /* CFIBENTRY_HPP_ */
