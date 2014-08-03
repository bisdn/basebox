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
#include "logging.h"
#include "cvlan.hpp"

namespace ethcore {

class cvlantable {
public:

	/**
	 *
	 */
	static cvlantable&
	add_vtable(const rofl::cdptid& dptid) {
		if (cvlantable::vtables.find(dptid) != cvlantable::vtables.end()) {
			delete cvlantable::vtables[dptid];
		}
		new cvlantable(dptid);
		return *(cvlantable::vtables[dptid]);
	};

	/**
	 *
	 */
	static cvlantable&
	set_vtable(const rofl::cdptid& dptid) {
		if (cvlantable::vtables.find(dptid) == cvlantable::vtables.end()) {
			new cvlantable(dptid);
		}
		return *(cvlantable::vtables[dptid]);
	};

	/**
	 *
	 */
	static const cvlantable&
	get_vtable(const rofl::cdptid& dptid) {
		if (cvlantable::vtables.find(dptid) == cvlantable::vtables.end()) {
			throw eVlanNotFound("cvlantable::get_vtable() dptid not found");
		}
		return *(cvlantable::vtables.at(dptid));
	};

	/**
	 *
	 */
	static void
	drop_vtable(const rofl::cdptid& dptid) {
		if (cvlantable::vtables.find(dptid) == cvlantable::vtables.end()) {
			return;
		}
		delete cvlantable::vtables[dptid];
	};

	/**
	 *
	 */
	static bool
	has_vtable(const rofl::cdptid& dptid) {
		return (not (cvlantable::vtables.find(dptid) == cvlantable::vtables.end()));
	};

public:

	/**
	 *
	 */
	cvlantable(const rofl::cdptid& dptid) : dptid(dptid) {
		if (cvlantable::vtables.find(dptid) != cvlantable::vtables.end()) {
			throw eVlanExists("cvlantable::cvlantable() dptid already exists");
		}
		cvlantable::vtables[dptid] = this;
	};

	/**
	 *
	 */
	~cvlantable() {
		cvlantable::vtables.erase(dptid);
	};

public:

	/**
	 *
	 */
	const rofl::cdptid&
	get_dptid() const { return dptid; };

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
		return (vlans[vid] = cvlan(dptid, vid));
	};

	/**
	 *
	 */
	cvlan&
	set_vlan(uint16_t vid) {
		if (vlans.find(vid) == vlans.end()) {
			vlans[vid] = cvlan(dptid, vid);
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

private:

	rofl::cdptid								dptid;
	std::map<uint16_t, cvlan>					vlans;

	static std::map<rofl::cdptid, cvlantable*> 	vtables;
};

}; // end of namespace

#endif /* CVLANTABLE_HPP_ */
