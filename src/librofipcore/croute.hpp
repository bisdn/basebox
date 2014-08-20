/*
 * croute.hpp
 *
 *  Created on: 02.07.2013
 *      Author: andreas
 */

#ifndef CROUTE_HPP_
#define CROUTE_HPP_ 1

#include <map>
#include <ostream>

#ifdef __cplusplus
extern "C" {
#endif
#include <inttypes.h>
#ifdef __cplusplus
}
#endif

#include <rofl/common/logging.h>
#include <rofl/common/crofbase.h>
#include <rofl/common/crofdpt.h>
#include <rofl/common/openflow/cofflowmod.h>
#include <rofl/common/croflexception.h>

#include "cnetlink.h"
#include "crtroute.h"
#include "clink.hpp"
#include "cnexthop.hpp"

namespace rofip {

class croute {
public:

	/**
	 *
	 */
	croute() :
		state(STATE_DETACHED), rttblid(0), rtindex(0), out_ofp_table_id(4) {};

	/**
	 *
	 */
	croute(
			uint8_t rttblid, unsigned int rtindex, const rofl::cdptid& dptid,
			uint8_t out_ofp_table_id = 4) :
				state(STATE_DETACHED), rttblid(rttblid), rtindex(rtindex), dptid(dptid),
				out_ofp_table_id(out_ofp_table_id) {};

	/**
	 *
	 */
	croute(const croute& dptroute) { *this = dptroute; };

	/**
	 *
	 */
	croute&
	operator= (const croute& route) {
		if (this == &route)
			return *this;
		state			= route.state;
		dptid			= route.dptid;
		rttblid			= route.rttblid;
		rtindex			= route.rtindex;
		out_ofp_table_id		= route.out_ofp_table_id;
		clear();
		for (std::map<uint32_t, cnexthop_in4>::const_iterator
				it = route.nexthops_in4.begin(); it != route.nexthops_in4.end(); ++it) {
			add_nexthop_in4(it->first) = it->second;
		}
		for (std::map<uint32_t, cnexthop_in6>::const_iterator
				it = route.nexthops_in6.begin(); it != route.nexthops_in6.end(); ++it) {
			add_nexthop_in6(it->first) = it->second;
		}
		return *this;
	};

	/**
	 *
	 */
	virtual
	~croute() {
		if (STATE_ATTACHED == state) {
			for (std::map<uint32_t, cnexthop_in4>::iterator
					it = nexthops_in4.begin(); it != nexthops_in4.end(); ++it) {
				it->second.handle_dpt_close(rofl::crofdpt::get_dpt(dptid));
			}
			for (std::map<uint32_t, cnexthop_in6>::iterator
					it = nexthops_in6.begin(); it != nexthops_in6.end(); ++it) {
				it->second.handle_dpt_close(rofl::crofdpt::get_dpt(dptid));
			}
		}
	};

public:

	/**
	 *
	 */
	const rofl::cdptid&
	get_dptid() const { return dptid; };

	/**
	 *
	 */
	unsigned int
	get_rtindex() const { return rtindex; };

	/**
	 *
	 */
	uint8_t
	get_rttblid() const { return rttblid; };

	/**
	 *
	 */
	uint8_t
	get_table_id() const { return out_ofp_table_id; };

public:

	/**
	 *
	 */
	void
	clear() { nexthops_in4.clear(); nexthops_in6.clear(); };

	/**
	 *
	 */
	cnexthop_in4&
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
	cnexthop_in4&
	set_nexthop_in4(unsigned int nhindex) {
		if (nexthops_in4.find(nhindex) == nexthops_in4.end()) {
			//nexthops_in4[nhindex] = cdptnexthop_in4(dptid);
		}
		return nexthops_in4[nhindex];
	};

