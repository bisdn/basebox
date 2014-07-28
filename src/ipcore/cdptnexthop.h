/*
 * dptnexthop.h
 *
 *  Created on: 03.07.2013
 *      Author: andreas
 */

#ifndef DPTNEXTHOP_H_
#define DPTNEXTHOP_H_ 1

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

#include "crtneigh.h"
#include "flowmod.h"
#include "cnetlink.h"

namespace ipcore
{

class cdptnexthop : public flowmod {
public:


	/**
	 *
	 */
	cdptnexthop() :
				rttblid(0), rtindex(0), nhindex(0) {};


	/**
	 *
	 */
	virtual
	~cdptnexthop() {};

	/**
	 *
	 */
	cdptnexthop(
			cdptnexthop const& nexthop) { *this = nexthop; };

	/**
	 *
	 */
	cdptnexthop&
	operator= (
			cdptnexthop const& nexthop) {
		if (this == &nexthop)
			return *this;
		dptid	 	= nexthop.dptid;
		rttblid 	= nexthop.rttblid;
		rtindex	 	= nexthop.rtindex;
		nhindex	 	= nexthop.nhindex;
		return *this;
	};

	/**
	 *
	 */
	cdptnexthop(
			uint8_t rttableid, unsigned int rtindex, unsigned int nhindex, const rofl::cdptid& dptid) :
				dptid(dptid),
				rttblid(rttableid),
				rtindex(rtindex),
				nhindex(nhindex) {};

public:

	/**
	 *
	 */
	virtual void
	update() {};

	/**
	 *
	 */
	void
	install() { flow_mod_add(rofl::openflow::OFPFC_ADD); };

	/**
	 *
	 */
	void
	reinstall() { flow_mod_add(rofl::openflow::OFPFC_MODIFY_STRICT); };

	/**
	 *
	 */
	void
	uninstall() { flow_mod_delete(); };


public:

	/**
	 *
	 */
	const rofl::cdptid&
	get_dptid() const { return dptid; };

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

private:

	/**
	 *
	 */
	virtual void
	flow_mod_add(uint8_t command = rofl::openflow::OFPFC_ADD);

	/**
	 *
	 */
	virtual void
	flow_mod_delete();


public:


	/**
	 *
	 */
	friend std::ostream&
	operator<< (std::ostream& os, const cdptnexthop& nexthop) {
		try {
			os << rofl::indent(0) << "<dptnexthop: >" 	<< std::endl;

			const rofcore::crtnexthop_in4& rtn =
					rofcore::cnetlink::get_instance().get_routes_in4(nexthop.get_rttblid()).
						get_route(nexthop.get_rtindex()).get_nexthops_in4().get_nexthop(nexthop.get_nhindex());

			os << rofl::indent(0) << "<dptnexthop: >" 	<< std::endl;
			os << rofl::indent(2) << "<weight: " 	<< rtn.get_weight() 	<< " >" << std::endl;
			os << rofl::indent(2) << "<ifindex: " 	<< rtn.get_ifindex() 	<< " >" << std::endl;
			os << rofl::indent(2) << "<realms: " 	<< rtn.get_realms() 	<< " >" << std::endl;
			os << rofl::indent(2) << "<flags: " 	<< rtn.get_flags() 		<< " >" << std::endl;

		} catch (...) {
			os << "<dptnexthop: ";
				os << "rttableid:" 	<< nexthop.get_rttblid() << " ";
				os << "rtindex:" 	<< nexthop.get_rtindex() 	<< " ";
				os << "nhindex:" 	<< nexthop.get_nhindex() 	<< " ";
			os << ">";
		}
		return os;
	};

private:

	rofl::cdptid				dptid;
	uint8_t						rttblid; // routing table id, not OFP related
	unsigned int				rtindex;
	unsigned int				nhindex;
};




class cdptnexthop_in4 : public cdptnexthop {
public:

	/**
	 *
	 */
	cdptnexthop_in4() {};

	/**
	 *
	 */
	cdptnexthop_in4(
			const cdptnexthop_in4& nexthop) { *this = nexthop; };

	/**
	 *
	 */
	cdptnexthop_in4&
	operator= (
			const cdptnexthop_in4& nexthop) {
		if (this == &nexthop)
			return *this;
		cdptnexthop::operator= (nexthop);
		return *this;
	};

