/*
 * dptaddr.h
 *
 *  Created on: 03.07.2013
 *      Author: andreas
 */

#ifndef DPTADDR_H_
#define DPTADDR_H_ 1

#include <rofl/common/crofdpt.h>
#include <rofl/common/logging.h>
#include <rofl/common/crofdpt.h>
#include <rofl/common/openflow/cofflowmod.h>
#include <rofl/common/protocols/farpv4frame.h>
#include <rofl/common/protocols/ficmpv4frame.h>
#include <rofl/common/protocols/ficmpv6frame.h>

#include "crtaddr.h"
#include "cnetlink.h"
#include "clogging.h"

namespace rofip {

class caddr {
public:

	/**
	 *
	 */
	caddr() :
		state(STATE_DETACHED), ifindex(0), adindex(0), in_ofp_table_id(0) {};

	/**
	 *
	 */
	caddr(int ifindex, uint16_t adindex, const rofl::cdpid& dpid, uint8_t in_ofp_table_id = 0) :
		state(STATE_DETACHED), ifindex(ifindex), adindex(adindex), dpid(dpid), in_ofp_table_id(in_ofp_table_id) {};

	/**
	 *
	 */
	virtual
	~caddr() {};

	/**
	 *
	 */
	caddr(
			caddr const& addr) { *this = addr; };

	/**
	 *
	 */
	caddr&
	operator= (
			caddr const& addr) {
		if (this == &addr)
			return *this;
		state	 = addr.state;
		dpid	 = addr.dpid;
		in_ofp_table_id = addr.in_ofp_table_id;
		ifindex	 = addr.ifindex;
		adindex	 = addr.adindex;
		return *this;
	};

public:

	/**
	 *
	 */
	const rofl::cdpid&
	get_dpid() const { return dpid; };

	/**
	 *
	 */
	void
	set_dpid(const rofl::cdpid& dpid) { this->dpid = dpid; };

	/**
	 *
	 */
	int
	get_ifindex() const { return ifindex; };

	/**
	 *
	 */
	void
	set_ifindex(int ifindex) { this->ifindex = ifindex; };

	/**
	 *
	 */
	uint16_t
	get_adindex() const { return adindex; };

	/**
	 *
	 */
	void
	set_adindex(uint16_t adindex) { this->adindex = adindex; };

	/**
	 *
	 */
	uint8_t
	get_table_id() const { return in_ofp_table_id; };

	/**
	 *
	 */
	void
	set_table_id(uint8_t table_id) { this->in_ofp_table_id = table_id; };

public:

	friend std::ostream&
	operator<< (std::ostream& os, caddr const& addr) {
		os << rofcore::indent(0) << "<caddr ";
		os << "adindex: " << (unsigned int)addr.adindex << " ";
		os << "ifindex: " << (unsigned int)addr.ifindex << " ";
		os << "tableid: " << (unsigned int)addr.in_ofp_table_id << " ";
		os << ">" << std::endl;
		return os;
	};

protected:

	enum ofp_state_t {
		STATE_DETACHED = 1,
		STATE_ATTACHED = 2,
	};

	enum ofp_state_t			state;
	int							ifindex;
	uint16_t					adindex;
	rofl::cdpid				dpid;
	uint8_t						in_ofp_table_id;

};



class caddr_in4 : public caddr {
public:

	/**
	 *
	 */
	caddr_in4() {};

	/**
	 *
	 */
	caddr_in4(
			int ifindex, uint16_t adindex, const rofl::cdpid& dpid, uint8_t table_id) :
				caddr(ifindex, adindex, dpid, table_id) {};

	/**
	 *
	 */
	caddr_in4(
			const caddr_in4& addr) { *this = addr; };

	/**
	 *
	 */
	caddr_in4&
	operator= (
			const caddr_in4& dptaddr) {
		if (this == &dptaddr)
			return *this;
		caddr::operator= (dptaddr);
		return *this;
	};

	/**
	 *
	 */
	~caddr_in4() {
		try {
			if (STATE_ATTACHED == state) {
				handle_dpt_close(rofl::crofdpt::get_dpt(dpid));
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
	operator<< (std::ostream& os, const caddr_in4& addr) {
		os << rofcore::indent(0) << "<caddr_in4 >" << std::endl;
		rofcore::indent i(2);
		os << dynamic_cast<const caddr&>(addr);
		try {
			const rofcore::crtaddr_in4& rtaddr =
					rofcore::cnetlink::get_instance().get_links().
							get_link(addr.ifindex).get_addrs_in4().get_addr(addr.adindex);
			rofcore::indent i(2); os << rtaddr;
		} catch (rofcore::crtlink::eRtLinkNotFound& e) {
		} catch (rofcore::crtaddr::eRtAddrNotFound& e) {
		}
		return os;
	};
};



class caddr_in6 : public caddr {
public:

	/**
	 *
	 */
	caddr_in6() {};

	/**
	 *
	 */
	caddr_in6(
			int ifindex, uint16_t adindex, const rofl::cdpid& dpid, uint8_t table_id) :
				caddr(ifindex, adindex, dpid, table_id) {};

	/**
	 *
	 */
	caddr_in6(
			const caddr_in6& addr) { *this = addr; };

	/**
	 *
	 */
	caddr_in6&
	operator= (
			const caddr_in6& dptaddr) {
		if (this == &dptaddr)
			return *this;
		caddr::operator= (dptaddr);
		return *this;
	};

	/**
	 *
	 */
	~caddr_in6() {
		try {
			if (STATE_ATTACHED == state) {
				handle_dpt_close(rofl::crofdpt::get_dpt(dpid));
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
	operator<< (std::ostream& os, const caddr_in6& addr) {
		os << rofcore::indent(0) << "<caddr_in6 >" << std::endl;
		rofcore::indent i(2);
		os << dynamic_cast<const caddr&>(addr);
		try {
			const rofcore::crtaddr_in6& rtaddr =
					rofcore::cnetlink::get_instance().get_links().
							get_link(addr.ifindex).get_addrs_in6().get_addr(addr.adindex);
			rofcore::indent i(2); os << rtaddr;
		} catch (rofcore::crtlink::eRtLinkNotFound& e) {
		} catch (rofcore::crtaddr::eRtAddrNotFound& e) {
		}
		return os;
	};
};




}; // end of namespace

#endif /* DPTADDR_H_ */
