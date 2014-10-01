/*
 * cgreterm.cpp
 *
 *  Created on: 18.08.2014
 *      Author: andreas
 */

#include "cgreterm.hpp"

using namespace roflibs::gre;

/*static*/std::string cgreterm::script_path_init = std::string("/var/lib/basebox/greterm.sh");
/*static*/std::string cgreterm::script_path_term = std::string("/var/lib/basebox/greterm.sh");


void
cgreterm_in4::handle_dpt_open_egress(rofl::crofdpt& dpt)
{
	try {
		rofl::openflow::cofflowmod fm(dpt.get_version());

		if (not flags.test(FLAG_EGRESS_FM_INSTALLED)) {
			fm.set_command(rofl::openflow::OFPFC_ADD);
		} else {
			fm.set_command(rofl::openflow::OFPFC_MODIFY_STRICT);
		}

		fm.set_idle_timeout(0);
		fm.set_hard_timeout(0);
		fm.set_priority(0xe000);
		fm.set_table_id(gre_ofp_table_id);

		fm.set_match().set_eth_type(rofl::fipv4frame::IPV4_ETHER);
		fm.set_match().set_ipv4_dst(laddr);
		fm.set_match().set_ipv4_src(raddr);
		fm.set_match().set_ip_proto(GRE_IP_PROTO);
		fm.set_match().set_matches().add_match(
				rofl::openflow::experimental::gre::coxmatch_ofx_gre_version(0 /*<< 15*/)); // GRE version 0
		fm.set_match().set_matches().add_match(
				rofl::openflow::experimental::gre::coxmatch_ofx_gre_prot_type(GRE_PROT_TYPE_TRANSPARENT_ETHERNET_BRIDGING)); // 0x6558
		fm.set_match().set_matches().add_match(
				rofl::openflow::experimental::gre::coxmatch_ofx_gre_key(gre_key)); // GRE key

		rofl::cindex index(0);
		// pop header GRE (removes outer ETHERNET/IPV4/GRE)
		fm.set_instructions().add_inst_apply_actions().set_actions().add_action_experimenter(rofl::cindex(0)).
				set_exp_id(rofl::openflow::experimental::gre::GRE_EXP_ID);
		fm.set_instructions().set_inst_apply_actions().set_actions().set_action_experimenter(rofl::cindex(0)).
				set_exp_body(rofl::openflow::experimental::gre::cofaction_exp_body_pop_gre(0/* not used for transparent bridging*/));

		// Send decapsulated ethernet frame out via assigned port
		fm.set_instructions().set_inst_apply_actions().set_actions().add_action_output(rofl::cindex(1)).
				set_port_no(gre_portno);

		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		flags.set(FLAG_EGRESS_FM_INSTALLED);

		state = STATE_ATTACHED;

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[cgreterm_in4][handle_dpt_open_egress] dpt not found" << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::error << "[cgreterm_in4][handle_dpt_open_egress] control channel is down" << std::endl;
	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::error << "[cgreterm_in4][handle_dpt_open_egress] control channel congested" << std::endl;
	} catch (rofl::RoflException& e) {
		rofcore::logging::error << "[cgreterm_in4][handle_dpt_open_egress] unexpected exception caught: " << e.what() << std::endl;
	}
}



