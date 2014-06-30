/*
 * cdptneigh.h
 *
 *  Created on: 03.07.2013
 *      Author: andreas
 */

#ifndef CDPTNEIGH_H_
#define CDPTNEIGH_H_ 1

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
#include "flowmod.h"
#include "cnetlink.h"

namespace ipcore
{

class cdptneigh : public flowmod {
public:


	/**
	 *
	 */
	cdptneigh() :
		of_port_no(0), ifindex(0), nbindex(0), table_id(0) {};


	/**
	 *
	 */
	virtual
	~cdptneigh() {};


	/**
	 *
	 */
	cdptneigh(
			cdptneigh const& neigh) { *this = neigh; };


	/**
	 *
	 */
	cdptneigh&
	operator= (
			cdptneigh const& neigh) {
		if (this == &neigh)
			return *this;
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
	cdptneigh(
			const rofl::cdptid& dptid,
			uint32_t of_port_no,
			int ifindex,
			uint16_t nbindex,
			uint8_t table_id) :
				dptid(dptid),
				of_port_no(of_port_no),
				ifindex(ifindex),
				nbindex(nbindex),
				table_id(table_id) {};

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

	friend std::ostream&
	operator<< (std::ostream& os, cdptneigh const& neigh) {
		os << rofl::indent(0) << "<cdptneigh ";
		os << "nbindex: " << (unsigned int)neigh.nbindex << " ";
		os << "ifindex: " << (unsigned int)neigh.ifindex << " ";
		os << "tableid: " << (unsigned int)neigh.table_id << " ";
		os << ">" << std::endl;
		return os;
	};

private:

	rofl::cdptid				dptid;
	uint32_t					of_port_no;
	int							ifindex;
	uint16_t					nbindex;
	uint8_t 					table_id;
};


class cdptneigh_in4 : public cdptneigh {
public:

	/**
	 *
	 */
	cdptneigh_in4() {};

	/**
	 *
	 */
	cdptneigh_in4(
			const rofl::cdptid& dptid, uint32_t of_port_no, int ifindex, uint16_t nbindex, uint8_t table_id) :
				cdptneigh(dptid, of_port_no, ifindex, nbindex, table_id) {};

	/**
	 *
	 */
	cdptneigh_in4(
			const cdptneigh_in4& neigh) { *this = neigh; };

	/**
	 *
	 */
	cdptneigh_in4&
	operator= (
			const cdptneigh_in4& dptneigh) {
		if (this == &dptneigh)
			return *this;
		cdptneigh::operator= (dptneigh);
		return *this;
	};

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
	operator<< (std::ostream& os, const cdptneigh_in4& neigh) {
		os << rofcore::indent(0) << "<cdptneigh_in4 >" << std::endl;
		rofcore::indent i(2);


		return os;
	};
};



class cdptneigh_in6 : public cdptneigh {
public:

	/**
	 *
	 */
	cdptneigh_in6() {};

	/**
	 *
	 */
	cdptneigh_in6(
			const rofl::cdptid& dptid, uint32_t of_port_no, int ifindex, uint16_t nbindex, uint8_t table_id) :
				cdptneigh(dptid, of_port_no, ifindex, nbindex, table_id) {};

	/**
	 *
	 */
	cdptneigh_in6(
			const cdptneigh_in6& neigh) { *this = neigh; };

	/**
	 *
	 */
	cdptneigh_in6&
	operator= (
			const cdptneigh_in6& dptneigh) {
		if (this == &dptneigh)
			return *this;
		cdptneigh::operator= (dptneigh);
		return *this;
	};

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
	operator<< (std::ostream& os, const cdptneigh_in6& neigh) {
		os << rofcore::indent(0) << "<cdptneigh_in6 >" << std::endl;
		rofcore::indent i(2);


		return os;
	};
};





}; // end of namespace

#endif /* DPTNEIGH_H_ */


