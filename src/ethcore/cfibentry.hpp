/*
 * cfibentry.hpp
 *
 *  Created on: 15.07.2013
 *      Author: andreas
 */

#ifndef CFIBENTRY_HPP_
#define CFIBENTRY_HPP_ 1

#include <iostream>

#ifdef __cplusplus
extern "C" {
#endif
#include <inttypes.h>
#ifdef __cplusplus
}
#endif

#include <rofl/common/ciosrv.h>
#include <rofl/common/caddress.h>
#include <rofl/common/crofbase.h>
#include <rofl/common/crofdpt.h>
#include <rofl/common/logging.h>

#include "logging.h"
#include "cdpid.hpp"

namespace ethcore {

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
		fib(0), dpid(0), vid(0), portno(0), tagged(true),
		entry_timeout(0), dst_stage_table_id(2), src_stage_table_id(1) {};

	/**
	 *
	 */
	cfibentry(
			cfibentry_owner *fib,
			uint64_t dpid,
			uint16_t vid,
			uint32_t portno,
			bool tagged,
			const rofl::caddress_ll& lladdr) :
				fib(fib), dpid(dpid), vid(vid), portno(portno), tagged(tagged), lladdr(lladdr),
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