void
cgreterm_in4::handle_dpt_close_egress(rofl::crofdpt& dpt)
{
	try {
		rofl::openflow::cofflowmod fm(dpt.get_version());

		fm.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		fm.set_idle_timeout(0);
		fm.set_hard_timeout(0);
		fm.set_priority(0xe000);
		fm.set_table_id(gre_ofp_table_id);

		fm.set_match().set_eth_type(rofl::fipv4frame::IPV4_ETHER);
		fm.set_match().set_ipv4_dst(laddr);
		fm.set_match().set_ipv4_src(raddr);
		fm.set_match().set_ip_proto(GRE_IP_PROTO);
		fm.set_match().set_matches().add_match(
				rofl::openflow::experimental::gre::coxmatch_ofx_gre_version(0 /*<< 15*/)); // GRE version 0
		fm.set_match().set_matches().add_match(
				rofl::openflow::experimental::gre::coxmatch_ofx_gre_prot_type(GRE_PROT_TYPE_TRANSPARENT_ETHERNET_BRIDGING)); // 0x6558
		fm.set_match().set_matches().add_match(
				rofl::openflow::experimental::gre::coxmatch_ofx_gre_key(gre_key)); // GRE key

		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		flags.reset(FLAG_EGRESS_FM_INSTALLED);

		state = STATE_DETACHED;

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[cgreterm_in4][handle_dpt_close_egress] dpt not found" << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::error << "[cgreterm_in4][handle_dpt_close_egress] control channel is down" << std::endl;
	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::error << "[cgreterm_in4][handle_dpt_close_egress] control channel congested" << std::endl;
	} catch (rofl::RoflException& e) {
		rofcore::logging::error << "[cgreterm_in4][handle_dpt_close_egress] unexpected exception caught: " << e.what() << std::endl;
	}
}



void
cgreterm_in4::handle_dpt_open_ingress(rofl::crofdpt& dpt)
{
	try {
		rofl::openflow::cofflowmod fm(dpt.get_version());

		if (not flags.test(FLAG_INGRESS_FM_INSTALLED)) {
			fm.set_command(rofl::openflow::OFPFC_ADD);
		} else {
			fm.set_command(rofl::openflow::OFPFC_MODIFY_STRICT);
		}

		fm.set_idle_timeout(0);
		fm.set_hard_timeout(0);
		fm.set_priority(0xe000);
		fm.set_table_id(eth_ofp_table_id);

		fm.set_match().set_in_port(gre_portno);

		rofl::cindex index;
		// push header GTP (prepends IPv4/UDP/GTP)
		fm.set_instructions().add_inst_apply_actions().set_actions().add_action_experimenter(index).
				set_exp_id(rofl::openflow::experimental::gre::GRE_EXP_ID);
		fm.set_instructions().set_inst_apply_actions().set_actions().set_action_experimenter(index).
				set_exp_body(rofl::openflow::experimental::gre::cofaction_exp_body_push_gre(rofl::fipv4frame::IPV4_ETHER));
		// set field IPv4 dst
		fm.set_instructions().set_inst_apply_actions().set_actions().set_action_set_field(++index).
				set_oxm(rofl::openflow::coxmatch_ofb_ipv4_dst(raddr));
		// set field IPv4 src
		fm.set_instructions().set_inst_apply_actions().set_actions().set_action_set_field(++index).
				set_oxm(rofl::openflow::coxmatch_ofb_ipv4_src(laddr));
		// decrement IPv4 TTL
		fm.set_instructions().set_inst_apply_actions().set_actions().set_action_dec_nw_ttl(++index);

		// set field IP proto
		fm.set_instructions().set_inst_apply_actions().set_actions().set_action_set_field(++index).
				set_oxm(rofl::openflow::coxmatch_ofb_ip_proto(GRE_IP_PROTO));
		// set field GRE version
		fm.set_instructions().set_inst_apply_actions().set_actions().set_action_set_field(++index).
				set_oxm(rofl::openflow::experimental::gre::coxmatch_ofx_gre_version(0));
		// set field GRE prot type
		fm.set_instructions().set_inst_apply_actions().set_actions().set_action_set_field(++index).
				set_oxm(rofl::openflow::experimental::gre::coxmatch_ofx_gre_prot_type(GRE_PROT_TYPE_TRANSPARENT_ETHERNET_BRIDGING));
		// set field GRE prot type
		fm.set_instructions().set_inst_apply_actions().set_actions().set_action_set_field(++index).
				set_oxm(rofl::openflow::experimental::gre::coxmatch_ofx_gre_key(gre_key));
		// Goto Next Table: ofp_table_id + 1
		fm.set_instructions().add_inst_goto_table().set_table_id(ip_fwd_ofp_table_id);

		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		flags.set(FLAG_INGRESS_FM_INSTALLED);

		state = STATE_ATTACHED;

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[cgreterm_in4][handle_dpt_open_egress] dpt not found" << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::error << "[cgreterm_in4][handle_dpt_open_egress] control channel is down" << std::endl;
	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::error << "[cgreterm_in4][handle_dpt_open_egress] control channel congested" << std::endl;
	} catch (rofl::RoflException& e) {
		rofcore::logging::error << "[cgreterm_in4][handle_dpt_open_egress] unexpected exception caught: " << e.what() << std::endl;
	}
}



