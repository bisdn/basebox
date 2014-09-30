/*
 * cethcore.hpp
 *
 *  Created on: 03.08.2014
 *      Author: andreas
 */

#ifndef CETHCORE_HPP_
#define CETHCORE_HPP_

#include <inttypes.h>
#include <iostream>
#include <map>
#include <rofl/common/crofdpt.h>
#include <rofl/common/thread_helper.h>

#include "roflibs/netlink/cnetlink.hpp"
#include "roflibs/netlink/clogging.hpp"
#include "roflibs/ethcore/cvlan.hpp"

namespace roflibs {
namespace ethernet {

class eEthCoreBase : public std::runtime_error {
public:
	eEthCoreBase(const std::string& __arg) : std::runtime_error(__arg) {};
};
class eEthCoreNotFound : public eEthCoreBase {
public:
	eEthCoreNotFound(const std::string& __arg) : eEthCoreBase(__arg) {};
};


class cethcore : public rofcore::cnetlink_common_observer {
public:

	/**
	 *
	 */
	static cethcore&
	add_eth_core(const rofl::cdpid& dpid,
			uint8_t table_id_eth_in, uint8_t table_id_eth_src,
			uint8_t table_id_eth_local, uint8_t table_id_eth_dst,
			uint16_t default_pvid = DEFAULT_PVID) {
		if (cethcore::ethcores.find(dpid) != cethcore::ethcores.end()) {
			delete cethcore::ethcores[dpid];
		}
		new cethcore(dpid, table_id_eth_in, table_id_eth_src, table_id_eth_local, table_id_eth_dst, default_pvid);
		return *(cethcore::ethcores[dpid]);
	};

	/**
	 *
	 */
	static cethcore&
	set_eth_core(const rofl::cdpid& dpid,
			uint8_t table_id_eth_in, uint8_t table_id_eth_src,
			uint8_t table_id_eth_local, uint8_t table_id_eth_dst,
			uint16_t default_pvid = DEFAULT_PVID) {
		if (cethcore::ethcores.find(dpid) == cethcore::ethcores.end()) {
			new cethcore(dpid, table_id_eth_in, table_id_eth_src, table_id_eth_local, table_id_eth_dst, default_pvid);
		}
		return *(cethcore::ethcores[dpid]);
	};

	/**
	 *
	 */
	static cethcore&
	set_eth_core(const rofl::cdpid& dpid) {
		if (cethcore::ethcores.find(dpid) == cethcore::ethcores.end()) {
			throw eEthCoreNotFound("cethcore::get_vtable() dpid not found");
		}
		return *(cethcore::ethcores[dpid]);
	};

	/**
	 *
	 */
	static const cethcore&
	get_eth_core(const rofl::cdpid& dpid) {
		if (cethcore::ethcores.find(dpid) == cethcore::ethcores.end()) {
			throw eEthCoreNotFound("cethcore::get_vtable() dpid not found");
		}
		return *(cethcore::ethcores.at(dpid));
	};

	/**
	 *
	 */
	static void
	drop_eth_core(const rofl::cdpid& dpid) {
		if (cethcore::ethcores.find(dpid) == cethcore::ethcores.end()) {
			return;
		}
		delete cethcore::ethcores[dpid];
	};

	/**
	 *
	 */
	static bool
	has_eth_core(const rofl::cdpid& dpid) {
		return (not (cethcore::ethcores.find(dpid) == cethcore::ethcores.end()));
	};

private:

	/**
	 *
	 */
	cethcore(const rofl::cdpid& dpid,
			uint8_t table_id_eth_in, uint8_t table_id_eth_src,
			uint8_t table_id_eth_local, uint8_t table_id_eth_dst,
			uint16_t default_pvid = DEFAULT_PVID) :
		state(STATE_IDLE), dpid(dpid), default_pvid(default_pvid),
		table_id_eth_in(table_id_eth_in),
		table_id_eth_src(table_id_eth_src),
		table_id_eth_local(table_id_eth_local),
		table_id_eth_dst(table_id_eth_dst) {
		if (cethcore::ethcores.find(dpid) != cethcore::ethcores.end()) {
			throw eVlanExists("cethcore::cethcore() dptid already exists");
		}
		cethcore::ethcores[dpid] = this;
	};

	/**
	 *
	 */
	~cethcore() {
		if (STATE_ATTACHED == state) {
			// TODO
		}
		cethcore::ethcores.erase(dpid);
	};

public:

	/**
	 *
	 */
	const rofl::cdpid&
	get_dpid() const { return dpid; };

