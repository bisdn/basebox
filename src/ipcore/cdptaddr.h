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

#include "crtaddr.h"
#include "flowmod.h"
#include "cnetlink.h"
#include "clogging.h"

namespace ipcore
{

class cdptaddr : public flowmod {
public:

	/**
	 *
	 */
	cdptaddr() :
		table_id(0), ifindex(0), adindex(0) {};

	/**
	 *
	 */
	virtual
	~cdptaddr() {};

	/**
	 *
	 */
	cdptaddr(
			cdptaddr const& addr) { *this = addr; };

	/**
	 *
	 */
	cdptaddr&
	operator= (
			cdptaddr const& addr) {
		if (this == &addr)
			return *this;
		dptid	 = addr.dptid;
		table_id = addr.table_id;
		ifindex	 = addr.ifindex;
		adindex	 = addr.adindex;
		return *this;
	};

	/**
	 *
	 */
	cdptaddr(
			const rofl::cdptid& dptid, int ifindex, uint16_t adindex, uint8_t table_id) :
				dptid(dptid),
				table_id(table_id),
				ifindex(ifindex),
				adindex(adindex) {};

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
	void
	set_dptid(const rofl::cdptid& dptid) { this->dptid = dptid; };

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
	get_table_id() const { return table_id; };

	/**
	 *
	 */
	void
	set_table_id(uint8_t table_id) { this->table_id = table_id; };

protected:


	/**
	 *
	 */
	virtual void
	flow_mod_add(
			uint8_t command = rofl::openflow::OFPFC_ADD) = 0;

	/**
	 *
	 */
	virtual void
	flow_mod_delete() = 0;


public:

	friend std::ostream&
	operator<< (std::ostream& os, cdptaddr const& addr) {
		os << rofl::indent(0) << "<cdptaddr ";
		os << "adindex: " << (unsigned int)addr.adindex << " ";
		os << "ifindex: " << (unsigned int)addr.ifindex << " ";
		os << "tableid: " << (unsigned int)addr.table_id << " ";
		os << ">" << std::endl;
		return os;
	};

protected:

	rofl::cdptid				dptid;
	uint8_t						table_id;
	int							ifindex;
	uint16_t					adindex;
};



class cdptaddr_in4 : public cdptaddr {
public:

	/**
	 *
	 */
	cdptaddr_in4() {};

	/**
	 *
	 */
	cdptaddr_in4(
			const rofl::cdptid& dptid, int ifindex, uint16_t adindex, uint8_t table_id) :
				cdptaddr(dptid, ifindex, adindex, table_id) {};

	/**
	 *
	 */
	cdptaddr_in4(
			const cdptaddr_in4& addr) { *this = addr; };

	/**
	 *
	 */
	cdptaddr_in4&
	operator= (
			const cdptaddr_in4& dptaddr) {
		if (this == &dptaddr)
			return *this;
		cdptaddr::operator= (dptaddr);
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
	operator<< (std::ostream& os, const cdptaddr_in4& addr) {
		os << rofcore::indent(0) << "<cdptaddr_in4 >" << std::endl;
		rofcore::indent i(2);


		return os;
	};
};



class cdptaddr_in6 : public cdptaddr {
public:

	/**
	 *
	 */
	cdptaddr_in6() {};

	/**
	 *
	 */
	cdptaddr_in6(
			const rofl::cdptid& dptid, int ifindex, uint16_t adindex, uint8_t table_id) :
				cdptaddr(dptid, ifindex, adindex, table_id) {};

	/**
	 *
	 */
	cdptaddr_in6(
			const cdptaddr_in6& addr) { *this = addr; };

	/**
	 *
	 */
	cdptaddr_in6&
	operator= (
			const cdptaddr_in6& dptaddr) {
		if (this == &dptaddr)
			return *this;
		cdptaddr::operator= (dptaddr);
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
	operator<< (std::ostream& os, const cdptaddr_in6& addr) {
		os << rofcore::indent(0) << "<cdptaddr_in6 >" << std::endl;
		rofcore::indent i(2);


		return os;
	};
};




}; // end of namespace

#endif /* DPTADDR_H_ */
