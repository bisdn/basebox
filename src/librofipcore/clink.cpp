/*
 * cdptport.cc
 *
 *  Created on: 28.06.2013
 *      Author: andreas
 */

#include "clink.hpp"

using namespace ipcore;




clink::clink() :
		state(STATE_DETACHED),
		ifindex(0),
		in_ofp_table_id(3),
		out_ofp_table_id(4),
		tagged(false),
		vid(0xffff)
{

}



clink::clink(
		const rofl::cdptid& dptid,
		int ifindex,
		const std::string& devname,
		const rofl::caddress_ll& hwaddr,
		uint8_t in_ofp_table_id,
		uint8_t out_ofp_table_id,
		bool tagged,
		uint16_t vid) :
				state(STATE_DETACHED),
				devname(devname),
				hwaddr(hwaddr),
				ifindex(ifindex),
				dptid(dptid),
				in_ofp_table_id(in_ofp_table_id),
				out_ofp_table_id(out_ofp_table_id),
				tagged(tagged),
				vid(vid)
{

}



clink::~clink()
{
	try {
		if (STATE_ATTACHED == state) {
			handle_dpt_close(rofl::crofdpt::get_dpt(dptid));
		}
	} catch (rofl::eRofDptNotFound& e) {};
}



void
clink::handle_dpt_open(rofl::crofdpt& dpt)
{
	try {
		state = STATE_ATTACHED;

		for (std::map<unsigned int, caddr_in4*>::iterator
				it = addrs_in4.begin(); it != addrs_in4.end(); ++it) {
			it->second->handle_dpt_open(dpt);
		}
		for (std::map<unsigned int, caddr_in6*>::iterator
				it = addrs_in6.begin(); it != addrs_in6.end(); ++it) {
			it->second->handle_dpt_open(dpt);
		}
		for (std::map<unsigned int, cneigh_in4*>::iterator
				it = neighs_in4.begin(); it != neighs_in4.end(); ++it) {
			it->second->handle_dpt_open(dpt);
		}
		for (std::map<unsigned int, cneigh_in6*>::iterator
				it = neighs_in6.begin(); it != neighs_in6.end(); ++it) {
			it->second->handle_dpt_open(dpt);
		}

	} catch (rofl::eRofBaseNotConnected& e) {

	} catch (rofl::eRofSockTxAgain& e) {

	}
}



void
clink::handle_dpt_close(
		rofl::crofdpt& dpt)
{
	try {
		state = STATE_DETACHED;

		for (std::map<unsigned int, caddr_in4*>::iterator
				it = addrs_in4.begin(); it != addrs_in4.end(); ++it) {
			it->second->handle_dpt_close(dpt);
		}
		for (std::map<unsigned int, caddr_in6*>::iterator
				it = addrs_in6.begin(); it != addrs_in6.end(); ++it) {
			it->second->handle_dpt_close(dpt);
		}
		for (std::map<unsigned int, cneigh_in4*>::iterator
				it = neighs_in4.begin(); it != neighs_in4.end(); ++it) {
			it->second->handle_dpt_close(dpt);
		}
		for (std::map<unsigned int, cneigh_in6*>::iterator
				it = neighs_in6.begin(); it != neighs_in6.end(); ++it) {
			it->second->handle_dpt_close(dpt);
		}

	} catch (rofl::eRofBaseNotConnected& e) {

	} catch (rofl::eRofSockTxAgain& e) {

	}
}



