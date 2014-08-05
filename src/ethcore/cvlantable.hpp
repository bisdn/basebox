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
	cvlantable(const cdpid& dpid) : dpid(dpid) {
		if (cvlantable::vtables.find(dpid) != cvlantable::vtables.end()) {
			throw eVlanExists("cvlantable::cvlantable() dptid already exists");
		}
		cvlantable::vtables[dpid] = this;
	};

	/**
	 *
	 */
	~cvlantable() {
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
		return (vlans[vid] = cvlan(dpid, vid));
	};

	/**
	 *
	 */
	cvlan&
	set_vlan(uint16_t vid) {
		if (vlans.find(vid) == vlans.end()) {
			vlans[vid] = cvlan(dpid, vid);
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

	cdpid									dpid;
	std::map<uint16_t, cvlan>				vlans;

	static std::map<cdpid, cvlantable*> 	vtables;
};

}; // end of namespace

#endif /* CVLANTABLE_HPP_ */
