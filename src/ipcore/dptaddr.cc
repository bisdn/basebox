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
		of_table_id(0), // FIXME
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
		of_table_id(0),
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
	of_table_id = addr.of_table_id;
	ifindex		= addr.ifindex;
	adindex		= addr.adindex;
	fe			= addr.fe;

	return *this;
}



dptaddr::dptaddr(
			rofl::crofbase* rofbase, rofl::crofdpt* dpt, int ifindex, uint16_t adindex) :
		rofbase(rofbase),
		dpt(dpt),
		of_table_id(0),
		ifindex(ifindex),
		adindex(adindex),
		fe(dpt->get_version())
{

}



void
dptaddr::open()
{
	flow_mod_add();
}



void
dptaddr::close()
{
	flow_mod_delete();
}



void
dptaddr::flow_mod_add()
{
	try {
		crtaddr& rta = cnetlink::get_instance().get_link(ifindex).get_addr(adindex);

		this->fe = rofl::openflow::cofflowmod(dpt->get_version());

		fe.set_command(rofl::openflow::OFPFC_ADD);
		fe.set_buffer_id(rofl::openflow::base::get_ofp_no_buffer(dpt->get_version()));
		fe.set_idle_timeout(0);
		fe.set_hard_timeout(0);
		fe.set_priority(0xfffe);
		fe.set_table_id(of_table_id);			// FIXME: check for first table-id in data path
		fe.set_flags(rofl::openflow12::OFPFF_SEND_FLOW_REM);

		fe.instructions.add_inst_apply_actions().get_actions().append_action_output(
				rofl::openflow::base::get_ofpp_controller_port(dpt->get_version()), 1518);

		switch (rta.get_family()) {
		case AF_INET:  {
			fe.match.set_eth_type(rofl::fipv4frame::IPV4_ETHER);
			fe.match.set_ipv4_dst(rta.get_local_addr());
		} break;
		case AF_INET6: {
			fe.match.set_eth_type(rofl::fipv6frame::IPV6_ETHER);
			fe.match.set_ipv6_dst(rta.get_local_addr());
		} break;
		}

		dpt->send_flow_mod_message(fe);

		//std::cerr << "dptaddr::flow_mod_add() => " << *this << std::endl;

	} catch (eNetLinkNotFound& e) {
		fprintf(stderr, "dptaddr::flow_mod_add() unable to find link\n");
	} catch (eRtLinkNotFound& e) {
		fprintf(stderr, "dptaddr::flow_mod_add() unable to find address\n");
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

		rofl::openflow::cofflowmod fe = this->fe;

		fe.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		fe.set_table_id(of_table_id);			// FIXME: check for first table-id in data path

		switch (rta.get_family()) {
		case AF_INET:  {
			fe.match.set_eth_type(rofl::fipv4frame::IPV4_ETHER);
			fe.match.set_ipv4_dst(rta.get_local_addr());
		} break;
		case AF_INET6: {
			fe.match.set_eth_type(rofl::fipv6frame::IPV6_ETHER);
			fe.match.set_ipv6_dst(rta.get_local_addr());
		} break;
		}

		dpt->send_flow_mod_message(fe);

		//std::cerr << "dptaddr::flow_mod_delete() => " << *this << std::endl;

	} catch (eRtLinkNotFound& e) {
		fprintf(stderr, "dptaddr::flow_mod_delete() unable to find link or address\n");
	}
}