	/**
	 *
	 */
	cdptnexthop_in4(
			uint8_t rttblid, unsigned int rtindex, unsigned int nhindex, const rofl::cdptid& dptid) :
				cdptnexthop(rttblid, rtindex, nhindex, dptid) {};


public:

	/**
	 *
	 */
	virtual void
	update();

protected:

	/**
	 *
	 */
	virtual void
	flow_mod_add(
			uint8_t command = rofl::openflow::OFPFC_ADD);

	/**
	 *
	 */
	virtual void
	flow_mod_delete();

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cdptnexthop_in4& nexthop) {
		try {
			os << rofl::indent(0) << "<dptnexthop_in4 >" 	<< std::endl;

			const rofcore::crtnexthop_in4& rtn =
					rofcore::cnetlink::get_instance().get_routes_in4(nexthop.get_rttblid()).
						get_route(nexthop.get_rtindex()).get_nexthops_in4().get_nexthop(nexthop.get_nhindex());

			os << rofcore::indent(2) << "<gateway: " 	<< rtn.get_gateway()	<< " >" << std::endl;
			os << rofcore::indent(2) << "<weight: " 	<< rtn.get_weight() 	<< " >" << std::endl;
			os << rofcore::indent(2) << "<ifindex: " 	<< rtn.get_ifindex() 	<< " >" << std::endl;
			os << rofcore::indent(2) << "<realms: " 	<< rtn.get_realms() 	<< " >" << std::endl;
			os << rofcore::indent(2) << "<flags: " 		<< rtn.get_flags() 		<< " >" << std::endl;

		} catch (...) {
			os << "<dptnexthop: ";
				os << "rttableid:" 	<< nexthop.get_rttblid() << " ";
				os << "rtindex:" 	<< nexthop.get_rtindex() 	<< " ";
				os << "nhindex:" 	<< nexthop.get_nhindex() 	<< " ";
			os << " in state -invalid- >";
		}

		return os;
	};

};





class cdptnexthop_in6 : public cdptnexthop {
public:

	/**
	 *
	 */
	cdptnexthop_in6() {};

	/**
	 *
	 */
	cdptnexthop_in6(
			const cdptnexthop_in6& nexthop) { *this = nexthop; };

	/**
	 *
	 */
	cdptnexthop_in6&
	operator= (
			const cdptnexthop_in6& nexthop) {
		if (this == &nexthop)
			return *this;
		cdptnexthop::operator= (nexthop);
		return *this;
	};

	/**
	 *
	 */
	cdptnexthop_in6(
			uint8_t rttableid, unsigned int rtindex, unsigned int nhindex, const rofl::cdptid& dptid) :
				cdptnexthop(rttableid, rtindex, nhindex, dptid) {};


public:

	/**
	 *
	 */
	virtual void
	update();

protected:

	/**
	 *
	 */
	virtual void
	flow_mod_add(
			uint8_t command = rofl::openflow::OFPFC_ADD);

	/**
	 *
	 */
	virtual void
	flow_mod_delete();

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cdptnexthop_in6& nexthop) {
		try {
			os << rofl::indent(0) << "<dptnexthop_in6 >" 	<< std::endl;

			const rofcore::crtnexthop_in6& rtn =
					rofcore::cnetlink::get_instance().get_routes_in6(nexthop.get_rttblid()).
						get_route(nexthop.get_rtindex()).get_nexthops_in6().get_nexthop(nexthop.get_nhindex());

			os << rofcore::indent(2) << "<gateway: " 	<< rtn.get_gateway()	<< " >" << std::endl;
			os << rofcore::indent(2) << "<weight: " 	<< rtn.get_weight() 	<< " >" << std::endl;
			os << rofcore::indent(2) << "<ifindex: " 	<< rtn.get_ifindex() 	<< " >" << std::endl;
			os << rofcore::indent(2) << "<realms: " 	<< rtn.get_realms() 	<< " >" << std::endl;
			os << rofcore::indent(2) << "<flags: " 		<< rtn.get_flags() 		<< " >" << std::endl;

		} catch (...) {
			os << "<dptnexthop: ";
				os << "rttableid:" 	<< nexthop.get_rttblid() << " ";
				os << "rtindex:" 	<< nexthop.get_rtindex() 	<< " ";
				os << "nhindex:" 	<< nexthop.get_nhindex() 	<< " ";
			os << " in state -invalid- >";
		}

		return os;
	};

};


}; // end of namespace

#endif /* DPTNEIGH_H_ */


