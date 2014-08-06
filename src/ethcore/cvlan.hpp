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
#include <map>
#include <rofl/common/crofdpt.h>

#include "logging.h"
#include "cmemberport.hpp"
#include "cdpid.hpp"
#include "cfibentry.hpp"

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


class cvlan : public cfibentry_owner {
public:

	enum cvlan_vid_type_t {
		VID_NO_VLAN = 0xffff,
	};

	/**
	 *
	 */
	cvlan() :
		vid(VID_NO_VLAN), group_id(0) {};

	/**
	 *
	 */
	cvlan(const cdpid& dpid, uint16_t vid = VID_NO_VLAN, uint32_t group_id = 0) :
		dpid(dpid), vid(vid), group_id(group_id) {};

	/**
	 *
	 */
	~cvlan()
		{};

	/**
	 *
	 */
	cvlan(const cvlan& vlan)
		{ *this = vlan; };

	/**
	 *
	 */
	cvlan&
	operator= (const cvlan& vlan) {
		if (this == &vlan)
			return *this;
		dpid = vlan.dpid;
		vid = vlan.vid;
		group_id = vlan.group_id;
		return *this;
	};

public:

	/**
	 *
	 */
	cmemberport&
	add_port(uint32_t portno, bool tagged = true) {
		if (ports.find(portno) != ports.end()) {
			ports[portno] = cmemberport(dpid, portno, vid, tagged);
		}
		return ports[portno];
	};

	/**
	 *
	 */
	cmemberport&
	set_port(uint32_t portno, bool tagged = true) {
		if (ports.find(portno) == ports.end()) {
			ports[portno] = cmemberport(dpid, portno, vid, tagged);
		}
		return ports[portno];
	};

	/**
	 *
	 */
	const cmemberport&
	get_port(uint32_t portno) const {
		if (ports.find(portno) == ports.end()) {
			throw eMemberPortNotFound("cvlan::get_port() portno not found");
		}
		return ports.at(portno);
	};

	/**
	 *
	 */
	void
	drop_port(uint32_t portno) {
		if (ports.find(portno) == ports.end()) {
			return;
		}
		ports.erase(portno);
	}

	/**
	 *
	 */
	bool
	has_port(uint32_t portno) const {
		return (not (ports.find(portno) == ports.end()));
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
	get_vid() const { return vid; };

	/**
	 *
	 */
	uint32_t
	get_group_id() const { return group_id; };

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cvlan& vlan) {
		os << rofcore::indent(0) << "<cvlan "
				<< "vid: " << (int)vlan.get_vid() << " "
				<< "group-id: " << (int)vlan.get_group_id() << " "
				<< " >" << std::endl;
		rofcore::indent i(2);
		for (std::map<uint32_t, cmemberport>::const_iterator
				it = vlan.ports.begin(); it != vlan.ports.end(); ++it) {
			os << it->second;
		}
		return os;
	};

protected:

	friend class cfibentry;

	/**
	 *
	 */
	virtual void
	fib_expired(cfibentry& entry);

private:

	cdpid				dpid;
	uint16_t			vid;
	uint32_t			group_id;	// OFP group identifier for broadcasting frames for this vid

	std::map<uint32_t, cmemberport>		ports;
};

}; // end of namespace

#endif /* CVLAN_HPP_ */
