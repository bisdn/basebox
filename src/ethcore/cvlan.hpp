/*
 * cvlan.hpp
 *
 *  Created on: 03.08.2014
 *      Author: andreas
 */

#ifndef CVLAN_HPP_
#define CVLAN_HPP_

#include <inttypes.h>
#include <iostream>
#include <exception>
#include <rofl/common/crofdpt.h>

#include "logging.h"

namespace ethcore {

class eVlanBase : public std::runtime_error {
public:
	eVlanBase(const std::string& __arg) : std::runtime_error(__arg) {};
};
class eVlanNotFound : public eVlanBase {
public:
	eVlanNotFound(const std::string& __arg) : eVlanBase(__arg) {};
};
class eVlanExists : public eVlanBase {
public:
	eVlanExists(const std::string& __arg) : eVlanBase(__arg) {};
};


class cvlan {
public:

	/**
	 *
	 */
	cvlan() : vid(0), group_id(0) {};

	/**
	 *
	 */
	~cvlan() {};

	/**
	 *
	 */
	cvlan(const cvlan& vlan) { *this = vlan; };

	/**
	 *
	 */
	cvlan&
	operator= (const cvlan& vlan) {
		if (this == &vlan)
			return *this;
		dptid = vlan.dptid;
		vid = vlan.vid;
		group_id = vlan.group_id;
		return *this;
	};

	/**
	 *
	 */
	cvlan(const rofl::cdptid& dptid, uint16_t vid) :
		dptid(dptid), vid(vid), group_id(0) {};

public:

	/**
	 *
	 */
	void
	set_dptid(const rofl::cdptid& dptid) { this->dptid = dptid; };

	/**
	 *
	 */
	const rofl::cdptid&
	get_dptid() const { return dptid; };

	/**
	 *
	 */
	uint16_t
	get_vid() const { return vid; };

	/**
	 *
	 */
	uint32_t
	get_group_id() const { return group_id; };

	/**
	 *
	 */
	void
	set_group_id(uint32_t group_id) { this->group_id = group_id; };

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cvlan& vlan) {
		os << rofcore::indent(0) << "<cvlan vid: " << (int)vlan.get_vid() << " >" << std::endl;
		return os;
	};

private:

	rofl::cdptid		dptid;
	uint16_t			vid;
	uint32_t			group_id;	// OFP group identifier for broadcasting frames for this vid
};

}; // end of namespace

#endif /* CVLAN_HPP_ */
