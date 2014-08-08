/*
 * dptaddr.cc
 *
 *  Created on: 03.07.2013
 *      Author: andreas
 */


#include "cdptaddr.h"

using namespace ipcore;



void
cdptaddr_in4::update()
{
	flow_mod_add(rofl::openflow::OFPFC_MODIFY_STRICT);
}



void
cdptaddr_in4::flow_mod_add(uint8_t command)
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);
		rofl::openflow::cofflowmod fe(dpt.get_version());

		fe.set_command(command);
		fe.set_buffer_id(rofl::openflow::base::get_ofp_no_buffer(dpt.get_version()));
		fe.set_idle_timeout(0);
		fe.set_hard_timeout(0);
		fe.set_priority(0xfffe);
		fe.set_table_id(table_id);			// FIXME: check for first table-id in data path
		fe.set_flags(rofl::openflow13::OFPFF_SEND_FLOW_REM);

		rofl::cindex index(0);

		fe.set_instructions().add_inst_apply_actions().set_actions().
				set_action_output(index).set_port_no(rofl::openflow::base::get_ofpp_controller_port(dpt.get_version()));
		fe.set_instructions().set_inst_apply_actions().set_actions().
				set_action_output(index).set_max_len(1518);

		//fe.set_match().set_in_port(cdptlink::get_link(ifindex).get_ofp_port_no());
		fe.set_match().set_eth_type(rofl::fipv4frame::IPV4_ETHER);
		fe.set_match().set_ipv4_dst(rofcore::cnetlink::get_instance().get_links().get_link(ifindex).get_addrs_in4().get_addr(adindex).get_local_addr());

		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[dptaddr_in4][flow_mod_add] unable to find data path" << std::endl << *this;

	} catch (rofcore::eNetLinkNotFound& e) {
		rofcore::logging::error << "[dptaddr_in4][flow_mod_add] unable to find link" << std::endl << *this;

	} catch (rofcore::crtlink::eRtLinkNotFound& e) {
		rofcore::logging::error << "[dptaddr_in4][flow_mod_add] unable to find address" << std::endl << *this;

	} catch (...) {
		rofcore::logging::error << "[dptaddr_in4][flow_mod_add] unexpected error" << std::endl << *this;

	}
}



void
cdptaddr_in4::flow_mod_delete()
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);
		rofl::openflow::cofflowmod fe(dpt.get_version());

		fe.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		fe.set_priority(0xfffe);
		fe.set_table_id(table_id);			// FIXME: check for first table-id in data path

		//fe.set_match().set_in_port(cdptlink::get_link(ifindex).get_ofp_port_no());
		fe.set_match().set_eth_type(rofl::fipv4frame::IPV4_ETHER);
		fe.set_match().set_ipv4_dst(rofcore::cnetlink::get_instance().get_links().get_link(ifindex).get_addrs_in4().get_addr(adindex).get_local_addr());

		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[dptaddr_in4][flow_mod_delete] unable to find data path" << std::endl << *this;

	} catch (rofcore::eNetLinkNotFound& e) {
		rofcore::logging::error << "[dptaddr_in4][flow_mod_delete] unable to find link" << std::endl << *this;

	} catch (rofcore::crtlink::eRtLinkNotFound& e) {
		rofcore::logging::error << "[dptaddr_in4][flow_mod_delete] unable to find address" << std::endl << *this;

	} catch (...) {
		rofcore::logging::error << "[dptaddr_in4][flow_mod_delete] unexpected error" << std::endl << *this;

	}
}



void
cdptaddr_in6::update()
{
	flow_mod_add(rofl::openflow::OFPFC_MODIFY_STRICT);
}



void
cdptaddr_in6::flow_mod_add(uint8_t command)
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);
		rofl::openflow::cofflowmod fe(dpt.get_version());

		fe.set_command(command);
		fe.set_buffer_id(rofl::openflow::base::get_ofp_no_buffer(dpt.get_version()));
		fe.set_idle_timeout(0);
		fe.set_hard_timeout(0);
		fe.set_priority(0xfffe);
		fe.set_table_id(table_id);			// FIXME: check for first table-id in data path
		fe.set_flags(rofl::openflow13::OFPFF_SEND_FLOW_REM);

		rofl::cindex index(0);

		fe.set_instructions().add_inst_apply_actions().set_actions().
				set_action_output(index).set_port_no(rofl::openflow::base::get_ofpp_controller_port(dpt.get_version()));
		fe.set_instructions().set_inst_apply_actions().set_actions().
				set_action_output(index).set_max_len(1518);

		//fe.set_match().set_in_port(cdptlink::get_link(ifindex).get_ofp_port_no());
		fe.set_match().set_eth_type(rofl::fipv6frame::IPV6_ETHER);
		fe.set_match().set_ipv6_dst(rofcore::cnetlink::get_instance().get_links().get_link(ifindex).get_addrs_in6().get_addr(adindex).get_local_addr());

		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[dptaddr_in6][flow_mod_add] unable to find data path" << std::endl << *this;

	} catch (rofcore::eNetLinkNotFound& e) {
		rofcore::logging::error << "[dptaddr_in6][flow_mod_add] unable to find link" << std::endl << *this;

	} catch (rofcore::crtlink::eRtLinkNotFound& e) {
		rofcore::logging::error << "[dptaddr_in6][flow_mod_add] unable to find address" << std::endl << *this;

	} catch (...) {
		rofcore::logging::error << "[dptaddr_in6][flow_mod_add] unexpected error" << std::endl << *this;

	}
}



void
cdptaddr_in6::flow_mod_delete()
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);
		rofl::openflow::cofflowmod fe(dpt.get_version());

		fe.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		fe.set_priority(0xfffe);
		fe.set_table_id(table_id);			// FIXME: check for first table-id in data path

		//fe.set_match().set_in_port(cdptlink::get_link(ifindex).get_ofp_port_no());
		fe.set_match().set_eth_type(rofl::fipv6frame::IPV6_ETHER);
		fe.set_match().set_ipv6_dst(rofcore::cnetlink::get_instance().get_links().get_link(ifindex).get_addrs_in6().get_addr(adindex).get_local_addr());

		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[dptaddr_in6][flow_mod_delete] unable to find data path" << std::endl << *this;

	} catch (rofcore::eNetLinkNotFound& e) {
		rofcore::logging::error << "[dptaddr_in6][flow_mod_delete] unable to find link" << std::endl << *this;

	} catch (rofcore::crtlink::eRtLinkNotFound& e) {
		rofcore::logging::error << "[dptaddr_in6][flow_mod_delete] unable to find address" << std::endl << *this;

	} catch (...) {
		rofcore::logging::error << "[dptaddr_in6][flow_mod_delete] unexpected error" << std::endl << *this;

	}
}