void
cgreterm_in4::handle_dpt_close_ingress(rofl::crofdpt& dpt)
{
	try {
		rofl::openflow::cofflowmod fm(dpt.get_version());

		fm.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		fm.set_idle_timeout(0);
		fm.set_hard_timeout(0);
		fm.set_priority(0xe000);
		fm.set_table_id(eth_ofp_table_id);

		fm.set_match().set_in_port(gre_portno);

		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		flags.reset(FLAG_INGRESS_FM_INSTALLED);

		state = STATE_DETACHED;

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[cgreterm_in4][handle_dpt_close_egress] dpt not found" << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::error << "[cgreterm_in4][handle_dpt_close_egress] control channel is down" << std::endl;
	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::error << "[cgreterm_in4][handle_dpt_close_egress] control channel congested" << std::endl;
	} catch (rofl::RoflException& e) {
		rofcore::logging::error << "[cgreterm_in4][handle_dpt_close_egress] unexpected exception caught: " << e.what() << std::endl;
	}
}



void
cgreterm_in6::handle_dpt_open_egress(rofl::crofdpt& dpt)
{
	try {
		rofl::openflow::cofflowmod fm(dpt.get_version());

		if (not flags.test(FLAG_EGRESS_FM_INSTALLED)) {
			fm.set_command(rofl::openflow::OFPFC_ADD);
		} else {
			fm.set_command(rofl::openflow::OFPFC_MODIFY_STRICT);
		}

		fm.set_idle_timeout(0);
		fm.set_hard_timeout(0);
		fm.set_priority(0xe000);
		fm.set_table_id(gre_ofp_table_id);

		fm.set_match().set_eth_type(rofl::fipv4frame::IPV4_ETHER);
		fm.set_match().set_ipv6_dst(laddr);
		fm.set_match().set_ipv6_src(raddr);
		fm.set_match().set_ip_proto(GRE_IP_PROTO);
		fm.set_match().set_matches().add_match(
				rofl::openflow::experimental::gre::coxmatch_ofx_gre_version(0 /*<< 15*/)); // GRE version 0
		fm.set_match().set_matches().add_match(
				rofl::openflow::experimental::gre::coxmatch_ofx_gre_prot_type(GRE_PROT_TYPE_TRANSPARENT_ETHERNET_BRIDGING)); // 0x6558
		fm.set_match().set_matches().add_match(
				rofl::openflow::experimental::gre::coxmatch_ofx_gre_key(gre_key)); // GRE key

		rofl::cindex index(0);
		// pop header GRE (removes outer ETHERNET/IPV4/GRE)
		fm.set_instructions().add_inst_apply_actions().set_actions().add_action_experimenter(rofl::cindex(0)).
				set_exp_id(rofl::openflow::experimental::gre::GRE_EXP_ID);
		fm.set_instructions().set_inst_apply_actions().set_actions().set_action_experimenter(rofl::cindex(0)).
				set_exp_body(rofl::openflow::experimental::gre::cofaction_exp_body_pop_gre(0/* not used for transparent bridging*/));

		// Send decapsulated ethernet frame out via assigned port
		fm.set_instructions().add_inst_apply_actions().set_actions().add_action_output(rofl::cindex(0)).
				set_port_no(gre_portno);

		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		flags.set(FLAG_EGRESS_FM_INSTALLED);

		state = STATE_ATTACHED;

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[cgreterm_in6][handle_dpt_open_egress] dpt not found" << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::error << "[cgreterm_in6][handle_dpt_open_egress] control channel is down" << std::endl;
	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::error << "[cgreterm_in6][handle_dpt_open_egress] control channel congested" << std::endl;
	} catch (rofl::RoflException& e) {
		rofcore::logging::error << "[cgreterm_in6][handle_dpt_open_egress] unexpected exception caught: " << e.what() << std::endl;
	}
}



