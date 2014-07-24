/*
 * dptnexthop.cc
 *
 *  Created on: 03.07.2013
 *      Author: andreas
 */

#include "cdptnexthop.h"

using namespace ipcore;




void
cdptnexthop_in4::update()
{
	rofcore::logging::warn << "[ipcore][cdptnexthop_in4][update] not implemented" << std::endl;
}



void
cdptnexthop_in4::flow_mod_add(uint8_t command)
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);
		rofl::openflow::cofflowmod fe(dpt.get_version());

		fe.set_command(command);
		fe.set_buffer_id(rofl::openflow::base::get_ofp_no_buffer(dpt.get_version()));
		fe.set_idle_timeout(0);
		fe.set_hard_timeout(0);
		fe.set_priority(0xfffe);
		fe.set_table_id(table_id);
		fe.set_flags(rofl::openflow13::OFPFF_SEND_FLOW_REM);

		// TODO: match sollte das Routingprefix sein, nicht nur die Adresse des Gateways!!!

		fe.set_match().set_eth_type(rofl::fipv4frame::IPV4_ETHER);
		fe.set_match().set_ipv4_dst(dstaddr, dstmask);

		rofl::cmacaddr eth_src(rofcore::cnetlink::get_instance().get_links().get_link(ifindex).get_hwaddr());
		rofl::cmacaddr eth_dst(rofcore::cnetlink::get_instance().get_links().get_link(ifindex).get_neighs_in4().get_neigh(nbindex).get_lladdr());

		rofl::cindex index(0);

		fe.set_instructions().add_inst_apply_actions().set_actions().
				add_action_set_field(index++).set_oxm(rofl::openflow::coxmatch_ofb_eth_src(eth_src));
		fe.set_instructions().set_inst_apply_actions().set_actions().
				add_action_set_field(index++).set_oxm(rofl::openflow::coxmatch_ofb_eth_dst(eth_dst));
		fe.set_instructions().set_inst_apply_actions().set_actions().
				add_action_output(index++).set_port_no(ofp_port_no);

		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[dptnexthop_in4][flow_mod_add] unable to find data path" << std::endl << *this;

	} catch (rofcore::eNetLinkNotFound& e) {
		rofcore::logging::error << "[dptnexthop_in4][flow_mod_add] unable to find link" << std::endl << *this;

	} catch (rofcore::crtlink::eRtLinkNotFound& e) {
		rofcore::logging::error << "[dptnexthop_in4][flow_mod_add] unable to find address" << std::endl << *this;

	} catch (...) {
		rofcore::logging::error << "[dptnexthop_in4][flow_mod_add] unexpected error" << std::endl << *this;

	}
}



void
cdptnexthop_in4::flow_mod_delete()
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);
		rofl::openflow::cofflowmod fe(dpt.get_version());

		fe.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		fe.set_buffer_id(rofl::openflow::base::get_ofp_no_buffer(dpt.get_version()));
		fe.set_idle_timeout(0);
		fe.set_hard_timeout(0);
		fe.set_priority(0xfffe);
		fe.set_table_id(table_id);

		/*
		 * TODO: match sollte das Routingprefix sein, nicht nur die Adresse des Gateways!!!
		 */
		fe.set_match().set_eth_type(rofl::fipv4frame::IPV4_ETHER);
		fe.set_match().set_ipv4_dst(rofcore::cnetlink::get_instance().get_links().get_link(ifindex).get_neighs_in.get_ne(nbindex).get_dst());

		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[dptnexthop_in4][flow_mod_delete] unable to find data path" << std::endl << *this;

	} catch (rofcore::eNetLinkNotFound& e) {
		rofcore::logging::error << "[dptnexthop_in4][flow_mod_delete] unable to find link" << std::endl << *this;

	} catch (rofcore::eRtLinkNotFound& e) {
		rofcore::logging::error << "[dptnexthop_in4][flow_mod_delete] unable to find address" << std::endl << *this;

	} catch (...) {
		rofcore::logging::error << "[dptnexthop_in4][flow_mod_delete] unexpected error" << std::endl << *this;

	}
}




void
cdptnexthop_in6::update()
{
	rofcore::logging::warn << "[ipcore][cdptnexthop_in6][update] not implemented" << std::endl;
}



void
cdptnexthop_in6::flow_mod_add(uint8_t command)
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);
		rofl::openflow::cofflowmod fe(dpt.get_version());

		fe.set_command(command);
		fe.set_buffer_id(rofl::openflow::base::get_ofp_no_buffer(dpt.get_version()));
		fe.set_idle_timeout(0);
		fe.set_hard_timeout(0);
		fe.set_priority(0xfffe);
		fe.set_table_id(table_id);
		fe.set_flags(rofl::openflow13::OFPFF_SEND_FLOW_REM);

		/*
		 * TODO: match sollte das Routingprefix sein, nicht nur die Adresse des Gateways!!!
		 */
		fe.set_match().set_eth_type(rofl::fipv6frame::IPV6_ETHER);
		fe.set_match().set_ipv6_dst(rofcore::cnetlink::get_instance().get_routes_in6(table_id).get_route(rtindex).get_ipv6_dst());

		rofl::cmacaddr eth_src(rofcore::cnetlink::get_instance().get_links().get_link(ifindex).get_hwaddr());
		rofl::cmacaddr eth_dst(rofcore::cnetlink::get_instance().get_routes_in6(table_id).get_route(rtindex).get_nxthops_in6().get_nexthop(nbindex).get_lladdr());

		fe.set_instructions().add_inst_apply_actions().set_actions().add_action_set_field(0).set_oxm(rofl::openflow::coxmatch_ofb_eth_src(eth_src));
		fe.set_instructions().set_inst_apply_actions().set_actions().add_action_set_field(1).set_oxm(rofl::openflow::coxmatch_ofb_eth_dst(eth_dst));
		fe.set_instructions().set_inst_apply_actions().set_actions().add_action_output(2).set_port_no(of_port_no);

		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[dptnexthop_in6][flow_mod_add] unable to find data path" << std::endl << *this;

	} catch (rofcore::eNetLinkNotFound& e) {
		rofcore::logging::error << "[dptnexthop_in6][flow_mod_add] unable to find link" << std::endl << *this;

	} catch (rofcore::eRtLinkNotFound& e) {
		rofcore::logging::error << "[dptnexthop_in6][flow_mod_add] unable to find address" << std::endl << *this;

	} catch (...) {
		rofcore::logging::error << "[dptnexthop_in6][flow_mod_add] unexpected error" << std::endl << *this;

	}
}



