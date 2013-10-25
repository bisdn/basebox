/*
 * dptneigh.h
 *
 *  Created on: 03.07.2013
 *      Author: andreas
 */

#ifndef DPTNEIGH_H_
#define DPTNEIGH_H_ 1

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

class dptneigh :
		public flowmod
{
private:

	rofl::crofbase				*rofbase;
	rofl::cofdpt				*dpt;
	uint32_t					of_port_no;
	uint8_t 					of_table_id;
	int							ifindex;
	uint16_t					nbindex;
	rofl::cflowentry			fe;

public:


	/**
	 *
	 */
	dptneigh();


	/**
	 *
	 */
	virtual
	~dptneigh();


	/**
	 *
	 */
	dptneigh(
			dptneigh const& neigh);


	/**
	 *
	 */
	dptneigh&
	operator= (
			dptneigh const& neigh);


	/**
	 *
	 */
	dptneigh(
			rofl::crofbase *rofbase,
			rofl::cofdpt* dpt,
			uint32_t of_port_no,
			uint8_t of_table_id,
			int ifindex,
			uint16_t nbindex);


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
	operator<< (std::ostream& os, dptneigh const& neigh)
	{
#if 0
		rofl::cflowentry fe(neigh.fe);
		char s_fe[1024];
		memset(s_fe, 0, sizeof(s_fe));
		snprintf(s_fe, sizeof(s_fe)-1, "%s", fe.c_str());
#endif

		os << "<dptneigh ";
			os << "ifindex=" << neigh.ifindex << " ";
			os << "nbindex=" << (unsigned int)neigh.nbindex << " ";
			os << "ofportno=" << (unsigned int)neigh.of_port_no << " ";
			os << "oftableid=" << (unsigned int)neigh.of_table_id << " ";
			//os << "flowentry=" << s_fe << " ";
		os << ">";
		return os;
	};
};

}; // end of namespace

#endif /* DPTNEIGH_H_ */


