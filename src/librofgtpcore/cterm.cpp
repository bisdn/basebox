/*
 * cterm.cpp
 *
 *  Created on: 18.08.2014
 *      Author: andreas
 */

#include "cterm.hpp"

using namespace rofgtp;

void
cterm_in4::handle_dpt_open_egress(rofl::crofdpt& dpt)
{
	try {
		rofl::openflow::cofflowmod fm(dpt.get_version());

		if (not flags.test(FLAG_EGRESS_FM_INSTALLED)) {
			fm.set_command(rofl::openflow::OFPFC_ADD);
		} else {
			fm.set_command(rofl::openflow::OFPFC_MODIFY_STRICT);
		}

		fm.set_idle_timeout(idle_timeout);
		fm.set_hard_timeout(0);
		fm.set_priority(0xe000);
		fm.set_table_id(ofp_table_id);

		fm.set_match().set_eth_type(rofl::fipv4frame::IPV4_ETHER);
		fm.set_match().set_ipv4_src(label_egress.get_saddr().get_addr());
		fm.set_match().set_ipv4_dst(label_egress.get_daddr().get_addr());
		fm.set_match().set_ip_proto(rofl::fudpframe::UDP_IP_PROTO);
		fm.set_match().set_udp_src(label_egress.get_saddr().get_port().get_value());
		fm.set_match().set_udp_dst(label_egress.get_daddr().get_port().get_value());
		fm.set_match().set_matches().add_match(rofl::openflow::experimental::gtp::coxmatch_ofx_gtp_msg_type(255)); // = G-PDU (255)
		fm.set_match().set_matches().add_match(rofl::openflow::experimental::gtp::coxmatch_ofx_gtp_teid(label_egress.get_teid().get_value()));

		rofl::cindex index(0);
		// pop header GTP (removes outer IPV4/UDP/GTP)
		fm.set_instructions().add_inst_apply_actions().set_actions().add_action_experimenter(rofl::cindex(0)).
				set_exp_id(rofl::openflow::experimental::gtp::GTP_EXP_ID);
		fm.set_instructions().set_inst_apply_actions().set_actions().set_action_experimenter(rofl::cindex(0)).
				set_exp_body(rofl::openflow::experimental::gtp::cofaction_exp_body_pop_gtp(rofl::fipv4frame::IPV4_ETHER));
		// Goto Next Table ofp_table_id + 1
		fm.set_instructions().add_inst_goto_table().set_table_id(ofp_table_id + 1);

		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		flags.set(FLAG_EGRESS_FM_INSTALLED);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[cterm_in4][handle_dpt_open_egress] dpt not found" << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::error << "[cterm_in4][handle_dpt_open_egress] control channel is down" << std::endl;
	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::error << "[cterm_in4][handle_dpt_open_egress] control channel congested" << std::endl;
	} catch (rofl::RoflException& e) {
		rofcore::logging::error << "[cterm_in4][handle_dpt_open_egress] unexpected exception caught: " << e.what() << std::endl;
	}
}



void
cterm_in4::handle_dpt_close_egress(rofl::crofdpt& dpt)
{
	try {
		rofl::openflow::cofflowmod fm(dpt.get_version());

		fm.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		fm.set_idle_timeout(idle_timeout);
		fm.set_hard_timeout(0);
		fm.set_priority(0xe000);
		fm.set_table_id(ofp_table_id);

		fm.set_match().set_eth_type(rofl::fipv4frame::IPV4_ETHER);
		fm.set_match().set_ipv4_src(label_egress.get_saddr().get_addr());
		fm.set_match().set_ipv4_dst(label_egress.get_daddr().get_addr());
		fm.set_match().set_ip_proto(rofl::fudpframe::UDP_IP_PROTO);
		fm.set_match().set_udp_src(label_egress.get_saddr().get_port().get_value());
		fm.set_match().set_udp_dst(label_egress.get_daddr().get_port().get_value());
		fm.set_match().set_matches().add_match(rofl::openflow::experimental::gtp::coxmatch_ofx_gtp_msg_type(255)); // = G-PDU (255)
		fm.set_match().set_matches().add_match(rofl::openflow::experimental::gtp::coxmatch_ofx_gtp_teid(label_egress.get_teid().get_value()));

		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		flags.reset(FLAG_EGRESS_FM_INSTALLED);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[cterm_in4][handle_dpt_close_egress] dpt not found" << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::error << "[cterm_in4][handle_dpt_close_egress] control channel is down" << std::endl;
	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::error << "[cterm_in4][handle_dpt_close_egress] control channel congested" << std::endl;
	} catch (rofl::RoflException& e) {
		rofcore::logging::error << "[cterm_in4][handle_dpt_close_egress] unexpected exception caught: " << e.what() << std::endl;
	}
}



