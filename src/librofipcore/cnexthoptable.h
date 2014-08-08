/*
 * cnexthoptable.h
 *
 *  Created on: 27.07.2014
 *      Author: andreas
 */

#ifndef CNEXTHOPTABLE_H_
#define CNEXTHOPTABLE_H_

#include <map>
#include <iostream>
#include <algorithm>

#include "cdptnexthop.h"
#include "clogging.h"

namespace ipcore {

class eNextHopTableBase		: public std::exception {};
class eNextHopTableExists	: public eNextHopTableBase {};
class eNextHopTableNotFound : public eNextHopTableBase {};

class cnexthoptable {
public:

	/**
	 *
	 */
	cnexthoptable() {};

	/**
	 *
	 */
	~cnexthoptable() {};

	/**
	 *
	 */
	cnexthoptable(
			const cnexthoptable& table) { *this = table; };

	/**
	 *
	 */
	cnexthoptable&
	operator= (
			const cnexthoptable& table) {
		if (this == &table)
			return *this;
		clear();
		for (std::map<uint32_t, cdptnexthop_in4>::const_iterator
				it = table.nexthops_in4.begin(); it != table.nexthops_in4.end(); ++it) {
			add_nexthop_in4(it->first) = it->second;
		}
		for (std::map<uint32_t, cdptnexthop_in6>::const_iterator
				it = table.nexthops_in6.begin(); it != table.nexthops_in6.end(); ++it) {
			add_nexthop_in6(it->first) = it->second;
		}
		return *this;
	};

	/**
	 *
	 */
	void
	install() {
		for (std::map<unsigned int, cdptnexthop_in4>::iterator
				it = nexthops_in4.begin(); it != nexthops_in4.end(); ++it) {
			it->second.install();
		}
		for (std::map<unsigned int, cdptnexthop_in6>::iterator
				it = nexthops_in6.begin(); it != nexthops_in6.end(); ++it) {
			it->second.install();
		}
	};

	/**
	 *
	 */
	void
	uninstall() {
		for (std::map<unsigned int, cdptnexthop_in4>::iterator
				it = nexthops_in4.begin(); it != nexthops_in4.end(); ++it) {
			it->second.uninstall();
		}
		for (std::map<unsigned int, cdptnexthop_in6>::iterator
				it = nexthops_in6.begin(); it != nexthops_in6.end(); ++it) {
			it->second.uninstall();
		}
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
	clear() { nexthops_in4.clear(); nexthops_in6.clear(); };

	/**
	 *
	 */
	cdptnexthop_in4&
	add_nexthop_in4(unsigned int nhindex) {
		if (nexthops_in4.find(nhindex) != nexthops_in4.end()) {
			nexthops_in4.erase(nhindex);
		}
		//nexthops_in4[nhindex] = cdptnexthop_in4(dptid);
		return nexthops_in4[nhindex];
	};

	/**
	 *
	 */
	cdptnexthop_in4&
	set_nexthop_in4(unsigned int nhindex) {
		if (nexthops_in4.find(nhindex) == nexthops_in4.end()) {
			//nexthops_in4[nhindex] = cdptnexthop_in4(dptid);
		}
		return nexthops_in4[nhindex];
	};

	/**
	 *
	 */
	const cdptnexthop_in4&
	get_nexthop_in4(unsigned int nhindex) const {
		if (nexthops_in4.find(nhindex) == nexthops_in4.end()) {
			throw eNextHopTableNotFound();
		}
		return nexthops_in4.at(nhindex);
	};

	/**
	 *
	 */
	void
	drop_nexthop_in4(unsigned int nhindex) {
		if (nexthops_in4.find(nhindex) == nexthops_in4.end()) {
			return;
		}
		nexthops_in4.erase(nhindex);
	};

	/**
	 *
	 */
	bool
	has_nexthop_in4(unsigned int nhindex) const {
		return (not (nexthops_in4.find(nhindex) == nexthops_in4.end()));
	};


	/**
	 *
	 */
	cdptnexthop_in6&
	add_nexthop_in6(unsigned int nhindex) {
		if (nexthops_in6.find(nhindex) != nexthops_in6.end()) {
			nexthops_in6.erase(nhindex);
		}
		//nexthops_in6[nhindex].set_dptid(dptid);
		return nexthops_in6[nhindex];
	};

	/**
	 *
	 */
	cdptnexthop_in6&
	set_nexthop_in6(unsigned int nhindex) {
		if (nexthops_in6.find(nhindex) == nexthops_in6.end()) {
			//nexthops_in6[nhindex].set_dptid(dptid);
		}
		return nexthops_in6[nhindex];
	};

	/**
	 *
	 */
	const cdptnexthop_in6&
	get_nexthop_in6(unsigned int nhindex) const {
		if (nexthops_in6.find(nhindex) == nexthops_in6.end()) {
			throw eNextHopTableNotFound();
		}
		return nexthops_in6.at(nhindex);
	};

	/**
	 *
	 */
	void
	drop_nexthop_in6(unsigned int nhindex) {
		if (nexthops_in6.find(nhindex) == nexthops_in6.end()) {
			return;
		}
		nexthops_in6.erase(nhindex);
	};

	/**
	 *
	 */
	bool
	has_nexthop_in6(unsigned int nhindex) const {
		return (not (nexthops_in6.find(nhindex) == nexthops_in6.end()));
	};


public:

	friend std::ostream&
	operator<< (std::ostream& os, const cnexthoptable& table) {
		os << rofcore::indent(0) << "<cnexthoptable >" << std::endl;
		rofcore::indent i(2);
		for (std::map<unsigned int, cdptnexthop_in4>::const_iterator
				it = table.nexthops_in4.begin(); it != table.nexthops_in4.end(); ++it) {
			os << it->second;
		}
		for (std::map<unsigned int, cdptnexthop_in6>::const_iterator
				it = table.nexthops_in6.begin(); it != table.nexthops_in6.end(); ++it) {
			os << it->second;
		}
		return os;
	};

private:

	rofl::cdptid								dptid;
	std::map<unsigned int, cdptnexthop_in4> 	nexthops_in4;	// key: nexthop ofp port-no
	std::map<unsigned int, cdptnexthop_in6> 	nexthops_in6;	// key: nexthop ofp port-no
};

}; // end of namespace ipcore

#endif /* CNEXTHOPTABLE_H_ */