void
cgreterm_in6::handle_dpt_close_egress(rofl::crofdpt& dpt)
{
	try {
		rofl::openflow::cofflowmod fm(dpt.get_version());

		fm.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		fm.set_idle_timeout(0);
		fm.set_hard_timeout(0);
		fm.set_priority(0xe000);
		fm.set_table_id(gre_ofp_table_id);

		fm.set_match().set_eth_type(rofl::fipv4frame::IPV4_ETHER);
		fm.set_match().set_ipv6_dst(laddr);
		fm.set_match().set_ipv6_src(raddr);
		fm.set_match().set_ip_proto(GRE_IP_PROTO);
		fm.set_match().set_matches().add_match(
				rofl::openflow::experimental::gre::coxmatch_ofx_gre_version(0 /*<< 15*/)); // GRE version 0
		fm.set_match().set_matches().add_match(
				rofl::openflow::experimental::gre::coxmatch_ofx_gre_prot_type(GRE_PROT_TYPE_TRANSPARENT_ETHERNET_BRIDGING)); // 0x6558
		fm.set_match().set_matches().add_match(
				rofl::openflow::experimental::gre::coxmatch_ofx_gre_key(gre_key)); // GRE key

		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		flags.reset(FLAG_EGRESS_FM_INSTALLED);

		state = STATE_DETACHED;

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[cgreterm_in6][handle_dpt_close_egress] dpt not found" << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::error << "[cgreterm_in6][handle_dpt_close_egress] control channel is down" << std::endl;
	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::error << "[cgreterm_in6][handle_dpt_close_egress] control channel congested" << std::endl;
	} catch (rofl::RoflException& e) {
		rofcore::logging::error << "[cgreterm_in6][handle_dpt_close_egress] unexpected exception caught: " << e.what() << std::endl;
	}
}



void
cgreterm_in6::handle_dpt_open_ingress(rofl::crofdpt& dpt)
{
	try {
		rofl::openflow::cofflowmod fm(dpt.get_version());

		if (not flags.test(FLAG_INGRESS_FM_INSTALLED)) {
			fm.set_command(rofl::openflow::OFPFC_ADD);
		} else {
			fm.set_command(rofl::openflow::OFPFC_MODIFY_STRICT);
		}

		fm.set_idle_timeout(0);
		fm.set_hard_timeout(0);
		fm.set_priority(0xe000);
		fm.set_table_id(eth_ofp_table_id);

		fm.set_match().set_in_port(gre_portno);

		rofl::cindex index;
		// push header GTP (prepends IPv4/UDP/GTP)
		fm.set_instructions().add_inst_apply_actions().set_actions().add_action_experimenter(index).
				set_exp_id(rofl::openflow::experimental::gre::GRE_EXP_ID);
		fm.set_instructions().set_inst_apply_actions().set_actions().set_action_experimenter(index).
				set_exp_body(rofl::openflow::experimental::gre::cofaction_exp_body_push_gre(rofl::fipv4frame::IPV4_ETHER));
		// set field IPv4 dst
		fm.set_instructions().set_inst_apply_actions().set_actions().set_action_set_field(++index).
				set_oxm(rofl::openflow::coxmatch_ofb_ipv6_dst(raddr));
		// set field IPv4 src
		fm.set_instructions().set_inst_apply_actions().set_actions().set_action_set_field(++index).
				set_oxm(rofl::openflow::coxmatch_ofb_ipv6_src(laddr));
		// decrement IPv4 TTL
		fm.set_instructions().set_inst_apply_actions().set_actions().set_action_dec_nw_ttl(++index);

		// set field IP proto
		fm.set_instructions().set_inst_apply_actions().set_actions().set_action_set_field(++index).
				set_oxm(rofl::openflow::coxmatch_ofb_ip_proto(GRE_IP_PROTO));
		// set field GRE version
		fm.set_instructions().set_inst_apply_actions().set_actions().set_action_set_field(++index).
				set_oxm(rofl::openflow::experimental::gre::coxmatch_ofx_gre_version(0));
		// set field GRE prot type
		fm.set_instructions().set_inst_apply_actions().set_actions().set_action_set_field(++index).
				set_oxm(rofl::openflow::experimental::gre::coxmatch_ofx_gre_prot_type(GRE_PROT_TYPE_TRANSPARENT_ETHERNET_BRIDGING));
		// set field GRE prot type
		fm.set_instructions().set_inst_apply_actions().set_actions().set_action_set_field(++index).
				set_oxm(rofl::openflow::experimental::gre::coxmatch_ofx_gre_key(gre_key));
		// Goto Next Table: IP forwarding table
		fm.set_instructions().add_inst_goto_table().set_table_id(ip_fwd_ofp_table_id);

		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		flags.set(FLAG_INGRESS_FM_INSTALLED);

		state = STATE_ATTACHED;

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[cgreterm_in6][handle_dpt_open_egress] dpt not found" << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::error << "[cgreterm_in6][handle_dpt_open_egress] control channel is down" << std::endl;
	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::error << "[cgreterm_in6][handle_dpt_open_egress] control channel congested" << std::endl;
	} catch (rofl::RoflException& e) {
		rofcore::logging::error << "[cgreterm_in6][handle_dpt_open_egress] unexpected exception caught: " << e.what() << std::endl;
	}
}