void
cterm_in4::handle_dpt_open_ingress(rofl::crofdpt& dpt)
{
	try {
		rofl::openflow::cofflowmod fm(dpt.get_version());

		if (not flags.test(FLAG_INGRESS_FM_INSTALLED)) {
			fm.set_command(rofl::openflow::OFPFC_ADD);
		} else {
			fm.set_command(rofl::openflow::OFPFC_MODIFY_STRICT);
		}

		fm.set_idle_timeout(idle_timeout);
		fm.set_hard_timeout(0);
		fm.set_priority(0xe000);
		fm.set_table_id(ofp_table_id - 1); // TODO: check ofp_table_id - 1

		fm.set_match().set_eth_type(tft_match.get_eth_type());
		if (tft_match.get_matches().has_match(rofl::openflow::OXM_TLV_BASIC_IPV4_SRC)) {
			fm.set_match().set_ipv4_src(tft_match.get_ipv4_src());
		}
		if (tft_match.get_matches().has_match(rofl::openflow::OXM_TLV_BASIC_IPV4_DST)) {
			fm.set_match().set_ipv4_dst(tft_match.get_ipv4_dst());
		}

		rofl::cindex index;
		// push header GTP (prepends IPv4/UDP/GTP)
		fm.set_instructions().add_inst_apply_actions().set_actions().add_action_experimenter(index).
				set_exp_id(rofl::openflow::experimental::gtp::GTP_EXP_ID);
		fm.set_instructions().set_inst_apply_actions().set_actions().set_action_experimenter(index).
				set_exp_body(rofl::openflow::experimental::gtp::cofaction_exp_body_push_gtp(rofl::fipv4frame::IPV4_ETHER));
		// set field IPv4 dst
		fm.set_instructions().set_inst_apply_actions().set_actions().set_action_set_field(++index).
				set_oxm(rofl::openflow::coxmatch_ofb_ipv4_dst(label_ingress.get_daddr().get_addr()));
		// set field IPv4 src
		fm.set_instructions().set_inst_apply_actions().set_actions().set_action_set_field(++index).
				set_oxm(rofl::openflow::coxmatch_ofb_ipv4_src(label_ingress.get_saddr().get_addr()));
		// set field IP proto
		fm.set_instructions().set_inst_apply_actions().set_actions().set_action_set_field(++index).
				set_oxm(rofl::openflow::coxmatch_ofb_ip_proto(rofl::fudpframe::UDP_IP_PROTO));
		// set field UDP dst
		fm.set_instructions().set_inst_apply_actions().set_actions().set_action_set_field(++index).
				set_oxm(rofl::openflow::coxmatch_ofb_udp_dst(label_ingress.get_daddr().get_port().get_value()));
		// set field UDP src
		fm.set_instructions().set_inst_apply_actions().set_actions().set_action_set_field(++index).
				set_oxm(rofl::openflow::coxmatch_ofb_udp_src(label_ingress.get_saddr().get_port().get_value()));
		// set field GTP msg type
		fm.set_instructions().set_inst_apply_actions().set_actions().set_action_set_field(++index).
				set_oxm(rofl::openflow::experimental::gtp::coxmatch_ofx_gtp_msg_type(255));
		// set field GTP TEID
		fm.set_instructions().set_inst_apply_actions().set_actions().set_action_set_field(++index).
				set_oxm(rofl::openflow::experimental::gtp::coxmatch_ofx_gtp_teid(label_ingress.get_teid().get_value()));
		// Goto Next Table: ofp_table_id + 1
		fm.set_instructions().add_inst_goto_table().set_table_id(ofp_table_id + 1);

		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		flags.set(FLAG_INGRESS_FM_INSTALLED);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[cterm_in4][handle_dpt_open_egress] dpt not found" << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::error << "[cterm_in4][handle_dpt_open_egress] control channel is down" << std::endl;
	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::error << "[cterm_in4][handle_dpt_open_egress] control channel congested" << std::endl;
	} catch (rofl::RoflException& e) {
		rofcore::logging::error << "[cterm_in4][handle_dpt_open_egress] unexpected exception caught: " << e.what() << std::endl;
	}
}



