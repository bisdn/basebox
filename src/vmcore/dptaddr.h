/*
 * dptaddr.h
 *
 *  Created on: 03.07.2013
 *      Author: andreas
 */

#ifndef DPTADDR_H_
#define DPTADDR_H_ 1

#include <rofl/common/crofbase.h>
#include <rofl/common/logging.h>
#include <rofl/common/crofdpt.h>
#include <rofl/common/openflow/cofflowmod.h>

#include "crtaddr.h"
#include "flowmod.h"
#include "cnetlink.h"

namespace dptmap
{

class dptaddr :
		public flowmod
{
private:

	rofl::crofbase*		rofbase;
	rofl::crofdpt*		dpt;
	uint8_t				of_table_id;
	int					ifindex;
	uint16_t			adindex;
	rofl::cofflowmod	fe;

public:


	/**
	 *
	 */
	dptaddr();


	/**
	 *
	 */
	virtual
	~dptaddr();


	/**
	 *
	 */
	dptaddr(
			dptaddr const& addr);


	/**
	 *
	 */
	dptaddr&
	operator= (
			dptaddr const& addr);


	/**
	 *
	 */
	dptaddr(
			rofl::crofbase *rofbase, rofl::crofdpt *dpt, int ifindex, uint16_t adindex);


	/**
	 *
	 */
	void
	open();


	/**
	 *
	 */
	void
	close();


public:


	/**
	 *
	 */
	int get_ifindex() const { return ifindex; };


	/**
	 *
	 */
	uint16_t get_adindex() const { return adindex; };


	/**
	 *
	 */
	rofl::cofflowmod get_flowentry() const { return fe; };


private:


	/**
	 *
	 */
	virtual void flow_mod_add();


	/**
	 *
	 */
	virtual void flow_mod_modify();


	/**
	 *
	 */
	virtual void flow_mod_delete();


public:


	/**
	 *
	 */
	friend std::ostream&
	operator<< (std::ostream& os, dptaddr const& addr) {
		try {
			crtaddr& rta = cnetlink::get_instance().get_link(addr.ifindex).get_addr(addr.adindex);

			os << rofl::indent(0) << "<dptaddr: adindex: " << (unsigned int)addr.adindex
					<< " oftableid: " << (unsigned int)addr.of_table_id << " >" << std::endl;

			os << rofl::indent(2) << "<family: " << rta.get_family_s() << " "
					<< " prefix:" << rta.get_prefixlen() << " devname:"
					<< cnetlink::get_instance().get_link(addr.ifindex).get_devname() << " >" << std::endl;

			{ os << rofl::indent(2) << "<local address: " << rta.get_local_addr() << " >" << std::endl; }
			{ os << rofl::indent(2) << "<crtaddr: >" << std::endl; rofl::indent i(2); os
											<< cnetlink::get_instance().get_link(addr.ifindex).get_addr(addr.adindex); }
			//{ os << rofl::indent(2) << "<flow-entry: >" << std::endl; rofl::indent(2); os << addr.fe; }

		} catch (eRtLinkNotFound& e) {
			os << rofl::indent(0) << "<dptaddr: adindex: " << (unsigned int)addr.adindex
					<< " oftableid: " << (unsigned int)addr.of_table_id << " >" << std::endl;
			os << rofl::indent(2) << "<associated crtaddr object not found >" << std::endl;

		} catch (eNetLinkNotFound& e) {
			os << rofl::indent(0) << "<dptaddr: adindex: " << (unsigned int)addr.adindex
					<< " oftableid: " << (unsigned int)addr.of_table_id << " >" << std::endl;
			os << rofl::indent(2) << "<associated crtlink object not found >";
		}
		return os;
	};
};

}; // end of namespace

#endif /* DPTADDR_H_ */
