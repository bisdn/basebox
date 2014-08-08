/*
 * cvlantable.hpp
 *
 *  Created on: 03.08.2014
 *      Author: andreas
 */

#ifndef CVLANTABLE_HPP_
#define CVLANTABLE_HPP_

#include <inttypes.h>
#include <iostream>
#include <map>
#include <rofl/common/crofdpt.h>

#include "clogging.h"
#include "cvlan.hpp"
#include "cdpid.hpp"

namespace ethcore {

class cvlantable {
public:

	/**
	 *
	 */
	static cvlantable&
	add_vtable(const cdpid& dpid) {
		if (cvlantable::vtables.find(dpid) != cvlantable::vtables.end()) {
			delete cvlantable::vtables[dpid];
		}
		new cvlantable(dpid);
		return *(cvlantable::vtables[dpid]);
	};

	/**
	 *
	 */
	static cvlantable&
	set_vtable(const cdpid& dpid) {
		if (cvlantable::vtables.find(dpid) == cvlantable::vtables.end()) {
			new cvlantable(dpid);
		}
		return *(cvlantable::vtables[dpid]);
	};

	/**
	 *
	 */
	static const cvlantable&
	get_vtable(const cdpid& dpid) {
		if (cvlantable::vtables.find(dpid) == cvlantable::vtables.end()) {
			throw eVlanNotFound("cvlantable::get_vtable() dpid not found");
		}
		return *(cvlantable::vtables.at(dpid));
	};

	/**
	 *
	 */
	static void
	drop_vtable(const cdpid& dpid) {
		if (cvlantable::vtables.find(dpid) == cvlantable::vtables.end()) {
			return;
		}
		delete cvlantable::vtables[dpid];
	};

	/**
	 *
	 */
	static bool
	has_vtable(const cdpid& dpid) {
		return (not (cvlantable::vtables.find(dpid) == cvlantable::vtables.end()));
	};

public:

	/**
	 *
	 */
	cvlantable(const cdpid& dpid) : state(STATE_IDLE), dpid(dpid) {
		if (cvlantable::vtables.find(dpid) != cvlantable::vtables.end()) {
			throw eVlanExists("cvlantable::cvlantable() dptid already exists");
		}
		cvlantable::vtables[dpid] = this;
	};

	/**
	 *
	 */
	~cvlantable() {
		if (STATE_ATTACHED == state) {
			// TODO
		}
		cvlantable::vtables.erase(dpid);
	};

public:

	/**
	 *
	 */
	const cdpid&
	get_dpid() const { return dpid; };

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
			vlans[vid].handle_dpt_open(rofl::crofdpt::get_dpt(dpid.get_dpid()));
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
				vlans[vid].handle_dpt_open(rofl::crofdpt::get_dpt(dpid.get_dpid()));
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
			throw eVlanNotFound("cvlantable::get_vlan() vid not found");
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
	operator<< (std::ostream& os, const cvlantable& vtable) {
		os << rofcore::indent(0) << "<cvlantable "
				<< " dpid: " << vtable.get_dpid() << " >" << std::endl;
		rofcore::indent i(2);
		for (std::map<uint16_t, cvlan>::const_iterator
				it = vtable.vlans.begin(); it != vtable.vlans.end(); ++it) {
			os << it->second;
		}
		return os;
	};

private:

	enum cvlan_state_t {
		STATE_IDLE = 1,
		STATE_DETACHED = 2,
		STATE_ATTACHED = 3,
	};

	cvlan_state_t							state;
	cdpid									dpid;
	std::map<uint16_t, cvlan>				vlans;
	std::set<uint32_t>						group_ids;

	static std::map<cdpid, cvlantable*> 	vtables;
};

}; // end of namespace

#endif /* CVLANTABLE_HPP_ */