void
cterm_in4::handle_dpt_close_ingress(rofl::crofdpt& dpt)
{
	try {
		rofl::openflow::cofflowmod fm(dpt.get_version());

		fm.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		fm.set_idle_timeout(idle_timeout);
		fm.set_hard_timeout(0);
		fm.set_priority(0xe000);
		fm.set_table_id(ofp_table_id - 1); // TODO: check ofp_table_id - 1

		fm.set_match().set_eth_type(tft_match.get_eth_type());
		if (tft_match.get_matches().has_match(rofl::openflow::OXM_TLV_BASIC_IPV4_SRC)) {
			fm.set_match().set_ipv4_src(tft_match.get_ipv4_src());
		}
		if (tft_match.get_matches().has_match(rofl::openflow::OXM_TLV_BASIC_IPV4_DST)) {
			fm.set_match().set_ipv4_dst(tft_match.get_ipv4_dst());
		}

		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		flags.reset(FLAG_INGRESS_FM_INSTALLED);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[cterm_in4][handle_dpt_close_egress] dpt not found" << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::error << "[cterm_in4][handle_dpt_close_egress] control channel is down" << std::endl;
	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::error << "[cterm_in4][handle_dpt_close_egress] control channel congested" << std::endl;
	} catch (rofl::RoflException& e) {
		rofcore::logging::error << "[cterm_in4][handle_dpt_close_egress] unexpected exception caught: " << e.what() << std::endl;
	}
}



void
cterm_in6::handle_dpt_open_egress(rofl::crofdpt& dpt)
{
	try {
		rofl::openflow::cofflowmod fm(dpt.get_version());

		if (not flags.test(FLAG_EGRESS_FM_INSTALLED)) {
			fm.set_command(rofl::openflow::OFPFC_ADD);
		} else {
			fm.set_command(rofl::openflow::OFPFC_MODIFY_STRICT);
		}

		fm.set_idle_timeout(idle_timeout);
		fm.set_hard_timeout(0);
		fm.set_priority(0xe000);
		fm.set_table_id(ofp_table_id);

		fm.set_match().set_eth_type(rofl::fipv6frame::IPV6_ETHER);
		fm.set_match().set_ipv6_src(label_egress.get_saddr().get_addr());
		fm.set_match().set_ipv6_dst(label_egress.get_daddr().get_addr());
		fm.set_match().set_ip_proto(rofl::fudpframe::UDP_IP_PROTO);
		fm.set_match().set_udp_src(label_egress.get_saddr().get_port().get_value());
		fm.set_match().set_udp_dst(label_egress.get_daddr().get_port().get_value());
		fm.set_match().set_matches().add_match(rofl::openflow::experimental::gtp::coxmatch_ofx_gtp_msg_type(255)); // = G-PDU (255)
		fm.set_match().set_matches().add_match(rofl::openflow::experimental::gtp::coxmatch_ofx_gtp_teid(label_egress.get_teid().get_value()));

		rofl::cindex index(0);
		// pop header GTP (removes outer IPV4/UDP/GTP)
		fm.set_instructions().add_inst_apply_actions().set_actions().add_action_experimenter(rofl::cindex(0)).
				set_exp_id(rofl::openflow::experimental::gtp::GTP_EXP_ID);
		fm.set_instructions().set_inst_apply_actions().set_actions().set_action_experimenter(rofl::cindex(0)).
				set_exp_body(rofl::openflow::experimental::gtp::cofaction_exp_body_pop_gtp(rofl::fipv6frame::IPV6_ETHER));
		// Goto Next Table ofp_table_id + 1
		fm.set_instructions().add_inst_goto_table().set_table_id(ofp_table_id + 1);

		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		flags.set(FLAG_EGRESS_FM_INSTALLED);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[cterm_in6][handle_dpt_open_egress] dpt not found" << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::error << "[cterm_in6][handle_dpt_open_egress] control channel is down" << std::endl;
	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::error << "[cterm_in6][handle_dpt_open_egress] control channel congested" << std::endl;
	} catch (rofl::RoflException& e) {
		rofcore::logging::error << "[cterm_in6][handle_dpt_open_egress] unexpected exception caught: " << e.what() << std::endl;
	}
}



