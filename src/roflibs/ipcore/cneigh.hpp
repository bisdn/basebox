/*
 * cneigh.hpp
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
#include <rofl/common/cdpid.h>
#include <rofl/common/protocols/fvlanframe.h>

#include "roflibs/netlink/crtneigh.hpp"
#include "roflibs/netlink/cnetlink.hpp"
#include "roflibs/netlink/ccookiebox.hpp"

namespace roflibs {
namespace ip {

class cneigh {
public:


	/**
	 *
	 */
	cneigh() :
		state(STATE_DETACHED), ifindex(0), nbindex(0), out_ofp_table_id(0), group_id(0) {};


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
		state				= neigh.state;
		dpid	 			= neigh.dpid;
		ifindex	 			= neigh.ifindex;
		nbindex	 			= neigh.nbindex;
		out_ofp_table_id 	= neigh.out_ofp_table_id;
		group_id			= neigh.group_id;
		return *this;
	};


	/**
	 *
	 */
	cneigh(
			int ifindex,
			uint16_t nbindex,
			const rofl::cdpid& dpid,
			uint8_t out_ofp_table_id) :
				state(STATE_DETACHED),
				ifindex(ifindex),
				nbindex(nbindex),
				dpid(dpid),
				out_ofp_table_id(out_ofp_table_id),
				group_id(0) {

		try {

			rofcore::cnetlink& netlink = rofcore::cnetlink::get_instance();

			// the neighbour ...
			const rofcore::crtneigh_in4& rtn =
					netlink.get_links().get_link(get_ifindex()).get_neighs_in4().get_neigh(get_nbindex());

			// neighbour mac address
			lladdr = rtn.get_lladdr();

		} catch (rofl::eRofDptNotFound& e) {
			rofcore::logging::error << "[roflibs][ipcore][cneigh_in4][cneigh] dpt not found" << std::endl;
		} catch (rofcore::eNetLinkNotFound& e) {
			rofcore::logging::error << "[roflibs][ipcore][cneigh_in4][cneigh] unable to find link" << e.what() << std::endl;
		} catch (rofcore::crtneigh::eRtNeighNotFound& e) {
			rofcore::logging::error << "[roflibs][ipcore][cneigh_in4][cneigh] unable to find neighbour" << e.what() << std::endl;
		} catch (rofcore::crtlink::eRtLinkNotFound& e) {
			rofcore::logging::error << "[roflibs][ipcore][cneigh_in4][cneigh] unable to find address" << e.what() << std::endl;
		}
	};

	/**
	 *
	 */
	virtual void
	update() = 0;

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
	uint8_t
	get_table_id() const { return out_ofp_table_id; };

	/**
	 *
	 */
	void
	set_table_id(uint8_t table_id) { this->out_ofp_table_id = table_id; };

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

	/**
	 *
	 */
	uint32_t
	get_group_id() const { return group_id; };

	/**
	 *
	 */
	const rofl::cmacaddr&
	get_lladdr() const { return lladdr; };

public:

	friend std::ostream&
	operator<< (std::ostream& os, cneigh const& neigh) {
		os << rofl::indent(0) << "<cneigh ";
		os << "nbindex: " << (unsigned int)neigh.nbindex << " ";
		os << "ifindex: " << (unsigned int)neigh.ifindex << " ";
		os << "tableid: " << (unsigned int)neigh.out_ofp_table_id << " ";
		os << "lladdr: " << neigh.lladdr.str() << " ";
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
	rofl::cdpid					dpid;
	uint8_t 					out_ofp_table_id;
	uint32_t					group_id;
	rofl::caddress_ll			lladdr;
};


