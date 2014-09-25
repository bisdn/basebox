/*
 * crelay.cpp
 *
 *  Created on: 19.08.2014
 *      Author: andreas
 */

#include "crelay.hpp"

using namespace roflibs::gtp;

void
crelay_in4::handle_dpt_open(rofl::crofdpt& dpt)
{
	try {
		rofl::openflow::cofflowmod fm(dpt.get_version());

		switch (state) {
		case STATE_DETACHED: {
			fm.set_command(rofl::openflow::OFPFC_ADD);
		} break;
		case STATE_ATTACHED: {
			fm.set_command(rofl::openflow::OFPFC_MODIFY_STRICT);
		} break;
		}

		fm.set_buffer_id(rofl::openflow::OFP_NO_BUFFER);
		fm.set_idle_timeout(idle_timeout);
		fm.set_hard_timeout(0);
		fm.set_priority(0x8000);
		fm.set_table_id(ofp_table_id);
		fm.set_match().set_eth_type(rofl::fipv4frame::IPV4_ETHER);
		fm.set_match().set_ipv4_src(label_in.get_saddr().get_addr());
		fm.set_match().set_ipv4_dst(label_in.get_daddr().get_addr());
		fm.set_match().set_ip_proto(rofl::fudpframe::UDP_IP_PROTO);
		fm.set_match().set_udp_src(label_in.get_saddr().get_port().get_value());
		fm.set_match().set_udp_dst(label_in.get_daddr().get_port().get_value());
		fm.set_match().set_matches().add_match(rofl::openflow::experimental::gtp::coxmatch_ofx_gtp_msg_type(255)); // = G-PDU (255)
		fm.set_match().set_matches().add_match(rofl::openflow::experimental::gtp::coxmatch_ofx_gtp_teid(label_in.get_teid().get_value()));

		rofl::cindex index(0);
		fm.set_instructions().set_inst_apply_actions().set_actions().add_action_set_field(index++).
				set_oxm(rofl::openflow::coxmatch_ofb_ipv4_src(label_out.get_saddr().get_addr()));
		fm.set_instructions().set_inst_apply_actions().set_actions().add_action_set_field(index++).
				set_oxm(rofl::openflow::coxmatch_ofb_ipv4_dst(label_out.get_daddr().get_addr()));
		fm.set_instructions().set_inst_apply_actions().set_actions().add_action_set_field(index++).
				set_oxm(rofl::openflow::coxmatch_ofb_udp_src(label_out.get_saddr().get_port().get_value()));
		fm.set_instructions().set_inst_apply_actions().set_actions().add_action_set_field(index++).
				set_oxm(rofl::openflow::coxmatch_ofb_udp_dst(label_out.get_daddr().get_port().get_value()));
		fm.set_instructions().set_inst_apply_actions().set_actions().add_action_set_field(index++).
				set_oxm(rofl::openflow::experimental::gtp::coxmatch_ofx_gtp_msg_type(255));
		fm.set_instructions().set_inst_apply_actions().set_actions().add_action_set_field(index++).
				set_oxm(rofl::openflow::experimental::gtp::coxmatch_ofx_gtp_teid(label_out.get_teid().get_value()));
		fm.set_instructions().add_inst_goto_table().set_table_id(ofp_table_id + 1);
		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		state = STATE_ATTACHED;

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[crelay_in4][handle_dpt_open] dpt not found" << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::error << "[crelay_in4][handle_dpt_open] control channel is down" << std::endl;
	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::error << "[crelay_in4][handle_dpt_open] control channel congested" << std::endl;
	} catch (rofl::RoflException& e) {
		rofcore::logging::error << "[crelay_in4][handle_dpt_open] unexpected exception caught: " << e.what() << std::endl;
	}
}



void
crelay_in4::handle_dpt_close(rofl::crofdpt& dpt)
{
	try {
		state = STATE_DETACHED;

		rofl::openflow::cofflowmod fm(dpt.get_version());

		fm.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		fm.set_buffer_id(rofl::openflow::OFP_NO_BUFFER);
		fm.set_idle_timeout(idle_timeout);
		fm.set_hard_timeout(0);
		fm.set_priority(0x8000);
		fm.set_table_id(ofp_table_id);
		fm.set_match().set_eth_type(rofl::fipv4frame::IPV4_ETHER);
		fm.set_match().set_ipv4_src(label_in.get_saddr().get_addr());
		fm.set_match().set_ipv4_dst(label_in.get_daddr().get_addr());
		fm.set_match().set_ip_proto(rofl::fudpframe::UDP_IP_PROTO);
		fm.set_match().set_udp_src(label_in.get_saddr().get_port().get_value());
		fm.set_match().set_udp_dst(label_in.get_daddr().get_port().get_value());
		fm.set_match().set_matches().add_match(rofl::openflow::experimental::gtp::coxmatch_ofx_gtp_msg_type(255)); // = G-PDU (255)
		fm.set_match().set_matches().add_match(rofl::openflow::experimental::gtp::coxmatch_ofx_gtp_teid(label_in.get_teid().get_value()));
		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[crelay_in4][handle_dpt_close] dpt not found" << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::error << "[crelay_in4][handle_dpt_close] control channel is down" << std::endl;
	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::error << "[crelay_in4][handle_dpt_close] control channel congested" << std::endl;
	}
}






