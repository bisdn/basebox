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
#include <rofl/common/crofdpt.h>
#include <rofl/common/openflow/cofflowmod.h>

#include "crtneigh.h"
#include "flowmod.h"
#include "cnetlink.h"

namespace dptmap
{

class dptneigh :
		public flowmod
{
private:

	rofl::crofbase				*rofbase;
	rofl::crofdpt				*dpt;
	uint32_t					of_port_no;
	uint8_t 					of_table_id;
	int							ifindex;
	uint16_t					nbindex;
	rofl::cofflowmod			fe;

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
			rofl::crofdpt* dpt,
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
	rofl::cofflowmod get_flowentry() const { return fe; };


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
	operator<< (std::ostream& os, dptneigh const& neigh) {
		os << "<dptneigh: nbindex: " << (unsigned int)neigh.nbindex << " table-id: " << (unsigned int)neigh.of_table_id << " >" << std::endl;
		try {
			crtneigh& rtn = cnetlink::get_instance().get_link(neigh.ifindex).get_neigh(neigh.nbindex);
			os << rofl::indent(2) << "<dst:" << rtn.get_dst()
					<< " devname:" << cnetlink::get_instance().get_link(neigh.ifindex).get_devname()
					<< " lladdr:" << rtn.get_lladdr() << " state " << rtn.get_state_s() << " >" << std::endl;

		} catch (eRtLinkNotFound& e) {
			os << rofl::indent(2) << "<associated crtneigh object not found >";
		} catch (eNetLinkNotFound& e) {
			os << rofl::indent(2) << "<associated crtlink object not found >";
		}
		return os;
	};
};

}; // end of namespace

#endif /* DPTNEIGH_H_ */


