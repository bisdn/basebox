#ifndef ETHERSWITCH_H
#define ETHERSWITCH_H 1

#include <set>
#include <map>

#include <rofl/common/caddress.h>
#include <rofl/common/crofbase.h>
#include <rofl/common/crofdpt.h>
#include <rofl/common/logging.h>

#include "cfib.h"
#include "sport.h"
#include "logging.h"
#include "cnetlink.h"
#include "cdptlink.h"
#include "cconfig.h"
#include "clinktable.h"

using namespace rofl;
using namespace rofl::openflow;

namespace ethercore {

class cethcore :
		public crofbase,
		public sport_owner,
		public cfib_owner,
		public rofcore::cnetlink_common_observer
{
private:

	uint64_t			dpid;
	uint8_t				port_stage_table_id;
	uint8_t				fib_in_stage_table_id;
	uint8_t				fib_out_stage_table_id;
	uint16_t			default_vid;
	std::set<uint32_t>	group_ids;		// set of group-ids in use by this controller (assigned to sport instances and cfib instance)
	unsigned int		timer_dump_interval;
	std::map<rofl::cdptid, clinktable>	ltable;
	bool				netlink_enabled;

#define DEFAULT_TIMER_DUMP_INTERVAL 10

	enum ethercore_timer_t {
		ETHSWITCH_TIMER_BASE 					= ((0x6271)),
		ETHSWITCH_TIMER_FIB 					= ((ETHSWITCH_TIMER_BASE) + 1),
		ETHSWITCH_TIMER_FLOW_STATS 				= ((ETHSWITCH_TIMER_BASE) + 2),
		ETHSWITCH_TIMER_FLOW_MOD_DELETE_ALL 	= ((ETHSWITCH_TIMER_BASE) + 3),
		ETHSWITCH_TIMER_DUMP				 	= ((ETHSWITCH_TIMER_BASE) + 4),
	};


    static cethcore          *sethcore;

    /**
     * @brief	Make copy constructor private for singleton
     */
    cethcore(const cethcore& ethcore) {};

	/**
	 *
	 * @param table_id
	 * @param default_vid
	 */
	cethcore(cofhello_elem_versionbitmap const& versionbitmap);

	/**
	 *
	 */
	virtual
	~cethcore();

public:

	/**
	 *
	 * @return
	 */
    static cethcore&
    get_instance(cofhello_elem_versionbitmap const& versionbitmap);


    /**
     *
     */
    void
    init(
    		uint8_t port_stage_table_id = 0,
			uint8_t fib_in_stage_table_id = 1,
			uint8_t fib_out_stage_table_id = 2,
			uint16_t default_vid = 1);


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
			std::string const& devname,
			uint16_t vid,
			bool tagged = true);

	/**
	 *
	 */
	void
	drop_port_from_vlan(
			uint64_t dpid,
			std::string const& devname,
			uint16_t vid);

private:

	virtual void
	link_created(unsigned int ifindex);

	virtual void
	link_updated(unsigned int ifindex);

	virtual void
	link_deleted(unsigned int ifindex);

private:

	virtual void
	handle_timeout(int opaque);

	virtual void
	handle_dpath_open(crofdpt& dpt);

	virtual void
	handle_dpath_close(crofdpt& dpt);

	virtual void
	handle_packet_in(crofdpt& dpt, cofmsg_packet_in& msg, uint8_t aux_id = 0);

	virtual void
	handle_port_status(crofdpt& dpt, cofmsg_port_status& msg, uint8_t aux_id = 0);

	virtual void
	handle_flow_stats_reply(crofdpt& dpt, cofmsg_flow_stats_reply& msg, uint8_t aux_id = 0);

	virtual void
	handle_flow_removed(crofdpt& dpt, cofmsg_flow_removed& msg, uint8_t aux_id = 0);

private:

	void
	request_flow_stats();

	virtual rofl::crofbase*
	get_rofbase() { return this; };

	virtual uint32_t
	get_idle_group_id();

	virtual void
	release_group_id(uint32_t group_id);


public:

	friend std::ostream&
	operator<< (std::ostream& os, const cethcore& ec) {
		os << indent(0) << "<ethcore ";
			os << "default-vid:" << (int)ec.default_vid << " ";
			os << "port-stage-table-id:" << (int)ec.port_stage_table_id << " ";
			os << "fib-in-stage-table-id:" << (int)ec.fib_in_stage_table_id << " ";
			os << "fib-out-stage-table-id:" << (int)ec.fib_out_stage_table_id << " ";
		os << ">" << std::endl;
		return os;
	};
};

}; // end of namespace

#endif
