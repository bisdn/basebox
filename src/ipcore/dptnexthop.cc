/*
 * dptnexthop.cc
 *
 *  Created on: 03.07.2013
 *      Author: andreas
 */

#include <dptnexthop.h>

using namespace dptmap;


dptnexthop::dptnexthop() :
		rofbase(0),
		dpt(0),
		of_port_no(0),
		of_table_id(0),
		ifindex(0),
		nbindex(0),
		fe(OFP12_VERSION),
		dstaddr(AF_INET),
		dstmask(AF_INET)
{

}




dptnexthop::~dptnexthop()
{

}



dptnexthop::dptnexthop(
		dptnexthop const& neigh) :
		rofbase(0),
		dpt(0),
		of_port_no(0),
		of_table_id(0),
		ifindex(0),
		nbindex(0),
		fe(OFP12_VERSION),
		dstaddr(AF_INET),
		dstmask(AF_INET)
{
	*this = neigh;
}



dptnexthop&
dptnexthop::operator= (
		dptnexthop const& neigh)
{
	if (this == &neigh)
		return *this;

	rofbase		= neigh.rofbase;
	dpt			= neigh.dpt;
	of_port_no	= neigh.of_port_no;
	of_table_id	= neigh.of_table_id;
	ifindex		= neigh.ifindex;
	nbindex		= neigh.nbindex;
	fe			= neigh.fe;
	dstaddr		= neigh.dstaddr;
	dstmask		= neigh.dstmask;

	return *this;
}



dptnexthop::dptnexthop(
		rofl::crofbase *rofbase,
		rofl::crofdpt* dpt,
		uint32_t of_port_no,
		uint8_t of_table_id,
		int ifindex,
		uint16_t nbindex,
		rofl::caddress const& dstaddr,
		rofl::caddress const& dstmask) :
		rofbase(rofbase),
		dpt(dpt),
		of_port_no(of_port_no),
		of_table_id(of_table_id),
		ifindex(ifindex),
		nbindex(nbindex),
		fe(dpt->get_version()),
		dstaddr(dstaddr),
		dstmask(dstmask)
{

}



void
dptnexthop::open()
{
	flow_mod_add();
}


void
dptnexthop::close()
{
	flow_mod_delete();
}



void
dptnexthop::flow_mod_add()
{
	try {
		crtneigh& rtn = cnetlink::get_instance().get_link(ifindex).get_neigh(nbindex);

		this->fe = rofl::openflow::cofflowmod(dpt->get_version());

		fe.set_command(rofl::openflow::OFPFC_ADD);
		fe.set_buffer_id(rofl::openflow::OFP_NO_BUFFER);
		fe.set_idle_timeout(0);
		fe.set_hard_timeout(0);
		fe.set_priority(0xfffe);
		fe.set_table_id(of_table_id);
		fe.set_flags(rofl::openflow12::OFPFF_SEND_FLOW_REM);

		switch (dstaddr.ca_saddr->sa_family) {
		case AF_INET:  {
			fe.match.set_eth_type(rofl::fipv4frame::IPV4_ETHER);
			fe.match.set_ipv4_dst(dstaddr, dstmask);
		} break;
		case AF_INET6: {
			fe.match.set_eth_type(rofl::fipv6frame::IPV6_ETHER);
			fe.match.set_ipv6_dst(dstaddr, dstmask);
		} break;
		}

		rofl::cmacaddr eth_src(cnetlink::get_instance().get_link(ifindex).get_hwaddr());
		rofl::cmacaddr eth_dst(cnetlink::get_instance().get_link(ifindex).get_neigh(nbindex).get_lladdr());

		fe.instructions.add_inst_apply_actions().get_actions().append_action_set_field(rofl::openflow::coxmatch_ofb_eth_src(eth_src));
		fe.instructions.set_inst_apply_actions().get_actions().append_action_set_field(rofl::openflow::coxmatch_ofb_eth_dst(eth_dst));
		fe.instructions.set_inst_apply_actions().get_actions().append_action_output(of_port_no);

		dpt->send_flow_mod_message(fe);

		//std::cerr << "dptnexthop::flow_mod_add() => " << *this << std::endl;

	} catch (eRtLinkNotFound& e) {
		fprintf(stderr, "dptnexthop::flow_mod_add() unable to find link or neighbor\n");
	}
}



void
dptnexthop::flow_mod_modify()
{
	// TODO
}



void
dptnexthop::flow_mod_delete()
{
	this->fe = rofl::openflow::cofflowmod(dpt->get_version());

	fe.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
	fe.set_buffer_id(rofl::openflow::OFP_NO_BUFFER);
	fe.set_idle_timeout(0);
	fe.set_hard_timeout(0);
	fe.set_priority(0xfffe);
	fe.set_table_id(of_table_id);

	switch (dstaddr.ca_saddr->sa_family) {
	case AF_INET:  {
		fe.match.set_eth_type(rofl::fipv4frame::IPV4_ETHER);
		fe.match.set_ipv4_dst(dstaddr, dstmask);
	} break;
	case AF_INET6: {
		fe.match.set_eth_type(rofl::fipv6frame::IPV6_ETHER);
		fe.match.set_ipv6_dst(dstaddr, dstmask);
	} break;
	}

	dpt->send_flow_mod_message(fe);

	//std::cerr << "dptnexthop::flow_mod_delete() => " << *this << std::endl;
}



rofl::caddress
dptnexthop::get_gateway() const
{
	return cnetlink::get_instance().get_link(ifindex).get_neigh(nbindex).get_dst();
}



