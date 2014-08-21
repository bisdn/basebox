/*
 * croutetable.hpp
 *
 *  Created on: 01.07.2014
 *      Author: andreas
 */

#ifndef CROUTETABLE_H_
#define CROUTETABLE_H_

#include <map>
#include <iostream>

#include "croute.hpp"
#include "clogging.h"

namespace rofip {

class eRtTableBase		: public std::runtime_error {
public:
	eRtTableBase(const std::string& __arg) : std::runtime_error(__arg) {};
};
class eRtTableNotFound : public eRtTableBase {
public:
	eRtTableNotFound(const std::string& __arg) : eRtTableBase(__arg) {};
};

class croutetable {
public:

	/**
	 *
	 */
	croutetable() :
		state(STATE_DETACHED), rttblid(0),
		local_ofp_table_id(0), out_ofp_table_id(2) {};

	/**
	 *
	 */
	croutetable(
			uint8_t rttblid, const rofl::cdpid& dpid,
			uint8_t local_ofp_table_id = 0, uint8_t out_ofp_table_id = 2) :
		state(STATE_DETACHED), rttblid(rttblid), dpid(dpid),
		local_ofp_table_id(local_ofp_table_id), out_ofp_table_id(out_ofp_table_id) {};

	/**
	 *
	 */
	~croutetable() {
		try {
			if (STATE_ATTACHED == state) {
				handle_dpt_close(rofl::crofdpt::get_dpt(dpid));
			}
		} catch (rofl::eRofDptNotFound& e) {};
	};

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
		state 				= rtable.state;
		dpid 				= rtable.dpid;
		rttblid 			= rtable.rttblid;
		local_ofp_table_id 	= rtable.local_ofp_table_id;
		out_ofp_table_id 	= rtable.out_ofp_table_id;
		rib4.clear();
		for (std::map<unsigned int, croute_in4>::const_iterator
				it = rtable.rib4.begin(); it != rtable.rib4.end(); ++it) {
			set_route_in4(it->first) = rtable.get_route_in4(it->first);
		}
		rib6.clear();
		for (std::map<unsigned int, croute_in6>::const_iterator
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
	croute_in4&
	add_route_in4(
			unsigned int rtindex) {
		if (rib4.find(rtindex) != rib4.end()) {
			rib4.erase(rtindex);
		}
		rib4[rtindex] = croute_in4(rttblid, rtindex, dpid, out_ofp_table_id);
		if (STATE_ATTACHED == state) {
			rib4[rtindex].handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
		}
		return rib4[rtindex];
	};

	/**
	 *
	 */
	croute_in4&
	set_route_in4(
			unsigned int rtindex) {
		if (rib4.find(rtindex) == rib4.end()) {
			rib4[rtindex] = croute_in4(rttblid, rtindex, dpid, out_ofp_table_id);
			if (STATE_ATTACHED == state) {
				rib4[rtindex].handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
			}
		}
		return (rib4[rtindex] = croute_in4(rttblid, rtindex, dpid, out_ofp_table_id));
	};

	/**
	 *
	 */
	const croute_in4&
	get_route_in4(
			unsigned int rtindex) const {
		if (rib4.find(rtindex) == rib4.end()) {
			throw eRtTableNotFound("croutetable::get_route_in4() rtindex not found");
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
	croute_in6&
	add_route_in6(
			unsigned int rtindex) {
		if (rib6.find(rtindex) != rib6.end()) {
			rib6.erase(rtindex);
		}
		rib6[rtindex] = croute_in6(rttblid, rtindex, dpid, out_ofp_table_id);
		if (STATE_ATTACHED == state) {
			rib6[rtindex].handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
		}
		return rib6[rtindex];
	};

	/**
	 *
	 */
	croute_in6&
	set_route_in6(
			unsigned int rtindex) {
		if (rib6.find(rtindex) == rib6.end()) {
			rib6[rtindex] = croute_in6(rttblid, rtindex, dpid, out_ofp_table_id);
			if (STATE_ATTACHED == state) {
				rib6[rtindex].handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
			}
		}
		return (rib6[rtindex] = croute_in6(rttblid, rtindex, dpid, out_ofp_table_id));
	};

	/**
	 *
	 */
	const croute_in6&
	get_route_in6(
			unsigned int rtindex) const {
		if (rib6.find(rtindex) == rib6.end()) {
			throw eRtTableNotFound("croutetable::get_route_in6() rtindex not found");
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

	/**
	 *
	 */
	void
	handle_dpt_open(rofl::crofdpt& dpt) {
		state = STATE_ATTACHED;
		for (std::map<unsigned int, croute_in4>::iterator
				it = rib4.begin(); it != rib4.end(); ++it) {
			it->second.handle_dpt_open(dpt);
		}
		for (std::map<unsigned int, croute_in6>::iterator
				it = rib6.begin(); it != rib6.end(); ++it) {
			it->second.handle_dpt_open(dpt);
		}
	};

	/**
	 *
	 */
	void
	handle_dpt_close(rofl::crofdpt& dpt) {
		state = STATE_DETACHED;
		for (std::map<unsigned int, croute_in4>::iterator
				it = rib4.begin(); it != rib4.end(); ++it) {
			it->second.handle_dpt_close(dpt);
		}
		for (std::map<unsigned int, croute_in6>::iterator
				it = rib6.begin(); it != rib6.end(); ++it) {
			it->second.handle_dpt_close(dpt);
		}
	};

public:

	friend std::ostream&
	operator<< (std::ostream& os, const croutetable& rtable) {
		os << rofcore::indent(0) << "<croutetable rttblid: " << (unsigned int)rtable.rttblid << " >" << std::endl;
		os << rofcore::indent(2) << "<RIBv4 >" << std::endl;
		{
			rofcore::indent i(4);
			for (std::map<unsigned int, croute_in4>::const_iterator
					it = rtable.rib4.begin(); it != rtable.rib4.end(); ++it) {
				os << it->second;
			}
		}
		os << rofcore::indent(2) << "<RIBv6 >" << std::endl;
		{
			rofcore::indent i(4);
			for (std::map<unsigned int, croute_in6>::const_iterator
					it = rtable.rib6.begin(); it != rtable.rib6.end(); ++it) {
				os << it->second;
			}
		}
		return os;
	};

private:


	enum ofp_state_t {
		STATE_DETACHED = 1,
		STATE_ATTACHED = 2,
	};

	enum ofp_state_t					state;
	rofl::cdpid 						dpid;
	uint8_t								rttblid;
	uint8_t								local_ofp_table_id;
	uint8_t								out_ofp_table_id;
	std::map<unsigned int, croute_in4> 	rib4;
	std::map<unsigned int, croute_in6> 	rib6;
};

}; // end of namespace cr4table

#endif /* CR4TABLE_H_ */