void
cterm_in6::handle_dpt_close_egress(rofl::crofdpt& dpt)
{
	try {
		rofl::openflow::cofflowmod fm(dpt.get_version());

		fm.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		fm.set_idle_timeout(idle_timeout);
		fm.set_hard_timeout(0);
		fm.set_priority(0xe000);
		fm.set_table_id(ofp_table_id);

		fm.set_match().set_eth_type(rofl::fipv6frame::IPV6_ETHER);
		fm.set_match().set_ipv6_src(label_egress.get_saddr().get_addr());
		fm.set_match().set_ipv6_dst(label_egress.get_daddr().get_addr());
		fm.set_match().set_ip_proto(rofl::fudpframe::UDP_IP_PROTO);
		fm.set_match().set_udp_src(label_egress.get_saddr().get_port().get_value());
		fm.set_match().set_udp_dst(label_egress.get_daddr().get_port().get_value());
		fm.set_match().set_matches().add_match(rofl::openflow::experimental::gtp::coxmatch_ofx_gtp_msg_type(255)); // = G-PDU (255)
		fm.set_match().set_matches().add_match(rofl::openflow::experimental::gtp::coxmatch_ofx_gtp_teid(label_egress.get_teid().get_value()));

		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		flags.reset(FLAG_EGRESS_FM_INSTALLED);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[cterm_in6][handle_dpt_close_egress] dpt not found" << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::error << "[cterm_in6][handle_dpt_close_egress] control channel is down" << std::endl;
	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::error << "[cterm_in6][handle_dpt_close_egress] control channel congested" << std::endl;
	} catch (rofl::RoflException& e) {
		rofcore::logging::error << "[cterm_in6][handle_dpt_close_egress] unexpected exception caught: " << e.what() << std::endl;
	}
}



