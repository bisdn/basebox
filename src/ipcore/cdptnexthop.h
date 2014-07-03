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
		ofp_port_no(0), table_id(0), ifindex(0), nbindex(0) {};


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
		ofp_port_no = nexthop.ofp_port_no;
		table_id 	= nexthop.table_id;
		ifindex	 	= nexthop.ifindex;
		nbindex	 	= nexthop.nbindex;
		return *this;
	};

	/**
	 *
	 */
	cdptnexthop(
			const rofl::cdptid& dptid, uint32_t ofp_port_no, int ifindex, uint16_t nhindex, uint8_t table_id) :
				dptid(dptid),
				ofp_port_no(ofp_port_no),
				table_id(table_id),
				ifindex(ifindex),
				nbindex(nhindex) {};

public:

	/**
	 *
	 */
	virtual void
	update() = 0;

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
	void
	set_dptid(const rofl::cdptid& dptid) { this->dptid = dptid; };

	/**
	 *
	 */
	uint32_t
	get_ofp_port_no() const { return ofp_port_no; };

	/**
	 *
	 */
	void
	set_ofp_port_no(uint32_t ofp_port_no) { this->ofp_port_no = ofp_port_no; };

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
	set_ifindex(int ifinfex) { this->ifindex = ifindex; };

	/**
	 *
	 */
	uint16_t
	get_nbindex() const { return nbindex; };

	/**
	 *
	 */
	void
	set_nbindex(uint16_t nhindex) { this->nbindex = nhindex; };

private:

	/**
	 *
	 */
	virtual void
	flow_mod_add(uint8_t command = rofl::openflow::OFPFC_ADD) = 0;

	/**
	 *
	 */
	virtual void
	flow_mod_delete() = 0;


public:


	/**
	 *
	 */
	friend std::ostream&
	operator<< (std::ostream& os, cdptnexthop const& neigh) {
		try {
			os << rofl::indent(0) << "<dptnexthop: >" 	<< std::endl;

			rofcore::crtneigh_in4& rtn = rofcore::cnetlink::get_instance().get_link(neigh.ifindex).get_neigh_in4(neigh.nbindex);
			os << rofl::indent(0) << "<dptnexthop: >" 	<< std::endl;
			os << rofl::indent(2) << "<destination: " 	<< rtn.get_dst() << " >" << std::endl;
			os << rofl::indent(2) << "<device: " 		<< rofcore::cnetlink::get_instance().get_link(neigh.ifindex).get_devname() << " >" << std::endl;
			os << rofl::indent(2) << "<hwaddr: " 		<< rtn.get_lladdr() << " >" << std::endl;
			os << rofl::indent(2) << "<state: " 		<< rtn.get_state() << " >" << std::endl;
			os << rofl::indent(2) << "<table-id: " 		<< (unsigned int)neigh.table_id << " >" << std::endl;

		} catch (rofcore::eNetLinkNotFound& e) {
			os << "<dptnexthop: ";
				os << "ifindex:" << neigh.ifindex << " ";
				os << "nhindex:" << (unsigned int)neigh.nbindex << " ";
				os << "ofportno:" << (unsigned int)neigh.ofp_port_no << " ";
				os << "oftableid: " << (unsigned int)neigh.table_id << " ";
			os << ">";
		} catch (rofcore::eRtLinkNotFound& e) {
			os << "<dptnexthop: ";
				os << "ifindex:" << neigh.ifindex << " ";
				os << "nhindex:" << (unsigned int)neigh.nbindex << " ";
				os << "ofportno:" << (unsigned int)neigh.ofp_port_no << " ";
				os << "oftableid: " << (unsigned int)neigh.table_id << " ";
			os << ">";
		}
		return os;
	};

private:

	rofl::cdptid				dptid;
	uint32_t					ofp_port_no;
	uint8_t						table_id;
	int							ifindex;
	uint16_t					nbindex;
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
			const rofl::cdptid& dptid, uint32_t ofp_port_no, int ifindex, uint16_t nhindex, uint8_t table_id) :
				cdptnexthop(dptid, ofp_port_no, ifindex, nhindex, table_id) {};

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
		dstaddr = nexthop.dstaddr;
		dstmask = nexthop.dstmask;
		return *this;
	};

public:

	/**
	 *
	 */
	const rofl::caddress_in4&
	get_addr() const { return dstaddr; };

	/**
	 *
	 */
	rofl::caddress_in4&
	set_addr() { return dstaddr; };

	/**
	 *
	 */
	void
	set_addr(const rofl::caddress_in4& dstaddr) { this->dstaddr = dstaddr; };

	/**
	 *
	 */
	const rofl::caddress_in4&
	get_mask() const { return dstmask; };

	/**
	 *
	 */
	rofl::caddress_in4&
	set_mask() { return dstmask; };

	/**
	 *
	 */
	void
	set_mask(const rofl::caddress_in4& dstmask) { this->dstmask = dstmask; };

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
		os << rofcore::indent(0) << "<cdptnexthop_in4 >" << std::endl;
		rofcore::indent i(2);


		return os;
	};

private:

	rofl::caddress_in4	dstaddr; // destination address when acting as a gateway
	rofl::caddress_in4	dstmask; // destination mask when acting as a gateway
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
			const rofl::cdptid& dptid, uint32_t ofp_port_no, int ifindex, uint16_t nhindex, uint8_t table_id) :
				cdptnexthop(dptid, ofp_port_no, ifindex, nhindex, table_id) {};

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
		dstaddr = nexthop.dstaddr;
		dstmask = nexthop.dstmask;
		return *this;
	};

public:

	/**
	 *
	 */
	const rofl::caddress_in6&
	get_addr() const { return dstaddr; };

	/**
	 *
	 */
	rofl::caddress_in6&
	set_addr() { return dstaddr; };

	/**
	 *
	 */
	void
	set_addr(const rofl::caddress_in6& dstaddr) { this->dstaddr = dstaddr; };

	/**
	 *
	 */
	const rofl::caddress_in6&
	get_mask() const { return dstmask; };

	/**
	 *
	 */
	rofl::caddress_in6&
	set_mask() { return dstmask; };

	/**
	 *
	 */
	void
	set_mask(const rofl::caddress_in6& dstmask) { this->dstmask = dstmask; };

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
		os << rofcore::indent(0) << "<cdptnexthop_in6 >" << std::endl;
		rofcore::indent i(2);


		return os;
	};

private:

	rofl::caddress_in6	dstaddr; // destination address when acting as a gateway
	rofl::caddress_in6	dstmask; // destination mask when acting as a gateway
};




}; // end of namespace

#endif /* DPTNEIGH_H_ */