	/**
	 *
	 */
	uint16_t
	get_default_pvid() const { return default_pvid; };

public:

	/**
	 *
	 */
	std::vector<uint16_t>
	get_vlans() const {
		rofl::RwLock rwlock(vlans_rwlock, rofl::RwLock::RWLOCK_READ);
		std::vector<uint16_t> vids;
		for (std::map<uint16_t, cvlan>::const_iterator
				it = vlans.begin(); it != vlans.end(); ++it) {
			vids.push_back(it->first);
		}
		return vids;
	};

	/**
	 *
	 */
	void
	clear() { vlans.clear(); };

	/**
	 *
	 */
	cvlan&
	add_vlan(uint16_t vid) {
		rofl::RwLock rwlock(vlans_rwlock, rofl::RwLock::RWLOCK_WRITE);
		if (vlans.find(vid) != vlans.end()) {
			vlans.erase(vid);
		}
		vlans[vid] = cvlan(dpid, table_id_eth_in, table_id_eth_src, table_id_eth_local, table_id_eth_dst, vid);
		if (STATE_ATTACHED == state) {
			vlans[vid].handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
		}
		return vlans[vid];
	};

	/**
	 *
	 */
	cvlan&
	set_vlan(uint16_t vid) {
		rofl::RwLock rwlock(vlans_rwlock, rofl::RwLock::RWLOCK_WRITE);
		if (vlans.find(vid) == vlans.end()) {
			vlans[vid] = cvlan(dpid, table_id_eth_in, table_id_eth_src, table_id_eth_local, table_id_eth_dst, vid);
			if (STATE_ATTACHED == state) {
				vlans[vid].handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
			}
		}
		return vlans[vid];
	};

	/**
	 *
	 */
	void
	drop_vlan(uint16_t vid) {
		rofl::RwLock rwlock(vlans_rwlock, rofl::RwLock::RWLOCK_WRITE);
		if (vlans.find(vid) == vlans.end()) {
			return;
		}
		if (STATE_ATTACHED == state) {
			rofl::crofdpt::get_dpt(dpid).release_group_id(vlans[vid].get_group_id());
		}
		vlans.erase(vid);
	};

	/**
	 *
	 */
	const cvlan&
	get_vlan(uint16_t vid) const {
		rofl::RwLock rwlock(vlans_rwlock, rofl::RwLock::RWLOCK_READ);
		if (vlans.find(vid) == vlans.end()) {
			throw eVlanNotFound("cethcore::get_vlan() vid not found");
		}
		return vlans.at(vid);
	};

	/**
	 *
	 */
	bool
	has_vlan(uint16_t vid) const {
		rofl::RwLock rwlock(vlans_rwlock, rofl::RwLock::RWLOCK_READ);
		return (not (vlans.find(vid) == vlans.end()));
	};

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
	operator<< (std::ostream& os, const cethcore& core) {
		rofl::RwLock rwlock(core.vlans_rwlock, rofl::RwLock::RWLOCK_READ);
		os << rofcore::indent(0) << "<cethcore default-pvid: " << (int)core.get_default_pvid() << " >" << std::endl;
		rofcore::indent i(2);
		os << core.get_dpid();
		for (std::map<uint16_t, cvlan>::const_iterator
				it = core.vlans.begin(); it != core.vlans.end(); ++it) {
			os << it->second;
		}
		return os;
	};

private:

	enum cethcore_state_t {
		STATE_IDLE = 1,
		STATE_DETACHED = 2,
		STATE_QUERYING = 3,
		STATE_ATTACHED = 4,
	};

	static const uint16_t				DEFAULT_PVID = 1;

	cethcore_state_t					state;
	rofl::cdpid							dpid;
	uint16_t							default_pvid;
	std::map<uint16_t, cvlan>			vlans;
	mutable rofl::PthreadRwLock			vlans_rwlock;
	std::set<uint32_t>					group_ids;
	uint8_t								table_id_eth_in;
	uint8_t								table_id_eth_src;
	uint8_t								table_id_eth_local;
	uint8_t								table_id_eth_dst;

	static std::map<rofl::cdpid, cethcore*> 	ethcores;

public:

	// links
	virtual void
	link_created(unsigned int ifindex);

	virtual void
	link_updated(unsigned int ifindex);

	virtual void
	link_deleted(unsigned int ifindex);
};

}; // end of namespace ethernet
}; // end of namespace roflibs

#endif /* CETHCORE_HPP_ */
