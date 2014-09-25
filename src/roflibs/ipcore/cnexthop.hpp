/*
 * cdptnexthop.h
 *
 *  Created on: 03.07.2013
 *      Author: andreas
 */

#ifndef DPTNEXTHOP_H_
#define DPTNEXTHOP_H_ 1

#include <ostream>
#include <exception>

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

#include <roflibs/netlink/cnetlink.hpp>

namespace roflibs {
namespace ip {

class eNextHopBase : public std::runtime_error {
public:
	eNextHopBase(const std::string& __arg) : std::runtime_error(__arg) {};
};
class eNextHopNotFound : public eNextHopBase {
public:
	eNextHopNotFound(const std::string& __arg) : eNextHopBase(__arg) {};
};

class cnexthop : public rofcore::cnetlink_neighbour_observer {
public:


	/**
	 *
	 */
	cnexthop() :
				state(STATE_DETACHED), rttblid(0), rtindex(0), nhindex(0), out_ofp_table_id(2) {};


	/**
	 *
	 */
	virtual
	~cnexthop() {};

	/**
	 *
	 */
	cnexthop(
			cnexthop const& nexthop) { *this = nexthop; };

	/**
	 *
	 */
	cnexthop&
	operator= (
			cnexthop const& nexthop) {
		if (this == &nexthop)
			return *this;
		state				= nexthop.state;
		dpid	 			= nexthop.dpid;
		rttblid 			= nexthop.rttblid;
		rtindex	 			= nexthop.rtindex;
		nhindex	 			= nexthop.nhindex;
		out_ofp_table_id 	= nexthop.out_ofp_table_id;
		return *this;
	};

	/**
	 *
	 */
	cnexthop(
			uint8_t rttableid, unsigned int rtindex, unsigned int nhindex, const rofl::cdpid& dpid, uint8_t out_ofp_table_id = 2) :
				state(STATE_DETACHED),
				dpid(dpid),
				rttblid(rttableid),
				rtindex(rtindex),
				nhindex(nhindex),
				out_ofp_table_id(out_ofp_table_id) {};

public:

	/**
	 *
	 */
	const rofl::cdpid&
	get_dpid() const { return dpid; };

	/**
	 *
	 */
	uint8_t
	get_rttblid() const { return rttblid; };

	/**
	 *
	 */
	unsigned int
	get_rtindex() const { return rtindex; };

	/**
	 *
	 */
	unsigned int
	get_nhindex() const { return nhindex; };

public:


	/**
	 *
	 */
	friend std::ostream&
	operator<< (std::ostream& os, const cnexthop& nexthop) {
		os << rofcore::indent(0) << "<cnexthop "
				<< "rttblid: " << (int)nexthop.rttblid << " "
				<< "rtindex: " << nexthop.rtindex << " "
				<< "nhindex: " << nexthop.nhindex << " "
				<< " >" << std::endl;
		return os;
	};

protected:

	enum ofp_state_t {
		STATE_DETACHED = 1,
		STATE_ATTACHED = 2,
	};

	enum ofp_state_t			state;
	rofl::cdpid				dpid;
	uint8_t						rttblid; // routing table id, not OFP related
	unsigned int				rtindex;
	unsigned int				nhindex;
	uint8_t						out_ofp_table_id;
};




class cnexthop_in4 : public cnexthop {
public:

	/**
	 *
	 */
	cnexthop_in4() {};

	/**
	 *
	 */
	cnexthop_in4(
			const cnexthop_in4& nexthop) { *this = nexthop; };

	/**
	 *
	 */
	cnexthop_in4&
	operator= (
			const cnexthop_in4& nexthop) {
		if (this == &nexthop)
			return *this;
		cnexthop::operator= (nexthop);
		return *this;
	};

	/**
	 *
	 */
	cnexthop_in4(
			uint8_t rttblid, unsigned int rtindex, unsigned int nhindex, const rofl::cdpid& dpid, uint8_t out_ofp_table_id = 2) :
				cnexthop(rttblid, rtindex, nhindex, dpid, out_ofp_table_id) {};

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

private:

	virtual void
	neigh_in4_created(unsigned int ifindex, uint16_t nbindex) {
		if (STATE_DETACHED == state) {
			handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
		}
	};

	virtual void
	neigh_in4_updated(unsigned int ifindex, uint16_t nbindex) {
		handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
	};

	virtual void
	neigh_in4_deleted(unsigned int ifindex, uint16_t nbindex) {
		if (STATE_ATTACHED == state) {
			handle_dpt_close(rofl::crofdpt::get_dpt(dpid));
		}
	};

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cnexthop_in4& nexthop) {
		try {
			os << rofcore::indent(0) << "<cnexthop_in4 >" 	<< std::endl;
			const rofcore::crtnexthop_in4& rtn =
					rofcore::cnetlink::get_instance().get_routes_in4(nexthop.get_rttblid()).
						get_route(nexthop.get_rtindex()).get_nexthops_in4().get_nexthop(nexthop.get_nhindex());
			rofcore::indent i(2); os << rtn;
		} catch (rofcore::crtroute::eRtRouteNotFound& e) {
		} catch (rofcore::crtnexthop::eRtNextHopNotFound& e) {
		}
		return os;
	};

protected:

};





class cnexthop_in6 : public cnexthop {
public:

	/**
	 *
	 */
	cnexthop_in6() {};

	/**
	 *
	 */
	cnexthop_in6(
			const cnexthop_in6& nexthop) { *this = nexthop; };

	/**
	 *
	 */
	cnexthop_in6&
	operator= (
			const cnexthop_in6& nexthop) {
		if (this == &nexthop)
			return *this;
		cnexthop::operator= (nexthop);
		return *this;
	};

	/**
	 *
	 */
	cnexthop_in6(
			uint8_t rttableid, unsigned int rtindex, unsigned int nhindex, const rofl::cdpid& dpid, uint8_t out_ofp_table_id = 2) :
				cnexthop(rttableid, rtindex, nhindex, dpid, out_ofp_table_id) {};


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

private:

	virtual void
	neigh_in6_created(unsigned int ifindex, uint16_t nbindex) {
		if (STATE_DETACHED == state) {
			handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
		}
	};

	virtual void
	neigh_in6_updated(unsigned int ifindex, uint16_t nbindex) {
		handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
	};

	virtual void
	neigh_in6_deleted(unsigned int ifindex, uint16_t nbindex) {
		if (STATE_ATTACHED == state) {
			handle_dpt_close(rofl::crofdpt::get_dpt(dpid));
		}
	};

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cnexthop_in6& nexthop) {
		try {
			os << rofcore::indent(0) << "<cnexthop_in6 >" 	<< std::endl;
			const rofcore::crtnexthop_in6& rtn =
					rofcore::cnetlink::get_instance().get_routes_in6(nexthop.get_rttblid()).
						get_route(nexthop.get_rtindex()).get_nexthops_in6().get_nexthop(nexthop.get_nhindex());
			rofcore::indent i(2); os << rtn;
		} catch (rofcore::crtroute::eRtRouteNotFound& e) {
		} catch (rofcore::crtnexthop::eRtNextHopNotFound& e) {
		}
		return os;
	};
};

}; // end of namespace ip
}; // end of namespace roflibs

#endif /* DPTNEIGH_H_ */