void
cdptnexthop_in6::flow_mod_delete()
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);
		rofl::openflow::cofflowmod fe(dpt.get_version());

		fe.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		fe.set_buffer_id(rofl::openflow::base::get_ofp_no_buffer(dpt.get_version()));
		fe.set_idle_timeout(0);
		fe.set_hard_timeout(0);
		fe.set_priority(0xfffe);
		fe.set_table_id(table_id);

		/*
		 * TODO: match sollte das Routingprefix sein, nicht nur die Adresse des Gateways!!!
		 */
		fe.set_match().set_eth_type(rofl::fipv6frame::IPV6_ETHER);
		fe.set_match().set_ipv6_dst(rofcore::cnetlink::get_instance().get_link(ifindex).get_nexthop_in6(nbindex).get_dst());

		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[dptnexthop_in6][flow_mod_delete] unable to find data path" << std::endl << *this;

	} catch (rofcore::eNetLinkNotFound& e) {
		rofcore::logging::error << "[dptnexthop_in6][flow_mod_delete] unable to find link" << std::endl << *this;

	} catch (rofcore::eRtLinkNotFound& e) {
		rofcore::logging::error << "[dptnexthop_in6][flow_mod_delete] unable to find address" << std::endl << *this;

	} catch (...) {
		rofcore::logging::error << "[dptnexthop_in6][flow_mod_delete] unexpected error" << std::endl << *this;

	}
}






cdptnexthop::cdptnexthop() :
		rofbase(0),
		dpt(0),
		ofp_port_no(0),
		table_id(0),
		ifindex(0),
		nbindex(0),
		fe(OFP12_VERSION),
		dstaddr(AF_INET),
		dstmask(AF_INET)
{

}




cdptnexthop::~cdptnexthop()
{

}



cdptnexthop::cdptnexthop(
		cdptnexthop const& neigh) :
		rofbase(0),
		dpt(0),
		ofp_port_no(0),
		table_id(0),
		ifindex(0),
		nbindex(0),
		fe(OFP12_VERSION),
		dstaddr(AF_INET),
		dstmask(AF_INET)
{
	*this = neigh;
}



cdptnexthop&
cdptnexthop::operator= (
		cdptnexthop const& neigh)
{
	if (this == &neigh)
		return *this;

	rofbase		= neigh.rofbase;
	dpt			= neigh.dpt;
	ofp_port_no	= neigh.ofp_port_no;
	table_id	= neigh.table_id;
	ifindex		= neigh.ifindex;
	nbindex		= neigh.nbindex;
	fe			= neigh.fe;
	dstaddr		= neigh.dstaddr;
	dstmask		= neigh.dstmask;

	return *this;
}



cdptnexthop::cdptnexthop(
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
		ofp_port_no(of_port_no),
		table_id(of_table_id),
		ifindex(ifindex),
		nbindex(nbindex),
		fe(dpt->get_version()),
		dstaddr(dstaddr),
		dstmask(dstmask)
{

}



void
cdptnexthop::open()
{
	flow_mod_add();
}


void
cdptnexthop::close()
{
	flow_mod_delete();
}



void
cdptnexthop::flow_mod_add()
{
	try {
		crtneigh& rtn = cnetlink::get_instance().get_link(ifindex).get_neigh(nbindex);

		this->fe = rofl::openflow::cofflowmod(dpt->get_version());

		fe.set_command(rofl::openflow::OFPFC_ADD);
		fe.set_buffer_id(rofl::openflow::OFP_NO_BUFFER);
		fe.set_idle_timeout(0);
		fe.set_hard_timeout(0);
		fe.set_priority(0xfffe);
		fe.set_table_id(table_id);
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
		fe.instructions.set_inst_apply_actions().get_actions().append_action_output(ofp_port_no);

		dpt->send_flow_mod_message(fe);

		//std::cerr << "dptnexthop::flow_mod_add() => " << *this << std::endl;

	} catch (eRtLinkNotFound& e) {
		fprintf(stderr, "dptnexthop::flow_mod_add() unable to find link or neighbor\n");
	}
}



void
cdptnexthop::flow_mod_modify()
{
	// TODO
}



void
cdptnexthop::flow_mod_delete()
{
	this->fe = rofl::openflow::cofflowmod(dpt->get_version());

	fe.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
	fe.set_buffer_id(rofl::openflow::OFP_NO_BUFFER);
	fe.set_idle_timeout(0);
	fe.set_hard_timeout(0);
	fe.set_priority(0xfffe);
	fe.set_table_id(table_id);

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
cdptnexthop::get_gateway() const
{
	return cnetlink::get_instance().get_link(ifindex).get_neigh(nbindex).get_dst();
}