	/**
	 *
	 */
	const cnexthop_in4&
	get_nexthop_in4(unsigned int nhindex) const {
		if (nexthops_in4.find(nhindex) == nexthops_in4.end()) {
			throw eNextHopNotFound("croute::get_nexthop_in4() nhindex not found");
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
	cnexthop_in6&
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
	cnexthop_in6&
	set_nexthop_in6(unsigned int nhindex) {
		if (nexthops_in6.find(nhindex) == nexthops_in6.end()) {
			//nexthops_in6[nhindex].set_dptid(dptid);
		}
		return nexthops_in6[nhindex];
	};

	/**
	 *
	 */
	const cnexthop_in6&
	get_nexthop_in6(unsigned int nhindex) const {
		if (nexthops_in6.find(nhindex) == nexthops_in6.end()) {
			throw eNextHopNotFound("croute::get_nexthop_in6() nhindex not found");
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

	/**
	 *
	 */
	friend std::ostream&
	operator<< (std::ostream& os, croute const& route) {
		os << rofcore::indent(0) << "<croute ";
		os << "rttblid:" << (unsigned int)route.get_rttblid() << " ";
		os << "rtindex:" << route.get_rtindex() << " ";
		os << " >" << std::endl;
		return os;
	};

protected:

	enum ofp_state_t {
		STATE_DETACHED = 1,
		STATE_ATTACHED = 2,
	};

	enum ofp_state_t			state;
	uint8_t					 	rttblid;
	unsigned int			 	rtindex;
	rofl::cdptid				dptid;
	uint8_t						out_ofp_table_id;	// openflow table-id for output stage

	/* we make here one assumption: only one nexthop exists per neighbor and route
	 * this should be valid under all circumstances
	 */
	std::map<unsigned int, cnexthop_in4> 	nexthops_in4;	// key: nexthop ofp port-no
	std::map<unsigned int, cnexthop_in6> 	nexthops_in6;	// key: nexthop ofp port-no
};



class croute_in4 : public croute {
public:

	/**
	 *
	 */
	croute_in4() {};

	/**
	 *
	 */
	croute_in4(
			uint8_t rttblid, unsigned int rtindex, const rofl::cdptid& dptid,
			uint8_t out_ofp_table_id);

	/**
	 *
	 */
	croute_in4(
			const croute_in4& dptroute) { *this = dptroute; };

	/**
	 *
	 */
	croute_in4&
	operator= (
			const croute_in4& dptroute) {
		if (this == &dptroute)
			return *this;
		croute::operator= (dptroute);
		return *this;
	};

	/*
	 *
	 */
	virtual
	~croute_in4() {
		try {
			if (STATE_ATTACHED == state) {
				handle_dpt_close(rofl::crofdpt::get_dpt(dptid));
			}
		} catch (rofl::eRofDptNotFound& e) {}
	};


public:

	/**
	 *
	 */
	void
	handle_dpt_open(rofl::crofdpt& dpt);

	/**
	 *
	 */
	void
	handle_dpt_close(rofl::crofdpt& dpt);

public:

	friend std::ostream&
	operator<< (std::ostream& os, const croute_in4& route) {
		os << rofcore::indent(0) << "<croute_in4 >" << std::endl;
		rofcore::indent i(2);
		os << dynamic_cast<const croute&>(route);
		try {
			const rofcore::crtroute_in4& rtroute =
						rofcore::cnetlink::get_instance().
								get_routes_in4(route.get_rttblid()).get_route(route.get_rtindex());
			rofcore::indent i(2); os << rtroute;
		} catch (rofcore::crtlink::eRtLinkNotFound& e) {
		} catch (rofcore::crtroute::eRtRouteNotFound& e) {
		}
		rofcore::indent j(2);
		for (std::map<unsigned int, cnexthop_in4>::const_iterator
				it = route.nexthops_in4.begin(); it != route.nexthops_in4.end(); ++it) {
			os << it->second;
		}
		return os;
	};
};



class croute_in6 : public croute {
public:

	/**
	 *
	 */
	croute_in6() {};

	/**
	 *
	 */
	croute_in6(
			uint8_t rttblid, unsigned int rtindex, const rofl::cdptid& dptid,
			uint8_t out_ofp_table_id);

	/**
	 *
	 */
	croute_in6(
			const croute_in6& dptroute) { *this = dptroute; };

	/**
	 *
	 */
	croute_in6&
	operator= (
			const croute_in6& dptroute) {
		if (this == &dptroute)
			return *this;
		croute::operator= (dptroute);
		return *this;
	};

	/*
	 *
	 */
	virtual
	~croute_in6() {
		try {
			if (STATE_ATTACHED == state) {
				handle_dpt_close(rofl::crofdpt::get_dpt(dptid));
			}
		} catch (rofl::eRofDptNotFound& e) {}
	};

public:

	/**
	 *
	 */
	void
	handle_dpt_open(rofl::crofdpt& dpt);

	/**
	 *
	 */
	void
	handle_dpt_close(rofl::crofdpt& dpt);


public:

	friend std::ostream&
	operator<< (std::ostream& os, const croute_in6& route) {
		os << rofcore::indent(0) << "<croute_in6 >" << std::endl;
		rofcore::indent i(2);
		os << dynamic_cast<const croute&>(route);
		try {
			const rofcore::crtroute_in6& rtroute =
						rofcore::cnetlink::get_instance().
								get_routes_in6(route.get_rttblid()).get_route(route.get_rtindex());
			rofcore::indent i(2); os << rtroute;
		} catch (rofcore::crtlink::eRtLinkNotFound& e) {
		} catch (rofcore::crtroute::eRtRouteNotFound& e) {
		}
		rofcore::indent j(2);
		for (std::map<unsigned int, cnexthop_in6>::const_iterator
				it = route.nexthops_in6.begin(); it != route.nexthops_in6.end(); ++it) {
			os << it->second;
		}
		return os;
	};
};

};

#endif /* CROUTE_HPP_ */