void
crelay_in6::handle_dpt_open(rofl::crofdpt& dpt)
{
	try {
		rofl::openflow::cofflowmod fm(dpt.get_version());

		switch (state) {
		case STATE_DETACHED: {
			fm.set_command(rofl::openflow::OFPFC_ADD);
		} break;
		case STATE_ATTACHED: {
			fm.set_command(rofl::openflow::OFPFC_MODIFY_STRICT);
		} break;
		}

		fm.set_buffer_id(rofl::openflow::OFP_NO_BUFFER);
		fm.set_idle_timeout(idle_timeout);
		fm.set_hard_timeout(0);
		fm.set_priority(0x8000);
		fm.set_table_id(ofp_table_id);
		fm.set_match().set_eth_type(rofl::fipv4frame::IPV4_ETHER);
		fm.set_match().set_ipv6_src(label_in.get_saddr().get_addr());
		fm.set_match().set_ipv6_dst(label_in.get_daddr().get_addr());
		fm.set_match().set_ip_proto(rofl::fudpframe::UDP_IP_PROTO);
		fm.set_match().set_udp_src(label_in.get_saddr().get_port().get_value());
		fm.set_match().set_udp_dst(label_in.get_daddr().get_port().get_value());
		fm.set_match().set_matches().add_match(rofl::openflow::experimental::gtp::coxmatch_ofx_gtp_msg_type(255)); // = G-PDU (255)
		fm.set_match().set_matches().add_match(rofl::openflow::experimental::gtp::coxmatch_ofx_gtp_teid(label_in.get_teid().get_value()));

		rofl::cindex index(0);
		fm.set_instructions().set_inst_apply_actions().set_actions().add_action_set_field(index++).
				set_oxm(rofl::openflow::coxmatch_ofb_ipv6_src(label_out.get_saddr().get_addr()));
		fm.set_instructions().set_inst_apply_actions().set_actions().add_action_set_field(index++).
				set_oxm(rofl::openflow::coxmatch_ofb_ipv6_dst(label_out.get_daddr().get_addr()));
		fm.set_instructions().set_inst_apply_actions().set_actions().add_action_set_field(index++).
				set_oxm(rofl::openflow::coxmatch_ofb_udp_src(label_out.get_saddr().get_port().get_value()));
		fm.set_instructions().set_inst_apply_actions().set_actions().add_action_set_field(index++).
				set_oxm(rofl::openflow::coxmatch_ofb_udp_dst(label_out.get_daddr().get_port().get_value()));
		fm.set_instructions().set_inst_apply_actions().set_actions().add_action_set_field(index++).
				set_oxm(rofl::openflow::experimental::gtp::coxmatch_ofx_gtp_msg_type(255));
		fm.set_instructions().set_inst_apply_actions().set_actions().add_action_set_field(index++).
				set_oxm(rofl::openflow::experimental::gtp::coxmatch_ofx_gtp_teid(label_out.get_teid().get_value()));
		fm.set_instructions().add_inst_goto_table().set_table_id(ofp_table_id + 1);

		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		state = STATE_ATTACHED;

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[crelay_in6][handle_dpt_open] dpt not found" << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::error << "[crelay_in6][handle_dpt_open] control channel is down" << std::endl;
	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::error << "[crelay_in6][handle_dpt_open] control channel congested" << std::endl;
	}
}



void
crelay_in6::handle_dpt_close(rofl::crofdpt& dpt)
{
	try {
		state = STATE_DETACHED;

		rofl::openflow::cofflowmod fm(dpt.get_version());

		fm.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		fm.set_buffer_id(rofl::openflow::OFP_NO_BUFFER);
		fm.set_idle_timeout(idle_timeout);
		fm.set_hard_timeout(0);
		fm.set_priority(0x8000);
		fm.set_table_id(ofp_table_id);
		fm.set_match().set_eth_type(rofl::fipv4frame::IPV4_ETHER);
		fm.set_match().set_ipv6_src(label_in.get_saddr().get_addr());
		fm.set_match().set_ipv6_dst(label_in.get_daddr().get_addr());
		fm.set_match().set_ip_proto(rofl::fudpframe::UDP_IP_PROTO);
		fm.set_match().set_udp_src(label_in.get_saddr().get_port().get_value());
		fm.set_match().set_udp_src(label_in.get_daddr().get_port().get_value());
		fm.set_match().set_matches().add_match(rofl::openflow::experimental::gtp::coxmatch_ofx_gtp_msg_type(255)); // = G-PDU (255)
		fm.set_match().set_matches().add_match(rofl::openflow::experimental::gtp::coxmatch_ofx_gtp_teid(label_in.get_teid().get_value()));

		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[crelay_in6][handle_dpt_close] dpt not found" << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::error << "[crelay_in6][handle_dpt_close] control channel is down" << std::endl;
	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::error << "[crelay_in6][handle_dpt_close] control channel congested" << std::endl;
	}
}
