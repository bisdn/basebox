/*
 * croutetable.h
 *
 *  Created on: 01.07.2014
 *      Author: andreas
 */

#ifndef CROUTETABLE_H_
#define CROUTETABLE_H_

#include <map>
#include <iostream>

#include "cdptroute.h"
#include "clogging.h"

namespace ipcore {

class eRtTableBase		: public std::exception {};
class eRtTableNotFound	: public eRtTableBase {};

class croutetable {
public:

	/**
	 *
	 */
	croutetable() {};

	/**
	 *
	 */
	~croutetable() {};

	/**
	 *
	 */
	croutetable(
			const croutetable& rtable) { *this = rtable; };

	/**
	 *
	 */
	croutetable&
	operator= (
			const croutetable& rtable) {
		if (this == &rtable)
			return *this;
		rib4.clear();
		for (std::map<unsigned int, cdptroute_in4>::const_iterator
				it = rtable.rib4.begin(); it != rtable.rib4.end(); ++it) {
			set_route_in4(it->first) = rtable.get_route_in4(it->first);
		}
		rib6.clear();
		for (std::map<unsigned int, cdptroute_in6>::const_iterator
				it = rtable.rib6.begin(); it != rtable.rib6.end(); ++it) {
			set_route_in6(it->first) = rtable.get_route_in6(it->first);
		}
		return *this;
	};

public:

	/**
	 *
	 */
	void
	clear() { clear_in4(); clear_in6(); };

	/**
	 *
	 */
	void
	clear_in4() { rib4.clear(); };

	/**
	 *
	 */
	void
	clear_in6() { rib6.clear(); };

	/**
	 *
	 */
	cdptroute_in4&
	add_route_in4(
			unsigned int rtindex) {
		if (rib4.find(rtindex) != rib4.end()) {
			rib4.erase(rtindex);
		}
		return rib4[rtindex];
	};

	/**
	 *
	 */
	cdptroute_in4&
	set_route_in4(
			unsigned int rtindex) {
		if (rib4.find(rtindex) == rib4.end()) {
			(void)rib4[rtindex];
		}
		return rib4[rtindex];
	};

	/**
	 *
	 */
	const cdptroute_in4&
	get_route_in4(
			unsigned int rtindex) const {
		if (rib4.find(rtindex) == rib4.end()) {
			throw eRtTableNotFound();
		}
		return rib4.at(rtindex);
	};

	/**
	 *
	 */
	void
	drop_route_in4(
			unsigned int rtindex) {
		if (rib4.find(rtindex) == rib4.end()) {
			return;
		}
		rib4.erase(rtindex);
	};

	/**
	 *
	 */
	bool
	has_route_in4(
			unsigned int rtindex) const {
		return (not (rib4.find(rtindex) == rib4.end()));
	};



	/**
	 *
	 */
	cdptroute_in6&
	add_route_in6(
			unsigned int rtindex) {
		if (rib6.find(rtindex) != rib6.end()) {
			rib6.erase(rtindex);
		}
		return rib6[rtindex];
	};

	/**
	 *
	 */
	cdptroute_in6&
	set_route_in6(
			unsigned int rtindex) {
		if (rib6.find(rtindex) == rib6.end()) {
			(void)rib6[rtindex];
		}
		return rib6[rtindex];
	};

	/**
	 *
	 */
	const cdptroute_in6&
	get_route_in6(
			unsigned int rtindex) const {
		if (rib6.find(rtindex) == rib6.end()) {
			throw eRtTableNotFound();
		}
		return rib6.at(rtindex);
	};

	/**
	 *
	 */
	void
	drop_route_in6(
			unsigned int rtindex) {
		if (rib6.find(rtindex) == rib6.end()) {
			return;
		}
		rib6.erase(rtindex);
	};

	/**
	 *
	 */
	bool
	has_route_in6(
			unsigned int rtindex) const {
		return (not (rib6.find(rtindex) == rib6.end()));
	};

public:

	friend std::ostream&
	operator<< (std::ostream& os, const croutetable& rtable) {
		os << rofcore::indent(0) << "<croutetable dptid: " << rtable.dptid << " >" << std::endl;
		os << rofcore::indent(2) << "<RIBv4 >" << std::endl;
		{
			rofcore::indent i(4);
			for (std::map<unsigned int, cdptroute_in4>::const_iterator
					it = rtable.rib4.begin(); it != rtable.rib4.end(); ++it) {
				os << it->second;
			}
		}
		os << rofcore::indent(2) << "<RIBv6 >" << std::endl;
		{
			rofcore::indent i(4);
			for (std::map<unsigned int, cdptroute_in6>::const_iterator
					it = rtable.rib6.begin(); it != rtable.rib6.end(); ++it) {
				os << it->second;
			}
		}
		return os;
	};

private:

	rofl::cdptid dptid;
	std::map<unsigned int, cdptroute_in4> 	rib4;
	std::map<unsigned int, cdptroute_in6> 	rib6;

};

}; // end of namespace cr4table

#endif /* CR4TABLE_H_ */
