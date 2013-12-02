/*
 * cfib.h
 *
 *  Created on: 15.07.2013
 *      Author: andreas
 */

#ifndef CFIB_H_
#define CFIB_H_ 1

#include <map>
#include <ostream>
#include <exception>
#include <algorithm>

#ifdef __cplusplus
extern "C" {
#endif
#include <inttypes.h>
#ifdef __cplusplus
}
#endif

#include <rofl/common/ciosrv.h>
#include <rofl/common/crofbase.h>
#include <rofl/common/openflow/cofdpt.h>
#include <rofl/common/cmacaddr.h>

#include "cfibentry.h"
#include "logging.h"
#include "sport.h"

namespace ethercore
{

class eFibBase			: public std::exception {};
class eFibBusy			: public eFibBase {};
class eFibInval			: public eFibBase {};
class eFibExists		: public eFibBase {};
class eFibNotFound		: public eFibBase {};

class cfib_owner {
public:
	/**
	 * @brief	Virtual destructor in vtable
	 */
	virtual ~cfib_owner() {};
	/**
	 * @brief	Returns handle to rofl::crofbase defining the OF endpoint
	 */
	virtual rofl::crofbase* get_rofbase() = 0;
	/**
	 * @brief	Returns an idle OpenFlow group-id for creating a group table entry
	 * @return assigned OF group-id
	 * @throw eSportExhausted when no idle group-ids are available
	 */
	virtual uint32_t get_idle_group_id() = 0;
	/**
	 * @brief	Release a previously assigned group-id for later reuse
	 * @param group_id assigned OF group-id
	 */
	virtual void release_group_id(uint32_t group_id) = 0;
};


class cfib :
		public cfibentry_owner,
		public rofl::ciosrv
{
	static std::map<uint64_t, std::map<uint16_t, cfib*> > fibs;

public:

	/**
	 * @brief	Returns reference to cfib instance associated with the specified <dpid,vid> pair
	 *
	 * @param dpid data path identifier
	 * @param vid VLAN identifier
	 *
	 * @return reference to cfib instance
	 * @throw eFibNotFound is thrown when no cfib instance exists for the specified <dpid,vid> pair
	 */
	static cfib&
	get_fib(uint64_t dpid, uint16_t vid = 0xffff);

	/**
	 * @brief	Destroy all cfib instances for a specific dpid
	 */
	static void
	destroy_fibs(uint64_t dpid);

private:

	cfib_owner								*fibowner;
	uint64_t 								dpid;		// OF data path identifier
	uint16_t								vid;		// VLAN identifier
	uint8_t									src_stage_table_id;
	uint8_t									dst_stage_table_id;
	std::map<rofl::cmacaddr, cfibentry*>	fibtable;	// map of mac-address <=> cfibentry mappings
	std::set<uint32_t>						ports;		// set of ports that are members of this VLAN
	uint32_t								flood_group_id;	// group-id for flood group entry for this cfib instance
	unsigned int							timer_dump_interval;

#define DEFAULT_TIMER_DUMP_INTERVAL 10

	enum cfib_timer_t {
		CFIB_TIMER_DUMP = 1,
	};

public:

	/**
	 * @brief	Constructor for a new cfib instance assigned to the specified <dpid,vid> pair
	 *
	 * Inserts this pointer of cfib instance to static map cfib::fibs
	 *
	 * @param fibowner surrounding environment for this cfib instance
	 * @param dpid OF data path identifier
	 * @param vid VLAN identifier
	 * @throw eFibExists is thrown in case of an already existing cfib instance for the specified <dpid,vid> pair
	 */
	cfib(
			cfib_owner *fibowner,
			uint64_t dpid,
			uint16_t vid,
			uint8_t src_stage_table_id,
			uint8_t dst_stage_table_id,
			unsigned int timer_dump_interval = DEFAULT_TIMER_DUMP_INTERVAL);

	/**
	 * @brief	Destructor
	 *
	 * Removes this pointer of cfib instance from static map cfib::fibs
	 */
	virtual
	~cfib();

	/**
	 * @brief	Add a port membership to this VLAN
	 *
	 * This notifies the associated sport instance of the new membership.
	 * Updates the flooding-group managed by this cfib instance with the new member.
	 */
	void
	add_port(
			uint32_t portno,
			bool tagged = true);

	/**
	 * @brief	Drops a port membership from this VLAN
	 *
	 * This notifies the associated sport instance of its changing membership.
	 * Updates the flooding-group managed by this cfib instance accordingly.
	 */
	void
	drop_port(
			uint32_t portno);

	/**
	 * @brief	Resets this cfib instance by removing all cfibentry instances.
	 *
	 * All sport memberships remain unaltered.
	 */
	void
	reset();

	/**
	 *
	 * @param msg
	 */
	virtual void
	handle_timeout(
			int opaque);

	/**
	 *
	 * @param dpt
	 * @param msg
	 */
	void
	handle_packet_in(
			rofl::cofmsg_packet_in& msg);

private:

	/**
	 *
	 */
	void
	block_stp();

	/**
	 *
	 */
	void
	add_flow_mod_in_stage();

	/**
	 *
	 */
	void
	drop_flow_mod_in_stage();

	/**
	 *
	 */
	void
	add_flow_mod_flood();

	/**
	 *
	 */
	void
	drop_flow_mod_flood();

	/**
	 *
	 */
	void
	add_group_entry_flood(
			bool modify = false);

	/**
	 *
	 */
	void
	drop_group_entry_flood();

private:

	friend class cfibentry;
	friend class sport;

	/**
	 *
	 */
	virtual void
	fib_timer_expired(cfibentry *entry);

	/**
	 *
	 */
	virtual rofl::crofbase*
	get_rofbase() const { return fibowner->get_rofbase(); };

	/**
	 *
	 */
	virtual uint32_t
	get_flood_group_id(uint16_t vid);

public:

	/**
	 *
	 */
	friend std::ostream&
	operator<< (std::ostream& os, cfib const& fib)
	{
		os << "fib<";
			os << "dpid: " << fib.dpid << " ";
			os << "vid:" << fib.vid << " ";
		os << ">";
		std::map<rofl::cmacaddr, cfibentry*>::const_iterator it;
		for (it = fib.fibtable.begin(); it != fib.fibtable.end(); ++it) {
			os << std::endl << "\t" << *(it->second);
		}
		return os;
	};
};

}; // end of namespace

#endif /* CFIB_H_ */
