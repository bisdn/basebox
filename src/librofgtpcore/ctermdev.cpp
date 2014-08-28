/*
 * ctermdev.cpp
 *
 *  Created on: 28.08.2014
 *      Author: andreas
 */

#include "ctermdev.hpp"

using namespace rofgtp;

void
ctermdev::handle_dpt_open(rofl::crofdpt& dpt, const rofcore::cprefix_in4& prefix)
{
	try {
		rofl::openflow::cofflowmod fe(dpt.get_version());

		switch (state) {
		case STATE_DETACHED: {
			fe.set_command(rofl::openflow::OFPFC_ADD);
		} break;
		case STATE_ATTACHED: {
			fe.set_command(rofl::openflow::OFPFC_MODIFY_STRICT);
		} break;
		}

		fe.set_priority(0xd800);
		fe.set_table_id(ofp_table_id);

		fe.set_match().set_eth_type(rofl::fipv4frame::IPV4_ETHER);
		fe.set_match().set_ipv4_dst(prefix.get_addr() & prefix.get_mask(), prefix.get_mask());
		fe.set_instructions().set_inst_apply_actions().set_actions().
				add_action_output(rofl::cindex(0)).set_port_no(rofl::openflow::OFPP_CONTROLLER);
		fe.set_instructions().set_inst_apply_actions().set_actions().
				set_action_output(rofl::cindex(0)).set_max_len(1518);

		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[rofgtp][ctermdev][handle_dpt_open] unable to find data path" << e.what() << std::endl;
	} catch (rofcore::eNetLinkNotFound& e) {
		rofcore::logging::error << "[rofgtp][ctermdev][handle_dpt_open] unable to find link" << e.what() << std::endl;
	} catch (rofcore::crtlink::eRtLinkNotFound& e) {
		rofcore::logging::error << "[rofgtp][ctermdev][handle_dpt_open] unable to find address" << e.what() << std::endl;
	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::error << "[rofgtp][ctermdev][handle_dpt_open] control channel congested" << e.what() << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::error << "[rofgtp][ctermdev][handle_dpt_open] control channel is down" << e.what() << std::endl;
	} catch (...) {
		rofcore::logging::error << "[rofgtp][ctermdev][handle_dpt_open] unexpected error" << std::endl;
	}
}



void
ctermdev::handle_dpt_close(rofl::crofdpt& dpt, const rofcore::cprefix_in4& prefix)
{
	try {
		rofl::openflow::cofflowmod fe(dpt.get_version());

		fe.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		fe.set_priority(0xd800);
		fe.set_table_id(ofp_table_id);

		fe.set_match().set_eth_type(rofl::fipv4frame::IPV4_ETHER);
		fe.set_match().set_ipv4_dst(prefix.get_addr() & prefix.get_mask(), prefix.get_mask());

		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[rofgtp][ctermdev][handle_dpt_close] unable to find data path" << e.what() << std::endl;
	} catch (rofcore::eNetLinkNotFound& e) {
		rofcore::logging::error << "[rofgtp][ctermdev][handle_dpt_close] unable to find link" << e.what() << std::endl;
	} catch (rofcore::crtlink::eRtLinkNotFound& e) {
		rofcore::logging::error << "[rofgtp][ctermdev][handle_dpt_close] unable to find address" << e.what() << std::endl;
	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::error << "[rofgtp][ctermdev][handle_dpt_close] control channel congested" << e.what() << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::error << "[rofgtp][ctermdev][handle_dpt_close] control channel is down" << e.what() << std::endl;
	} catch (...) {
		rofcore::logging::error << "[rofgtp][ctermdev][handle_dpt_close] unexpected error" << std::endl;
	}
}



void
ctermdev::handle_dpt_open(rofl::crofdpt& dpt, const rofcore::cprefix_in6& prefix)
{
	try {
		rofl::openflow::cofflowmod fe(dpt.get_version());

		switch (state) {
		case STATE_DETACHED: {
			fe.set_command(rofl::openflow::OFPFC_ADD);
		} break;
		case STATE_ATTACHED: {
			fe.set_command(rofl::openflow::OFPFC_MODIFY_STRICT);
		} break;
		}

		fe.set_priority(0xd800);
		fe.set_table_id(ofp_table_id);

		fe.set_match().set_eth_type(rofl::fipv6frame::IPV6_ETHER);
		fe.set_match().set_ipv6_dst(prefix.get_addr() & prefix.get_mask(), prefix.get_mask());
		fe.set_instructions().set_inst_apply_actions().set_actions().
				add_action_output(rofl::cindex(0)).set_port_no(rofl::openflow::OFPP_CONTROLLER);
		fe.set_instructions().set_inst_apply_actions().set_actions().
				set_action_output(rofl::cindex(0)).set_max_len(1518);

		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[rofgtp][ctermdev][handle_dpt_open] unable to find data path" << e.what() << std::endl;
	} catch (rofcore::eNetLinkNotFound& e) {
		rofcore::logging::error << "[rofgtp][ctermdev][handle_dpt_open] unable to find link" << e.what() << std::endl;
	} catch (rofcore::crtlink::eRtLinkNotFound& e) {
		rofcore::logging::error << "[rofgtp][ctermdev][handle_dpt_open] unable to find address" << e.what() << std::endl;
	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::error << "[rofgtp][ctermdev][handle_dpt_open] control channel congested" << e.what() << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::error << "[rofgtp][ctermdev][handle_dpt_open] control channel is down" << e.what() << std::endl;
	} catch (...) {
		rofcore::logging::error << "[rofgtp][ctermdev][handle_dpt_open] unexpected error" << std::endl;
	}
}



void
ctermdev::handle_dpt_close(rofl::crofdpt& dpt, const rofcore::cprefix_in6& prefix)
{
	try {
		rofl::openflow::cofflowmod fe(dpt.get_version());

		fe.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		fe.set_priority(0xd800);
		fe.set_table_id(ofp_table_id);

		fe.set_match().set_eth_type(rofl::fipv6frame::IPV6_ETHER);
		fe.set_match().set_ipv6_dst(prefix.get_addr() & prefix.get_mask(), prefix.get_mask());

		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[rofgtp][ctermdev][handle_dpt_close] unable to find data path" << e.what() << std::endl;
	} catch (rofcore::eNetLinkNotFound& e) {
		rofcore::logging::error << "[rofgtp][ctermdev][handle_dpt_close] unable to find link" << e.what() << std::endl;
	} catch (rofcore::crtlink::eRtLinkNotFound& e) {
		rofcore::logging::error << "[rofgtp][ctermdev][handle_dpt_close] unable to find address" << e.what() << std::endl;
	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::error << "[rofgtp][ctermdev][handle_dpt_close] control channel congested" << e.what() << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::error << "[rofgtp][ctermdev][handle_dpt_close] control channel is down" << e.what() << std::endl;
	} catch (...) {
		rofcore::logging::error << "[rofgtp][ctermdev][handle_dpt_close] unexpected error" << std::endl;
	}
}