class cneigh_in4 :
		public cneigh,
		public roflibs::common::openflow::ccookie_owner {
public:

	/**
	 *
	 */
	cneigh_in4() :
		cookie(roflibs::common::openflow::ccookie_owner::acquire_cookie())
	{};

	/**
	 *
	 */
	cneigh_in4(
			int ifindex, uint16_t nbindex, const rofl::cdpid& dpid, uint8_t out_ofp_table_id) :
				cneigh(ifindex, nbindex, dpid, out_ofp_table_id),
				cookie(roflibs::common::openflow::ccookie_owner::acquire_cookie())
	{};

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
			const cneigh_in4& neigh) {
		if (this == &neigh)
			return *this;
		cneigh::operator= (neigh);
		return *this;
	};

	/**
	 *
	 */
	~cneigh_in4() {
		try {
			if (STATE_ATTACHED == state) {
				handle_dpt_close(rofl::crofdpt::get_dpt(dpid));
			}
		} catch (rofl::eRofDptNotFound& e) {}
	};

	/**
	 *
	 */
	virtual void
	update();

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

	/**
	 *
	 */
	virtual void
	handle_packet_in(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_packet_in& msg) {};

	/**
	 *
	 */
	virtual void
	handle_flow_removed(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_flow_removed& msg) {};

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cneigh_in4& neigh) {
		os << rofcore::indent(0) << "<cneigh_in4 >" << std::endl;
		try {
			rofcore::cnetlink& netlink = rofcore::cnetlink::get_instance();

			// the neighbour ...
			const rofcore::crtneigh_in4& rtn =
					netlink.get_links().get_link(neigh.get_ifindex()).get_neighs_in4().get_neigh(neigh.get_nbindex());

			os << rofcore::indent(2) << "<" << rtn.get_dst().str() << " at " << rtn.get_lladdr().str() << " >" << std::endl;
		} catch(...) {}
		return os;
	};

private:

	uint64_t	cookie;
};


class cneigh_in4_find_by_dst {
	rofl::caddress_in4 dst;
public:
	cneigh_in4_find_by_dst(const rofl::caddress_in4& dst) :
		dst(dst) {};
	bool operator() (const std::pair<unsigned int, cneigh_in4*>& p) const {
		return (p.second->get_crtneigh_in4().get_dst() == dst);
	};
};


class cneigh_in6 :
		public cneigh,
		public roflibs::common::openflow::ccookie_owner {
public:

	/**
	 *
	 */
	cneigh_in6() :
		cookie(roflibs::common::openflow::ccookie_owner::acquire_cookie())
	{};

	/**
	 *
	 */
	cneigh_in6(
			int ifindex, uint16_t nbindex, const rofl::cdpid& dpid, uint8_t out_ofp_table_id) :
				cneigh(ifindex, nbindex, dpid, out_ofp_table_id),
				cookie(roflibs::common::openflow::ccookie_owner::acquire_cookie())
	{};

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
			const cneigh_in6& neigh) {
		if (this == &neigh)
			return *this;
		cneigh::operator= (neigh);
		return *this;
	};

	/**
	 *
	 */
	~cneigh_in6() {
		try {
			if (STATE_ATTACHED == state) {
				handle_dpt_close(rofl::crofdpt::get_dpt(dpid));
			}
		} catch (rofl::eRofDptNotFound& e) {}
	};

	/**
	 *
	 */
	virtual void
	update();

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

	/**
	 *
	 */
	virtual void
	handle_packet_in(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_packet_in& msg) {};

	/**
	 *
	 */
	virtual void
	handle_flow_removed(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_flow_removed& msg) {};

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cneigh_in6& neigh) {
		os << rofcore::indent(0) << "<cneigh_in6 >" << std::endl;
		try {
			rofcore::cnetlink& netlink = rofcore::cnetlink::get_instance();

			// the neighbour ...
			const rofcore::crtneigh_in6& rtn =
					netlink.get_links().get_link(neigh.get_ifindex()).get_neighs_in6().get_neigh(neigh.get_nbindex());

			os << rofcore::indent(2) << "<" << rtn.get_dst().str() << " at " << rtn.get_lladdr().str() << " >" << std::endl;
		} catch(...) {}
		return os;
	};

private:

	uint64_t	cookie;
};


class cneigh_in6_find_by_dst {
	rofl::caddress_in6 dst;
public:
	cneigh_in6_find_by_dst(const rofl::caddress_in6& dst) :
		dst(dst) {};
	bool operator() (const std::pair<unsigned int, cneigh_in6*>& p) const {
		return (p.second->get_crtneigh_in6().get_dst() == dst);
	};
};

}; // end of namespace ip
}; // end of namespace roflibs

#endif /* CNEIGH_HPP_ */


