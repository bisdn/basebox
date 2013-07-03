/*
 * dptaddr.cc
 *
 *  Created on: 03.07.2013
 *      Author: andreas
 */


#include <dptaddr.h>

using namespace dptmap;



dptaddr::dptaddr() :
		rofbase(0),
		dpt(0),
		ifindex(0),
		adindex(0),
		fe(OFP12_VERSION)
{

}




dptaddr::~dptaddr()
{

}



dptaddr::dptaddr(
			dptaddr const& addr) :
		rofbase(0),
		dpt(0),
		ifindex(0),
		adindex(0),
		fe(OFP12_VERSION)
{
	*this = addr;
}



dptaddr&
dptaddr::operator= (
			dptaddr const& addr)
{
	if (this == &addr)
		return *this;

	rofbase		= addr.rofbase;
	dpt			= addr.dpt;
	ifindex		= addr.ifindex;
	adindex		= addr.adindex;
	fe			= addr.fe;

	return *this;
}



dptaddr::dptaddr(
			rofl::crofbase* rofbase, rofl::cofdpt* dpt, int ifindex, uint16_t adindex) :
		rofbase(rofbase),
		dpt(dpt),
		ifindex(ifindex),
		adindex(adindex),
		fe(dpt->get_version())
{

}



void
dptaddr::flow_mod_add()
{
	try {
		crtaddr& rta = cnetlink::get_instance().get_link(ifindex).get_addr(adindex);

		this->fe = rofl::cflowentry(dpt->get_version());

		fe.set_command(OFPFC_ADD);
		fe.set_buffer_id(OFP_NO_BUFFER);
		fe.set_idle_timeout(0);
		fe.set_hard_timeout(0);
		fe.set_priority(0xfffe);
		fe.set_table_id(0);			// FIXME: check for first table-id in data path

		fe.instructions.next() = rofl::cofinst_apply_actions();
		fe.instructions.back().actions.next() = rofl::cofaction_output(OFPP_CONTROLLER, 1518); // FIXME: check the mtu value

		switch (rta.get_family()) {
		case AF_INET:  { fe.match.set_ipv4_dst(rta.get_local_addr()); } break;
		case AF_INET6: { fe.match.set_ipv6_dst(rta.get_local_addr()); } break;
		}

		rofbase->send_flow_mod_message(dpt, fe);

		std::cerr << "dptaddr::flow_mod_add() => " << *this << std::endl;

	} catch (eRtLinkNotFound& e) {
		fprintf(stderr, "dptaddr::flow_mod_add() unable to find link or address\n");
	}
}



void
dptaddr::flow_mod_modify()
{
	// TODO
}



void
dptaddr::flow_mod_delete()
{
	try {
		crtaddr& rta = cnetlink::get_instance().get_link(ifindex).get_addr(adindex);

		rofl::cflowentry fe(dpt->get_version());

		fe.set_command(OFPFC_DELETE_STRICT);
		fe.set_table_id(0);			// FIXME: check for first table-id in data path

		switch (rta.get_family()) {
		case AF_INET:  { fe.match.set_ipv4_dst(rta.get_local_addr()); } break;
		case AF_INET6: { fe.match.set_ipv6_dst(rta.get_local_addr()); } break;
		}

		rofbase->send_flow_mod_message(dpt, fe);

		std::cerr << "dptaddr::flow_mod_delete() => " << *this << std::endl;

	} catch (eRtLinkNotFound& e) {
		fprintf(stderr, "dptaddr::flow_mod_delete() unable to find link or address\n");
	}
}


