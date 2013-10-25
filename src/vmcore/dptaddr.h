/*
 * dptaddr.h
 *
 *  Created on: 03.07.2013
 *      Author: andreas
 */

#ifndef DPTADDR_H_
#define DPTADDR_H_ 1

#include <rofl/common/crofbase.h>
#include <rofl/common/openflow/cofdpt.h>
#include <rofl/common/openflow/cflowentry.h>

#include <crtaddr.h>
#include <flowmod.h>
#include <cnetlink.h>

namespace dptmap
{

class dptaddr :
		public flowmod
{
private:

	rofl::crofbase*		rofbase;
	rofl::cofdpt*		dpt;
	uint8_t				of_table_id;
	int					ifindex;
	uint16_t			adindex;
	rofl::cflowentry	fe;

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
			rofl::crofbase *rofbase, rofl::cofdpt *dpt, int ifindex, uint16_t adindex);


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
	rofl::cflowentry get_flowentry() const { return fe; };


public:


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
	operator<< (std::ostream& os, dptaddr const& addr)
	{
#if 0
		rofl::cflowentry fe(addr.fe);
		char s_fe[1024];
		memset(s_fe, 0, sizeof(s_fe));
		snprintf(s_fe, sizeof(s_fe)-1, "%s", fe.c_str());
#endif
		crtaddr& rta = cnetlink::get_instance().get_link(addr.ifindex).get_addr(addr.adindex);

		os << "<dptaddr: ";
			//os << "ifindex=" << addr.ifindex << " ";
			os << "adindex=" << (unsigned int)addr.adindex << " ";
			os << rta.get_family() << " " << rta.get_local_addr() << "/" << rta.get_prefixlen() << " dev " << cnetlink::get_instance().get_link(addr.ifindex).get_devname();
			os << "oftableid=" << (unsigned int)addr.of_table_id << " ";
			//os << "flowentry=" << s_fe << " ";
		os << ">";
		return os;
	};
};

}; // end of namespace

#endif /* DPTADDR_H_ */
