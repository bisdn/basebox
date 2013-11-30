/*
 * cfibentry.h
 *
 *  Created on: 15.07.2013
 *      Author: andreas
 */

#ifndef CFIBENTRY_H_
#define CFIBENTRY_H_ 1

#include <ostream>

#ifdef __cplusplus
extern "C" {
#endif
#include <inttypes.h>
#ifdef __cplusplus
}
#endif

#include <rofl/common/ciosrv.h>
#include <rofl/common/cmacaddr.h>
#include <rofl/common/crofbase.h>
#include <rofl/common/openflow/cofdpt.h>

#include "logging.h"

namespace ethercore
{

class cfibentry; // forward declaration, see below

class cfibentry_owner
{
public:
	virtual ~cfibentry_owner() {};
	virtual void fib_timer_expired(cfibentry *entry) = 0;
	virtual rofl::crofbase* get_rofbase() const = 0;
	virtual uint32_t get_flood_group_id(uint16_t vid) = 0;
};

class cfibentry :
		public rofl::ciosrv
{
#define CFIBENTRY_DEFAULT_TIMEOUT		60

private:

	enum cfibentry_timer_t {
		CFIBENTRY_ENTRY_EXPIRED = 0x99ae,
	};

	cfibentry_owner				*fib;
	uint64_t					dpid;
	uint16_t					vid;
	uint32_t					portno;
	bool						tagged;
	rofl::cmacaddr				lladdr;
	uint8_t						src_stage_table_id;
	uint8_t						dst_stage_table_id;
	int							entry_timeout;

public:

	/**
	 *
	 */
	cfibentry(
			cfibentry_owner *fib,
			uint8_t src_stage_table_id,
			uint8_t dst_stage_table_id,
			rofl::cmacaddr lladdr,
			uint64_t dpid,
			uint16_t vid,
			uint32_t out_port_no,
			bool tagged = true);

	/**
	 *
	 */
	virtual
	~cfibentry();

	/**
	 *
	 */
	uint32_t
	get_portno() const { return portno; };

	/**
	 *
	 */
	void
	set_portno(uint32_t out_port_no);

	/**
	 *
	 */
	rofl::cmacaddr const&
	get_lladdr() const { return lladdr; };

	enum flow_mod_cmd_t {
		FLOW_MOD_ADD 	= 1,
		FLOW_MOD_MODIFY = 2,
		FLOW_MOD_DELETE = 3,
	};

	/**
	 *
	 */
	void
	flow_mod_configure(
			enum flow_mod_cmd_t flow_mod_cmd);


private:

	/**
	 *
	 */
	virtual void
	handle_timeout(int opaque);

public:

	/**
	 *
	 */
	friend std::ostream&
	operator<< (std::ostream& os, cfibentry const& entry)
	{
		os << "<fibentry ";
			os << "dpid:" 		<< (unsigned long long)entry.dpid << " ";
			os << "vid:" 		<< (int)entry.vid << " ";
			os << "lladdr: " 	<< entry.lladdr << " ";
			os << "portno: " 	<< entry.portno << " ";
			os << "tagged:" 	<< (entry.tagged ? "true":"false") << " ";
		os << ">";
		return os;
	};
};

}; // end of namespace

#endif /* CFIBENTRY_H_ */
