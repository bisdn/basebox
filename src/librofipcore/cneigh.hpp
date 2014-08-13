/*
 * cdptneigh.h
 *
 *  Created on: 03.07.2013
 *      Author: andreas
 */

#ifndef CNEIGH_HPP_
#define CNEIGH_HPP_ 1

#include <ostream>

#ifdef __cplusplus
extern "C" {
#endif
#include <inttypes.h>
#ifdef __cplusplus
}
#endif

#include <rofl/common/crofbase.h>
#include <rofl/common/crofdpt.h>
#include <rofl/common/openflow/cofflowmod.h>
#include <rofl/common/cdptid.h>

#include "crtneigh.h"
#include "cnetlink.h"

namespace ipcore {

class cneigh {
public:


	/**
	 *
	 */
	cneigh() :
		state(STATE_DETACHED), ifindex(0), nbindex(0), table_id(0), of_port_no(0) {};


	/**
	 *
	 */
	virtual
	~cneigh() {};


	/**
	 *
	 */
	cneigh(
			cneigh const& neigh) { *this = neigh; };


	/**
	 *
	 */
	cneigh&
	operator= (
			cneigh const& neigh) {
		if (this == &neigh)
			return *this;
		state		= neigh.state;
		dptid	 	= neigh.dptid;
		of_port_no 	= neigh.of_port_no;
		ifindex	 	= neigh.ifindex;
		nbindex	 	= neigh.nbindex;
		table_id 	= neigh.table_id;
		return *this;
	};


	/**
	 *
	 */
	cneigh(
			int ifindex,
			uint16_t nbindex,
			const rofl::cdptid& dptid,
			uint8_t table_id,
			uint32_t of_port_no) :
				state(STATE_DETACHED),
				ifindex(ifindex),
				nbindex(nbindex),
				dptid(dptid),
				table_id(table_id),
				of_port_no(of_port_no) {};

public:

	/**
	 *
	 */
	const rofl::cdptid&
	get_dptid() const { return dptid; };

	/**
	 *
	 */
	void
	set_dptid(const rofl::cdptid& dptid) { this->dptid = dptid; };

	/**
	 *
	 */
	uint32_t
	get_ofp_port_no() const { return of_port_no; };

	/**
	 *
	 */
	void
	set_ofp_port_no(uint32_t ofp_port_no) { this->of_port_no = ofp_port_no; };

	/**
	 *
	 */
	uint8_t
	get_table_id() const { return table_id; };

	/**
	 *
	 */
	void
	set_table_id(uint8_t table_id) { this->table_id = table_id; };

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
	get_nbindex() const { return nbindex; };

	/**
	 *
	 */
	void
	set_nbindex(uint16_t nbindex) { this->nbindex = nbindex; };

public:

	friend std::ostream&
	operator<< (std::ostream& os, cneigh const& neigh) {
		os << rofl::indent(0) << "<cdptneigh ";
		os << "nbindex: " << (unsigned int)neigh.nbindex << " ";
		os << "ifindex: " << (unsigned int)neigh.ifindex << " ";
		os << "tableid: " << (unsigned int)neigh.table_id << " ";
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
	uint16_t					nbindex;
	rofl::cdptid				dptid;
	uint8_t 					table_id;
	uint32_t					of_port_no;
};


class cneigh_in4 : public cneigh {
public:

	/**
	 *
	 */
	cneigh_in4() {};

	/**
	 *
	 */
	cneigh_in4(
			int ifindex, uint16_t nbindex, const rofl::cdptid& dptid, uint8_t table_id, uint32_t of_port_no) :
				cneigh(ifindex, nbindex, dptid, table_id, of_port_no) {};

	/**
	 *
	 */
	cneigh_in4(
			const cneigh_in4& neigh) { *this = neigh; };

	/**
	 *
	 */
	cneigh_in4&
	operator= (
			const cneigh_in4& dptneigh) {
		if (this == &dptneigh)
			return *this;
		cneigh::operator= (dptneigh);
		return *this;
	};

	/**
	 *
	 */
	~cneigh_in4() {
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
	const rofcore::crtneigh_in4&
	get_crtneigh_in4() const {
		return rofcore::cnetlink::get_instance().get_links().
				get_link(get_ifindex()).get_neighs_in4().get_neigh(get_nbindex());
	};

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
	operator<< (std::ostream& os, const cneigh_in4& neigh) {
		os << rofcore::indent(0) << "<cdptneigh_in4 >" << std::endl;
		rofcore::indent i(2);


		return os;
	};
};


class cneigh_in4_find_by_dst {
	rofl::caddress_in4 dst;
public:
	cneigh_in4_find_by_dst(const rofl::caddress_in4& dst) :
		dst(dst) {};
	bool operator() (const std::pair<unsigned int, cneigh_in4>& p) const {
		return (p.second.get_crtneigh_in4().get_dst() == dst);
	};
};


class cneigh_in6 : public cneigh {
public:

	/**
	 *
	 */
	cneigh_in6() {};

	/**
	 *
	 */
	cneigh_in6(
			int ifindex, uint16_t nbindex, const rofl::cdptid& dptid, uint8_t table_id, uint32_t of_port_no) :
				cneigh(ifindex, nbindex, dptid, table_id, of_port_no) {};

	/**
	 *
	 */
	cneigh_in6(
			const cneigh_in6& neigh) { *this = neigh; };

	/**
	 *
	 */
	cneigh_in6&
	operator= (
			const cneigh_in6& dptneigh) {
		if (this == &dptneigh)
			return *this;
		cneigh::operator= (dptneigh);
		return *this;
	};

	/**
	 *
	 */
	~cneigh_in6() {
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
	const rofcore::crtneigh_in6&
	get_crtneigh_in6() const {
		return rofcore::cnetlink::get_instance().get_links().
				get_link(get_ifindex()).get_neighs_in6().get_neigh(get_nbindex());
	};

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
	operator<< (std::ostream& os, const cneigh_in6& neigh) {
		os << rofcore::indent(0) << "<cdptneigh_in6 >" << std::endl;
		rofcore::indent i(2);


		return os;
	};
};


class cneigh_in6_find_by_dst {
	rofl::caddress_in6 dst;
public:
	cneigh_in6_find_by_dst(const rofl::caddress_in6& dst) :
		dst(dst) {};
	bool operator() (const std::pair<unsigned int, cneigh_in6>& p) const {
		return (p.second.get_crtneigh_in6().get_dst() == dst);
	};
};


}; // end of namespace

#endif /* CNEIGH_HPP_ */


