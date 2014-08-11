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

#include "clogging.h"
#include "cvlan.hpp"
#include "cdpid.hpp"

namespace ethcore {

class cethcore {
public:

	/**
	 *
	 */
	static cethcore&
	add_core(const cdpid& dpid, uint16_t default_pvid = DEFAULT_PVID) {
		if (cethcore::ethcores.find(dpid) != cethcore::ethcores.end()) {
			delete cethcore::ethcores[dpid];
		}
		new cethcore(dpid, default_pvid);
		return *(cethcore::ethcores[dpid]);
	};

	/**
	 *
	 */
	static cethcore&
	set_core(const cdpid& dpid, uint16_t default_pvid = DEFAULT_PVID) {
		if (cethcore::ethcores.find(dpid) == cethcore::ethcores.end()) {
			new cethcore(dpid, default_pvid);
		}
		return *(cethcore::ethcores[dpid]);
	};

	/**
	 *
	 */
	static const cethcore&
	get_core(const cdpid& dpid) {
		if (cethcore::ethcores.find(dpid) == cethcore::ethcores.end()) {
			throw eVlanNotFound("cethcore::get_vtable() dpid not found");
		}
		return *(cethcore::ethcores.at(dpid));
	};

	/**
	 *
	 */
	static void
	drop_core(const cdpid& dpid) {
		if (cethcore::ethcores.find(dpid) == cethcore::ethcores.end()) {
			return;
		}
		delete cethcore::ethcores[dpid];
	};

	/**
	 *
	 */
	static bool
	has_core(const cdpid& dpid) {
		return (not (cethcore::ethcores.find(dpid) == cethcore::ethcores.end()));
	};

public:

	/**
	 *
	 */
	cethcore(const cdpid& dpid, uint16_t default_pvid = DEFAULT_PVID) :
		state(STATE_IDLE), dpid(dpid), default_pvid(default_pvid) {
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
	const cdpid&
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
	void
	clear() { vlans.clear(); };

	/**
	 *
	 */
	cvlan&
	add_vlan(uint16_t vid) {
		if (vlans.find(vid) != vlans.end()) {
			vlans.erase(vid);
		}
		vlans[vid] = cvlan(dpid, vid, get_next_group_id());
		if (STATE_ATTACHED == state) {
			vlans[vid].handle_dpt_open(rofl::crofdpt::get_dpt(dpid.get_dpid()), get_next_group_id());
		}
		return vlans[vid];
	};

	/**
	 *
	 */
	cvlan&
	set_vlan(uint16_t vid) {
		if (vlans.find(vid) == vlans.end()) {
			vlans[vid] = cvlan(dpid, vid, get_next_group_id());
			if (STATE_ATTACHED == state) {
				vlans[vid].handle_dpt_open(rofl::crofdpt::get_dpt(dpid.get_dpid()), get_next_group_id());
			}
		}
		return vlans[vid];
	};

	/**
	 *
	 */
	void
	drop_vlan(uint16_t vid) {
		if (vlans.find(vid) == vlans.end()) {
			return;
		}
		release_group_id(vlans[vid].get_group_id());
		vlans.erase(vid);
	};

	/**
	 *
	 */
	const cvlan&
	get_vlan(uint16_t vid) const {
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

	/**
	 *
	 */
	virtual void
	handle_port_desc_stats_reply(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_port_desc_stats_reply& msg);

	/**
	 *
	 */
	virtual void
	handle_port_desc_stats_reply_timeout(rofl::crofdpt& dpt, uint32_t xid);

private:

	/**
	 *
	 */
	void
	drop_buffer(
			const rofl::cauxid& auxid, uint32_t buffer_id);

	/**
	 *
	 */
	uint32_t
	get_next_group_id();

	/**
	 *
	 */
	void
	release_group_id(uint32_t group_id);

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cethcore& core) {
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
	cdpid								dpid;
	uint16_t							default_pvid;
	std::map<uint16_t, cvlan>			vlans;
	std::set<uint32_t>					group_ids;

	static std::map<cdpid, cethcore*> 	ethcores;
};

}; // end of namespace

#endif /* CETHCORE_HPP_ */
