#ifndef ETHERSWITCH_H
#define ETHERSWITCH_H 1

#include <set>
#include <map>

#include <rofl/common/cmacaddr.h>
#include <rofl/common/caddress.h>
#include <rofl/common/crofbase.h>
#include <rofl/common/openflow/cofdpt.h>

#include "cfib.h"
#include "sport.h"
#include "logging.h"

using namespace rofl;

namespace ethercore
{

class ethcore :
		public crofbase,
		public sport_owner,
		public cfib_owner
{
private:

	uint8_t				table_id;
	uint16_t			default_vid;
	std::set<uint32_t>	group_ids;		// set of group-ids in use by this controller (assigned to sport instances and cfib instance)

	enum ethercore_timer_t {
		ETHSWITCH_TIMER_BASE 					= ((0x6271)),
		ETHSWITCH_TIMER_FIB 					= ((ETHSWITCH_TIMER_BASE) + 1),
		ETHSWITCH_TIMER_FLOW_STATS 				= ((ETHSWITCH_TIMER_BASE) + 2),
		ETHSWITCH_TIMER_FLOW_MOD_DELETE_ALL 	= ((ETHSWITCH_TIMER_BASE) + 3),
	};

public:

	/**
	 *
	 * @param table_id
	 * @param default_vid
	 */
	ethcore(
			uint8_t table_id = 0,
			uint16_t default_vid = 1);

	/**
	 *
	 */
	virtual
	~ethcore();

	/**
	 *
	 */
	void
	add_vlan(
			uint64_t dpid,
			uint16_t vid);

	/**
	 *
	 */
	void
	drop_vlan(
			uint64_t dpid,
			uint16_t vid);

	/**
	 *
	 */
	void
	add_port_to_vlan(
			uint64_t dpid,
			uint32_t portno,
			uint16_t vid,
			bool tagged = true);

	/**
	 *
	 */
	void
	drop_port_from_vlan(
			uint64_t dpid,
			uint32_t portno,
			uint16_t vid);

private:

	virtual void
	handle_timeout(int opaque);

	virtual void
	handle_dpath_open(cofdpt *dpt);

	virtual void
	handle_dpath_close(cofdpt *dpt);

	virtual void
	handle_packet_in(cofdpt *dpt, cofmsg_packet_in *msg);

	virtual void
	handle_flow_stats_reply(cofdpt *dpt, cofmsg_flow_stats_reply *msg);

public:

	void
	request_flow_stats();

	virtual rofl::crofbase*
	get_rofbase() { return this; };

	virtual uint32_t
	get_idle_group_id();

	virtual void
	release_group_id(uint32_t group_id);
};

}; // end of namespace

#endif