void
cgreterm_in6::handle_dpt_close_ingress(rofl::crofdpt& dpt)
{
	try {
		rofl::openflow::cofflowmod fm(dpt.get_version());

		fm.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		fm.set_idle_timeout(0);
		fm.set_hard_timeout(0);
		fm.set_priority(0xe000);
		fm.set_table_id(eth_ofp_table_id);

		fm.set_match().set_in_port(gre_portno);

		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		flags.reset(FLAG_INGRESS_FM_INSTALLED);

		state = STATE_DETACHED;

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[cgreterm_in6][handle_dpt_close_egress] dpt not found" << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::error << "[cgreterm_in6][handle_dpt_close_egress] control channel is down" << std::endl;
	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::error << "[cgreterm_in6][handle_dpt_close_egress] control channel congested" << std::endl;
	} catch (rofl::RoflException& e) {
		rofcore::logging::error << "[cgreterm_in6][handle_dpt_close_egress] unexpected exception caught: " << e.what() << std::endl;
	}
}


void
cgreterm_in4::hook_init()
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dpid);

		std::vector<std::string> argv;
		std::vector<std::string> envp;

		std::stringstream s_cmd;
		s_cmd << "CMD=INIT";
		envp.push_back(s_cmd.str());

		std::stringstream s_bridge;
		s_bridge << "BRIDGE=grebr" << gre_portno;
		envp.push_back(s_bridge.str());

		std::stringstream s_iface;
		s_iface << "IFACE=" << dpt.get_ports().get_port(gre_portno).get_name();
		envp.push_back(s_iface.str());

		std::stringstream s_greiface;
		s_greiface << "GREIFACE=gretap" << gre_portno;
		envp.push_back(s_greiface.str());

		std::stringstream s_local;
		s_local << "GRELOCAL=" << laddr.str();
		envp.push_back(s_local.str());

		std::stringstream s_remote;
		s_remote << "GREREMOTE=" << raddr.str();
		envp.push_back(s_remote.str());

		std::stringstream s_key;
		s_key << "GREKEY=" << gre_key;
		envp.push_back(s_key.str());

		std::stringstream s_csum;
		s_csum << "GRECSUM=yes";
		envp.push_back(s_csum.str());

		cgreterm::execute(cgreterm::script_path_init, argv, envp);

	} catch (rofl::eRofDptNotFound& e) {

	}
}


