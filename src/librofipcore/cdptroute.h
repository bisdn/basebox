/*
 * cdptroute.h
 *
 *  Created on: 02.07.2013
 *      Author: andreas
 */

#ifndef DPTROUTE_H_
#define DPTROUTE_H_ 1

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
#include "cnexthoptable.h"

namespace ipcore {

class cdptroute : public flowmod {
public:

	/**
	 *
	 */
	cdptroute() : rttblid(0), rtindex(0) {};

	/**
	 *
	 */
	cdptroute(
			uint8_t rttblid, unsigned int rtindex, const rofl::cdptid& dptid) :
				dptid(dptid), rttblid(rttblid), rtindex(rtindex) {};

	/**
	 *
	 */
	cdptroute(const cdptroute& dptroute) { *this = dptroute; };

	/**
	 *
	 */
	cdptroute&
	operator= (const cdptroute& dptroute) {
		if (this == &dptroute)
			return *this;
		dptid			= dptroute.dptid;
		rttblid			= dptroute.rttblid;
		rtindex			= dptroute.rtindex;
		nexthoptable	= dptroute.nexthoptable;
		return *this;
	};

	/**
	 *
	 */
	virtual
	~cdptroute() {};

public:

	/**
	 *
	 */
	virtual void
	update() { reinstall(); };

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
	const cnexthoptable&
	get_nexthop_table() const { return nexthoptable; };

	/**
	 *
	 */
	cnexthoptable&
	set_nexthop_table() { return nexthoptable; };

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

protected:


	/**
	 *
	 */
	virtual void
	flow_mod_add(
			uint8_t command = rofl::openflow::OFPFC_ADD) {};

	/**
	 *
	 */
	virtual void
	flow_mod_delete() {};



public:

	/**
	 *
	 */
	friend std::ostream&
	operator<< (std::ostream& os, cdptroute const& route) {
		os << rofcore::indent(0) << "<cdptroute ";
		os << "rttblid:" << (unsigned int)route.get_rttblid() << " ";
		os << "rtindex:" << route.get_rtindex() << " ";
		os << " >" << std::endl;
		rofcore::indent i(2);
		os << route.get_nexthop_table();
		return os;
	};

private:

	rofl::cdptid					dptid;
	uint8_t					 		rttblid;
	unsigned int			 		rtindex;

	/* we make here one assumption: only one nexthop exists per neighbor and route
	 * this should be valid under all circumstances
	 */
	cnexthoptable 					nexthoptable;
};



class cdptroute_in4 : public cdptroute {
public:

	/**
	 *
	 */
	cdptroute_in4() {};

	/**
	 *
	 */
	cdptroute_in4(
			uint8_t rttblid, unsigned int rtindex, const rofl::cdptid& dptid);

	/**
	 *
	 */
	cdptroute_in4(
			const cdptroute_in4& dptroute) { *this = dptroute; };

	/**
	 *
	 */
	cdptroute_in4&
	operator= (
			const cdptroute_in4& dptroute) {
		if (this == &dptroute)
			return *this;
		cdptroute::operator= (dptroute);
		return *this;
	};

	/*
	 *
	 */
	virtual
	~cdptroute_in4() {};


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
	operator<< (std::ostream& os, const cdptroute_in4& dptroute) {
		const rofcore::crtroute& rtroute =
				rofcore::cnetlink::get_instance().
							get_routes_in4(dptroute.get_rttblid()).get_route(dptroute.get_rtindex());
			os << rofl::indent(0) << "<dptroute_in4 >" 	<< std::endl;
			rofl::indent i(2);
			os << dynamic_cast<const cdptroute&>(dptroute);
			os << rtroute;
		return os;
	};
};



class cdptroute_in6 : public cdptroute {
public:

	/**
	 *
	 */
	cdptroute_in6() {};

	/**
	 *
	 */
	cdptroute_in6(
			uint8_t rttblid, unsigned int rtindex, const rofl::cdptid& dptid);

	/**
	 *
	 */
	cdptroute_in6(
			const cdptroute_in6& dptroute) { *this = dptroute; };

	/**
	 *
	 */
	cdptroute_in6&
	operator= (
			const cdptroute_in6& dptroute) {
		if (this == &dptroute)
			return *this;
		cdptroute::operator= (dptroute);
		return *this;
	};

	/*
	 *
	 */
	virtual
	~cdptroute_in6() {};


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
	operator<< (std::ostream& os, const cdptroute_in6& dptroute) {
		const rofcore::crtroute& rtroute =
				rofcore::cnetlink::get_instance().
							get_routes_in6(dptroute.get_rttblid()).get_route(dptroute.get_rtindex());
			os << rofl::indent(0) << "<dptroute_in6 >" 	<< std::endl;
			rofl::indent i(2);
			os << dynamic_cast<const cdptroute&>(dptroute);
			os << rtroute;
		return os;
	};
};

};

#endif /* CDPTROUTE_H_ */
