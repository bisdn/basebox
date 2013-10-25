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

#include <rofl/common/crofbase.h>
#include <rofl/common/openflow/cofdpt.h>
#include <rofl/common/openflow/cflowentry.h>

#include <crtneigh.h>
#include <flowmod.h>
#include <cnetlink.h>

namespace dptmap
{

class dptnexthop :
		public flowmod
{
private:

	rofl::crofbase				*rofbase;
	rofl::cofdpt				*dpt;
	uint32_t					of_port_no;
	uint8_t						of_table_id;
	int							ifindex;
	uint16_t					nbindex;
	rofl::cflowentry			fe;
	rofl::caddress				dstaddr; // destination address when acting as a gateway
	rofl::caddress				dstmask; // destination mask when acting as a gateway

public:


	/**
	 *
	 */
	dptnexthop();


	/**
	 *
	 */
	virtual
	~dptnexthop();


	/**
	 *
	 */
	dptnexthop(
			dptnexthop const& neigh);


	/**
	 *
	 */
	dptnexthop&
	operator= (
			dptnexthop const& neigh);


	/**
	 *
	 */
	dptnexthop(
			rofl::crofbase *rofbase,
			rofl::cofdpt* dpt,
			uint32_t of_port_no,
			uint8_t of_table_id,
			int ifindex,
			uint16_t nbindex,
			rofl::caddress const& dstaddr,
			rofl::caddress const& dstmask);


public:


	/**
	 *
	 */
	int get_ifindex() const { return ifindex; };


	/**
	 *
	 */
	uint16_t get_nbindex() const { return nbindex; };


	/**
	 *
	 */
	rofl::cflowentry get_flowentry() const { return fe; };


	/**
	 *
	 */
	rofl::caddress get_dstaddr() const { return dstaddr; };


	/**
	 *
	 */
	rofl::caddress get_dstmask() const { return dstmask; };


	/**
	 *
	 */
	rofl::caddress get_gateway() const;


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
	operator<< (std::ostream& os, dptnexthop const& neigh)
	{
#if 0
		rofl::cflowentry fe(neigh.fe);
		char s_fe[1024];
		memset(s_fe, 0, sizeof(s_fe));
		snprintf(s_fe, sizeof(s_fe)-1, "%s", fe.c_str());
#endif
		os << "dptnexthop{";
			os << "ifindex=" << neigh.ifindex << " ";
			os << "nbindex=" << (unsigned int)neigh.nbindex << " ";
			os << "ofportno=" << (unsigned int)neigh.of_port_no << " ";
			os << "oftableid=" << (unsigned int)neigh.of_table_id << " ";
			os << "dstaddr=" << neigh.dstaddr << " ";
			os << "dstmask=" << neigh.dstmask << " ";
			//os << "flowentry=" << s_fe << " ";
		os << "}";
		return os;
	};
};

}; // end of namespace

#endif /* DPTNEIGH_H_ */


