/*
 * cdptneigh.cc
 *
 *  Created on: 03.07.2013
 *      Author: andreas
 */

#include "cdptneigh.h"

using namespace ipcore;


void
cdptneigh_in4::update()
{
	rofcore::logging::warn << "[ipcore][cdptneigh_in4][update] not implemented" << std::endl;
}



void
cdptneigh_in4::flow_mod_add(uint8_t command)
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
		fe.set_match().set_eth_type(rofl::fipv4frame::IPV4_ETHER);
		fe.set_match().set_ipv4_dst(rofcore::cnetlink::get_instance().get_link(ifindex).get_neigh_in4(nbindex).get_dst());

		rofl::cmacaddr eth_src(rofcore::cnetlink::get_instance().get_link(ifindex).get_hwaddr());
		rofl::cmacaddr eth_dst(rofcore::cnetlink::get_instance().get_link(ifindex).get_neigh_in4(nbindex).get_lladdr());

		fe.set_instructions().add_inst_apply_actions().set_actions().add_action_set_field(0).set_oxm(rofl::openflow::coxmatch_ofb_eth_src(eth_src));
		fe.set_instructions().set_inst_apply_actions().set_actions().add_action_set_field(1).set_oxm(rofl::openflow::coxmatch_ofb_eth_dst(eth_dst));
		fe.set_instructions().set_inst_apply_actions().set_actions().add_action_output(2).set_port_no(of_port_no);

		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[dptneigh_in4][flow_mod_add] unable to find data path" << std::endl << *this;

	} catch (rofcore::eNetLinkNotFound& e) {
		rofcore::logging::error << "[dptneigh_in4][flow_mod_add] unable to find link" << std::endl << *this;

	} catch (rofcore::eRtLinkNotFound& e) {
		rofcore::logging::error << "[dptneigh_in4][flow_mod_add] unable to find address" << std::endl << *this;

	} catch (...) {
		rofcore::logging::error << "[dptneigh_in4][flow_mod_add] unexpected error" << std::endl << *this;

	}
}



void
cdptneigh_in4::flow_mod_delete()
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
		fe.set_match().set_ipv4_dst(rofcore::cnetlink::get_instance().get_link(ifindex).get_neigh_in4(nbindex).get_dst());

		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[dptneigh_in4][flow_mod_delete] unable to find data path" << std::endl << *this;

	} catch (rofcore::eNetLinkNotFound& e) {
		rofcore::logging::error << "[dptneigh_in4][flow_mod_delete] unable to find link" << std::endl << *this;

	} catch (rofcore::eRtLinkNotFound& e) {
		rofcore::logging::error << "[dptneigh_in4][flow_mod_delete] unable to find address" << std::endl << *this;

	} catch (...) {
		rofcore::logging::error << "[dptneigh_in4][flow_mod_delete] unexpected error" << std::endl << *this;

	}
}




void
cdptneigh_in6::update()
{
	rofcore::logging::warn << "[ipcore][cdptneigh_in6][update] not implemented" << std::endl;
}



void
cdptneigh_in6::flow_mod_add(uint8_t command)
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
		fe.set_match().set_ipv6_dst(rofcore::cnetlink::get_instance().get_link(ifindex).get_neigh_in6(nbindex).get_dst());

		rofl::cmacaddr eth_src(rofcore::cnetlink::get_instance().get_link(ifindex).get_hwaddr());
		rofl::cmacaddr eth_dst(rofcore::cnetlink::get_instance().get_link(ifindex).get_neigh_in6(nbindex).get_lladdr());

		fe.set_instructions().add_inst_apply_actions().set_actions().add_action_set_field(0).set_oxm(rofl::openflow::coxmatch_ofb_eth_src(eth_src));
		fe.set_instructions().set_inst_apply_actions().set_actions().add_action_set_field(1).set_oxm(rofl::openflow::coxmatch_ofb_eth_dst(eth_dst));
		fe.set_instructions().set_inst_apply_actions().set_actions().add_action_output(2).set_port_no(of_port_no);

		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[dptneigh_in6][flow_mod_add] unable to find data path" << std::endl << *this;

	} catch (rofcore::eNetLinkNotFound& e) {
		rofcore::logging::error << "[dptneigh_in6][flow_mod_add] unable to find link" << std::endl << *this;

	} catch (rofcore::eRtLinkNotFound& e) {
		rofcore::logging::error << "[dptneigh_in6][flow_mod_add] unable to find address" << std::endl << *this;

	} catch (...) {
		rofcore::logging::error << "[dptneigh_in6][flow_mod_add] unexpected error" << std::endl << *this;

	}
}



void
cdptneigh_in6::flow_mod_delete()
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
		fe.set_match().set_ipv6_dst(rofcore::cnetlink::get_instance().get_link(ifindex).get_neigh_in6(nbindex).get_dst());

		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[dptneigh_in6][flow_mod_delete] unable to find data path" << std::endl << *this;

	} catch (rofcore::eNetLinkNotFound& e) {
		rofcore::logging::error << "[dptneigh_in6][flow_mod_delete] unable to find link" << std::endl << *this;

	} catch (rofcore::eRtLinkNotFound& e) {
		rofcore::logging::error << "[dptneigh_in6][flow_mod_delete] unable to find address" << std::endl << *this;

	} catch (...) {
		rofcore::logging::error << "[dptneigh_in6][flow_mod_delete] unexpected error" << std::endl << *this;

	}
}