void
cterm_in6::handle_dpt_open_ingress(rofl::crofdpt& dpt)
{
	try {
		rofl::openflow::cofflowmod fm(dpt.get_version());

		if (not flags.test(FLAG_INGRESS_FM_INSTALLED)) {
			fm.set_command(rofl::openflow::OFPFC_ADD);
		} else {
			fm.set_command(rofl::openflow::OFPFC_MODIFY_STRICT);
		}

		fm.set_idle_timeout(idle_timeout);
		fm.set_hard_timeout(0);
		fm.set_priority(0xe000);
		fm.set_table_id(ofp_table_id - 1); // TODO: check ofp_table_id - 1

		fm.set_match().set_eth_type(tft_match.get_eth_type());
		if (tft_match.get_matches().has_match(rofl::openflow::OXM_TLV_BASIC_IPV4_SRC)) {
			fm.set_match().set_ipv6_src(tft_match.get_ipv6_src());
		}
		if (tft_match.get_matches().has_match(rofl::openflow::OXM_TLV_BASIC_IPV4_DST)) {
			fm.set_match().set_ipv6_dst(tft_match.get_ipv6_dst());
		}

		rofl::cindex index;
		// push header GTP (prepends IPv6/UDP/GTP)
		fm.set_instructions().add_inst_apply_actions().set_actions().add_action_experimenter(index).
				set_exp_id(rofl::openflow::experimental::gtp::GTP_EXP_ID);
		fm.set_instructions().set_inst_apply_actions().set_actions().set_action_experimenter(index).
				set_exp_body(rofl::openflow::experimental::gtp::cofaction_exp_body_push_gtp(rofl::fipv6frame::IPV6_ETHER));
		// set field IPv6 dst
		fm.set_instructions().set_inst_apply_actions().set_actions().set_action_set_field(++index).
				set_oxm(rofl::openflow::coxmatch_ofb_ipv6_dst(label_ingress.get_daddr().get_addr()));
		// set field IPv6 src
		fm.set_instructions().set_inst_apply_actions().set_actions().set_action_set_field(++index).
				set_oxm(rofl::openflow::coxmatch_ofb_ipv6_src(label_ingress.get_saddr().get_addr()));
		// set field IP proto
		fm.set_instructions().set_inst_apply_actions().set_actions().set_action_set_field(++index).
				set_oxm(rofl::openflow::coxmatch_ofb_ip_proto(rofl::fudpframe::UDP_IP_PROTO));
		// set field UDP dst
		fm.set_instructions().set_inst_apply_actions().set_actions().set_action_set_field(++index).
				set_oxm(rofl::openflow::coxmatch_ofb_udp_dst(label_ingress.get_daddr().get_port().get_value()));
		// set field UDP src
		fm.set_instructions().set_inst_apply_actions().set_actions().set_action_set_field(++index).
				set_oxm(rofl::openflow::coxmatch_ofb_udp_src(label_ingress.get_saddr().get_port().get_value()));
		// set field GTP msg type
		fm.set_instructions().set_inst_apply_actions().set_actions().set_action_set_field(++index).
				set_oxm(rofl::openflow::experimental::gtp::coxmatch_ofx_gtp_msg_type(255));
		// set field GTP TEID
		fm.set_instructions().set_inst_apply_actions().set_actions().set_action_set_field(++index).
				set_oxm(rofl::openflow::experimental::gtp::coxmatch_ofx_gtp_teid(label_ingress.get_teid().get_value()));
		// Goto Next Table: ofp_table_id + 1
		fm.set_instructions().add_inst_goto_table().set_table_id(ofp_table_id + 1);

		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		flags.set(FLAG_INGRESS_FM_INSTALLED);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[cterm_in6][handle_dpt_open_egress] dpt not found" << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::error << "[cterm_in6][handle_dpt_open_egress] control channel is down" << std::endl;
	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::error << "[cterm_in6][handle_dpt_open_egress] control channel congested" << std::endl;
	} catch (rofl::RoflException& e) {
		rofcore::logging::error << "[cterm_in6][handle_dpt_open_egress] unexpected exception caught: " << e.what() << std::endl;
	}
}



void
cterm_in6::handle_dpt_close_ingress(rofl::crofdpt& dpt)
{
	try {
		rofl::openflow::cofflowmod fm(dpt.get_version());

		fm.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		fm.set_idle_timeout(idle_timeout);
		fm.set_hard_timeout(0);
		fm.set_priority(0xe000);
		fm.set_table_id(ofp_table_id - 1); // TODO: check ofp_table_id - 1

		fm.set_match().set_eth_type(tft_match.get_eth_type());
		if (tft_match.get_matches().has_match(rofl::openflow::OXM_TLV_BASIC_IPV6_SRC)) {
			fm.set_match().set_ipv6_src(tft_match.get_ipv6_src());
		}
		if (tft_match.get_matches().has_match(rofl::openflow::OXM_TLV_BASIC_IPV6_DST)) {
			fm.set_match().set_ipv6_dst(tft_match.get_ipv6_dst());
		}

		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		flags.reset(FLAG_INGRESS_FM_INSTALLED);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[cterm_in6][handle_dpt_close_egress] dpt not found" << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::error << "[cterm_in6][handle_dpt_close_egress] control channel is down" << std::endl;
	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::error << "[cterm_in6][handle_dpt_close_egress] control channel congested" << std::endl;
	} catch (rofl::RoflException& e) {
		rofcore::logging::error << "[cterm_in6][handle_dpt_close_egress] unexpected exception caught: " << e.what() << std::endl;
	}
}



