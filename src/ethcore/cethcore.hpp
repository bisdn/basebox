#ifndef ETHERSWITCH_H
#define ETHERSWITCH_H 1

#include <set>
#include <map>

#include <rofl/common/caddress.h>
#include <rofl/common/crofbase.h>
#include <rofl/common/crofdpt.h>
#include <rofl/common/logging.h>

#include "sport.h"
#include "logging.h"
#include "cnetlink.h"
#include "cdptlink.h"
#include "cconfig.h"
#include "clinktable.h"
#include "cvlantable.hpp"
#include "cdpid.hpp"

namespace ethcore {

class eEthCoreBase : public std::runtime_error {
public:
	eEthCoreBase(const std::string& __arg) : std::runtime_error(__arg) {};
};
class eEthCoreNotFound : public eEthCoreBase {
public:
	eEthCoreNotFound(const std::string& __arg) : eEthCoreBase(__arg) {};
};
class eEthCoreExists : public eEthCoreBase {
public:
	eEthCoreExists(const std::string& __arg) : eEthCoreBase(__arg) {};
};


class cethcore : public rofcore::cnetlink_common_observer {
public:

	/**
	 *
	 */
	static cethcore&
	add_ethcore(const cdpid& dpid) {
		if (cethcore::ethcores.find(dpid) != cethcore::ethcores.end()) {
			delete cethcore::ethcores[dpid];
		}
		new cethcore(dpid);
		return *(cethcore::ethcores[dpid]);
	};

	/**
	 *
	 */
	static cethcore&
	set_ethcore(const cdpid& dpid) {
		if (cethcore::ethcores.find(dpid) == cethcore::ethcores.end()) {
			new cethcore(dpid);
		}
		return *(cethcore::ethcores[dpid]);
	};

	/**
	 *
	 */
	static const cethcore&
	get_ethcore(const cdpid& dpid) {
		if (cethcore::ethcores.find(dpid) == cethcore::ethcores.end()) {
			throw eEthCoreNotFound("cethcore::get_ethcore() dpid not found");
		}
		return *(cethcore::ethcores.at(dpid));
	};

	/**
	 *
	 */
	static void
	drop_ethcore(const cdpid& dpid) {
		if (cethcore::ethcores.find(dpid) == cethcore::ethcores.end()) {
			return;
		}
		delete cethcore::ethcores[dpid];
	};

	/**
	 *
	 */
	static bool
	has_ethcore(const cdpid& dpid) {
		return (not (cethcore::ethcores.find(dpid) == cethcore::ethcores.end()));
	};

public:

	/**
	 *
	 */
	cethcore(const cdpid& dpid) :
				dpid(dpid),
				port_stage_table_id(0),
				fib_in_stage_table_id(1),
				fib_out_stage_table_id(2),
				default_vid(1),
				netlink_enabled(false) {
		if (cethcore::ethcores.find(dpid) != cethcore::ethcores.end()) {
			throw eEthCoreExists("cethcore::cethcore() dpid already exists");
		}
		cethcore::ethcores[dpid] = this;
	};

	/**
	 *
	 */
	virtual
	~cethcore() {
		cethcore::ethcores.erase(dpid);
	};

private:

	virtual void
	link_created(unsigned int ifindex);

	virtual void
	link_updated(unsigned int ifindex);

	virtual void
	link_deleted(unsigned int ifindex);

public:

	virtual void
	handle_dpt_open(
			rofl::crofdpt& dpt);

	virtual void
	handle_dpt_close(
			rofl::crofdpt& dpt);

	virtual void
	handle_packet_in(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_packet_in& msg);

	virtual void
	handle_port_status(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_port_status& msg);

	virtual void
	handle_flow_removed(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_flow_removed& msg);

	virtual void
	handle_error_message(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_error& msg);

public:

	virtual uint32_t
	get_idle_group_id();

	virtual void
	release_group_id(uint32_t group_id);


public:

	friend std::ostream&
	operator<< (std::ostream& os, const cethcore& ec) {
		os << rofcore::indent(0) << "<cethcore ";
			os << "default-vid:" << (int)ec.default_vid << " ";
			os << "port-stage-table-id:" << (int)ec.port_stage_table_id << " ";
			os << "fib-in-stage-table-id:" << (int)ec.fib_in_stage_table_id << " ";
			os << "fib-out-stage-table-id:" << (int)ec.fib_out_stage_table_id << " ";
		os << ">" << std::endl;
		return os;
	};

private:

	cdpid				dpid;
	uint8_t				port_stage_table_id;
	uint8_t				fib_in_stage_table_id;
	uint8_t				fib_out_stage_table_id;
	uint16_t			default_vid;
	std::set<uint32_t>	group_ids;		// set of group-ids in use by this controller (assigned to sport instances and cfib instance)
	//std::map<rofl::cdptid, clinktable>	ltable;
	bool				netlink_enabled;

	static std::map<cdpid, cethcore*> 	ethcores;
};

}; // end of namespace

#endif