void
cgreterm_in4::hook_term()
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dpid);

		std::vector<std::string> argv;
		std::vector<std::string> envp;

		std::stringstream s_cmd;
		s_cmd << "CMD=TERM";
		envp.push_back(s_cmd.str());

		std::stringstream s_bridge;
		s_bridge << "BRIDGE=grebr" << gre_portno;
		envp.push_back(s_bridge.str());

		std::stringstream s_iface;
		s_iface << "IFACE=" << dpt.get_ports().get_port(gre_portno).get_name();
		envp.push_back(s_iface.str());

		std::stringstream s_greiface;
		s_greiface << "GREIFACE=gretap" << gre_portno;
		envp.push_back(s_greiface.str());

		cgreterm::execute(cgreterm::script_path_term, argv, envp);

	} catch (rofl::eRofDptNotFound& e) {

	}
}


void
cgreterm_in6::hook_init()
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dpid);

		std::vector<std::string> argv;
		std::vector<std::string> envp;

		std::stringstream s_cmd;
		s_cmd << "CMD=INIT";
		envp.push_back(s_cmd.str());

		std::stringstream s_bridge;
		s_bridge << "BRIDGE=grebr" << gre_portno;
		envp.push_back(s_bridge.str());

		std::stringstream s_iface;
		s_iface << "IFACE=" << dpt.get_ports().get_port(gre_portno).get_name();
		envp.push_back(s_iface.str());

		std::stringstream s_greiface;
		s_greiface << "GREIFACE=gretap" << gre_portno;
		envp.push_back(s_greiface.str());

		std::stringstream s_local;
		s_local << "GRELOCAL=" << laddr.str();
		envp.push_back(s_local.str());

		std::stringstream s_remote;
		s_remote << "GREREMOTE=" << raddr.str();
		envp.push_back(s_remote.str());

		std::stringstream s_key;
		s_key << "GREKEY=" << gre_key;
		envp.push_back(s_key.str());

		std::stringstream s_csum;
		s_csum << "GRECSUM=yes";
		envp.push_back(s_csum.str());

		cgreterm::execute(cgreterm::script_path_init, argv, envp);

	} catch (rofl::eRofDptNotFound& e) {

	}
}


void
cgreterm_in6::hook_term()
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dpid);

		std::vector<std::string> argv;
		std::vector<std::string> envp;

		std::stringstream s_cmd;
		s_cmd << "CMD=TERM";
		envp.push_back(s_cmd.str());

		std::stringstream s_bridge;
		s_bridge << "BRIDGE=grebr" << gre_portno;
		envp.push_back(s_bridge.str());

		std::stringstream s_iface;
		s_iface << "IFACE=" << dpt.get_ports().get_port(gre_portno).get_name();
		envp.push_back(s_iface.str());

		std::stringstream s_greiface;
		s_greiface << "GREIFACE=gretap" << gre_portno;
		envp.push_back(s_greiface.str());

		cgreterm::execute(cgreterm::script_path_term, argv, envp);

	} catch (rofl::eRofDptNotFound& e) {

	}
}


void
cgreterm::execute(
		std::string const& executable,
		std::vector<std::string> argv,
		std::vector<std::string> envp)
{
	pid_t pid = 0;

	if ((pid = fork()) < 0) {
		rofcore::logging::error << "[cgreterm][execute] syscall error fork(): " << errno << ":" << strerror(errno) << std::endl;
		return;
	}

	if (pid > 0) { // father process
		return;
	}


	// child process

	std::vector<const char*> vctargv;
	for (std::vector<std::string>::iterator
			it = argv.begin(); it != argv.end(); ++it) {
		vctargv.push_back((*it).c_str());
	}
	vctargv.push_back(NULL);


	std::vector<const char*> vctenvp;
	for (std::vector<std::string>::iterator
			it = envp.begin(); it != envp.end(); ++it) {
		vctenvp.push_back((*it).c_str());
	}
	vctenvp.push_back(NULL);


	execvpe(executable.c_str(), (char*const*)&vctargv[0], (char*const*)&vctenvp[0]);

	std::cerr << "HAEHAEHAEHAEH => executable: " << executable << std::endl;

	exit(1); // just in case execvpe fails
}
