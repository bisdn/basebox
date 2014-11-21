/*
 * cgreterm.cpp
 *
 *  Created on: 18.08.2014
 *      Author: andreas
 */

#include "cgreterm.hpp"
#include "roflibs/ethcore/cethcore.hpp"
#include "cgrecore.hpp"

using namespace roflibs::gre;

/*static*/std::string cgreterm::script_path_init = std::string("/var/lib/basebox/greterm.sh");
/*static*/std::string cgreterm::script_path_term = std::string("/var/lib/basebox/greterm.sh");


cgreterm::cgreterm(const rofl::cdpid& dpid, uint8_t eth_ofp_table_id,
		uint8_t gre_ofp_table_id, uint8_t ip_fwd_ofp_table_id,
		uint32_t gre_portno, uint32_t gre_key) :
	state(STATE_DETACHED), dpid(dpid), eth_ofp_table_id(eth_ofp_table_id),
	gre_ofp_table_id(gre_ofp_table_id), ip_fwd_ofp_table_id(ip_fwd_ofp_table_id),
	idle_timeout(DEFAULT_IDLE_TIMEOUT),
	gre_portno(gre_portno), gre_key(gre_key), gretap(0),
	cookie_gre_port_redirect(roflibs::common::openflow::ccookie_owner::acquire_cookie()),
	cookie_gre_tunnel_redirect(roflibs::common::openflow::ccookie_owner::acquire_cookie())
{
	// FIXME: fix port-name clashes for multiple datapath elements
	std::stringstream ss; ss << std::string("gretap") << gre_portno;
	gretap = new rofcore::ctapdev(this, dpid, ss.str(), /*pvid*/0, rofl::caddress_ll("00:00:00:00:00:00"),
			cgrecore::get_gre_core(dpid).get_thread_id());
}



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
		// pop VLAN header
		fm.set_instructions().add_inst_apply_actions().set_actions().add_action_pop_vlan(++index);
		// pop header GRE (removes outer ETHERNET/IPV4/GRE)
		fm.set_instructions().set_inst_apply_actions().set_actions().add_action_experimenter(++index).
				set_exp_id(rofl::openflow::experimental::gre::GRE_EXP_ID);
		fm.set_instructions().set_inst_apply_actions().set_actions().set_action_experimenter(index).
				set_exp_body(rofl::openflow::experimental::gre::cofaction_exp_body_pop_gre(0/* not used for transparent bridging*/));

		// Send decapsulated ethernet frame out via assigned port
		fm.set_instructions().set_inst_apply_actions().set_actions().add_action_output(++index).
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
		// push vlan, all IP traffic within the datapath carries one
		fm.set_instructions().set_inst_apply_actions().set_actions().add_action_push_vlan(++index);
		fm.set_instructions().set_inst_apply_actions().set_actions().set_action_push_vlan(index).
				set_eth_type(rofl::fvlanframe::VLAN_CTAG_ETHER);
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

		fm.set_match().set_eth_type(rofl::fipv6frame::IPV6_ETHER);
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
		// pop VLAN header
		fm.set_instructions().add_inst_apply_actions().set_actions().add_action_pop_vlan(++index);
		// pop header GRE (removes outer ETHERNET/IPV4/GRE)
		fm.set_instructions().set_inst_apply_actions().set_actions().add_action_experimenter(++index).
				set_exp_id(rofl::openflow::experimental::gre::GRE_EXP_ID);
		fm.set_instructions().set_inst_apply_actions().set_actions().set_action_experimenter(index).
				set_exp_body(rofl::openflow::experimental::gre::cofaction_exp_body_pop_gre(0/* not used for transparent bridging*/));

		// Send decapsulated ethernet frame out via assigned port
		fm.set_instructions().set_inst_apply_actions().set_actions().add_action_output(++index).
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

		fm.set_match().set_eth_type(rofl::fipv6frame::IPV6_ETHER);
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
		// push vlan, all IP traffic within the datapath carries one
		fm.set_instructions().set_inst_apply_actions().set_actions().add_action_push_vlan(++index);
		fm.set_instructions().set_inst_apply_actions().set_actions().set_action_push_vlan(index).
				set_eth_type(rofl::fvlanframe::VLAN_CTAG_ETHER);
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
		//gre_portname = dpt.get_ports().get_port(gre_portno).get_name();
		std::stringstream sstap; sstap << std::string("gretap") << gre_portno;
		gre_tap_portname = sstap.str();
		std::stringstream sstun; sstun << std::string("gretun") << gre_portno;
		gre_tun_portname = sstun.str();

		std::vector<std::string> argv;
		std::vector<std::string> envp;

		std::stringstream s_cmd;
		s_cmd << "CMD=INIT";
		envp.push_back(s_cmd.str());

		std::stringstream s_bridge;
		s_bridge << "BRIDGE=grebr" << gre_portno;
		envp.push_back(s_bridge.str());

		std::stringstream s_iface;
		s_iface << "GRETAP=" << gre_tap_portname;
		envp.push_back(s_iface.str());

		std::stringstream s_greiface;
		s_greiface << "GRETUN=" << gre_tun_portname;
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
		rofcore::logging::error << "[cgreterm_in4][hook_init] datapath not found, dpid: " << dpid.str() << std::endl;
	} catch (rofl::openflow::ePortNotFound& e) {
		rofcore::logging::error << "[cgreterm_in4][hook_init] unable to find portno: " << (unsigned int)gre_portno << std::endl;
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
		s_iface << "GRETAP=" << gre_tap_portname;
		envp.push_back(s_iface.str());

		std::stringstream s_greiface;
		s_greiface << "GRETUN=" << gre_tun_portname;
		envp.push_back(s_greiface.str());

		cgreterm::execute(cgreterm::script_path_term, argv, envp);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[cgreterm_in4][hook_term] datapath not found, dpid: " << dpid.str() << std::endl;
	}
}


void
cgreterm_in6::hook_init()
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dpid);
		//gre_portname = dpt.get_ports().get_port(gre_portno).get_name();
		std::stringstream sstap; sstap << std::string("gretap") << gre_portno;
		gre_tap_portname = sstap.str();
		std::stringstream sstun; sstun << std::string("gretun") << gre_portno;
		gre_tun_portname = sstun.str();

		std::vector<std::string> argv;
		std::vector<std::string> envp;

		std::stringstream s_cmd;
		s_cmd << "CMD=INIT";
		envp.push_back(s_cmd.str());

		std::stringstream s_bridge;
		s_bridge << "BRIDGE=grebr" << gre_portno;
		envp.push_back(s_bridge.str());

		std::stringstream s_iface;
		s_iface << "GRETAP=" << gre_tap_portname;
		envp.push_back(s_iface.str());

		std::stringstream s_greiface;
		s_greiface << "GRETUN=" << gre_tun_portname;
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
		rofcore::logging::error << "[cgreterm_in6][hook_init] datapath not found, dpid: " << dpid.str() << std::endl;
	} catch (rofl::openflow::ePortNotFound& e) {
		rofcore::logging::error << "[cgreterm_in6][hook_init] unable to find portno: " << (unsigned int)gre_portno << std::endl;
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
		s_iface << "GRETAP=" << gre_tap_portname;
		envp.push_back(s_iface.str());

		std::stringstream s_greiface;
		s_greiface << "GRETUN=" << gre_tun_portname;
		envp.push_back(s_greiface.str());

		cgreterm::execute(cgreterm::script_path_term, argv, envp);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[cgreterm_in6][hook_term] datapath not found, dpid: " << dpid.str() << std::endl;
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

	exit(1); // just in case execvpe fails
}



void
cgreterm::handle_packet_in(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_packet_in& msg)
{
	rofcore::logging::debug << "[cgreterm][handle_packet_in] pkt received" << std::endl;

	// ethernet frames received from gre port are enqueued to our local tap device
	if (msg.get_cookie() == cookie_gre_port_redirect) {
		if (!gretap) {
			rofcore::logging::debug << "[cgreterm][handle_packet_in] no gretap device, dropping packet " << std::endl << msg;
			return;
		}

		rofcore::logging::debug << "[cgreterm][handle_packet_in] pkt received from GRE tap port" << std::endl;

		rofl::cpacket *pkt = rofcore::cpacketpool::get_instance().acquire_pkt();
		*pkt = msg.get_packet();
		gretap->enqueue(pkt);

	} else
	// ip/gre frames received from peer entity are enqueued to ethcore's tap devices
	if (msg.get_cookie() == cookie_gre_tunnel_redirect) {
		// store packet in ethcore and thus, tap devices
		roflibs::eth::cethcore::set_eth_core(dpt.get_dpid()).handle_packet_in(dpt, auxid, msg);
	}
}



void
cgreterm::enqueue(rofcore::cnetdev *netdev, rofl::cpacket* pkt)
{
	try {
		rofcore::ctapdev* tapdev = dynamic_cast<rofcore::ctapdev*>( netdev );
		if (0 == tapdev) {
			throw eLinkTapDevNotFound("cgreterm::enqueue() tap device not found");
		}

		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(tapdev->get_dpid());

		if (not dpt.get_channel().is_established()) {
			throw eLinkNoDptAttached("cgreterm::enqueue() dpt not found");
		}

		rofl::openflow::cofactions actions(dpt.get_version());
		actions.set_action_output(rofl::cindex(0)).set_port_no(gre_portno);

		rofcore::logging::debug << "[cgreterm][enqueue] injecting pkt to GRE tap port" << std::endl;

		dpt.send_packet_out_message(
				rofl::cauxid(0),
				rofl::openflow::base::get_ofp_no_buffer(dpt.get_version()),
				rofl::openflow::base::get_ofpp_controller_port(dpt.get_version()),
				actions,
				pkt->soframe(),
				pkt->length());

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[cgreterm][enqueue] no data path attached, dropping outgoing packet" << std::endl;

	} catch (eLinkNoDptAttached& e) {
		rofcore::logging::error << "[cgreterm][enqueue] no data path attached, dropping outgoing packet" << std::endl;

	} catch (eLinkTapDevNotFound& e) {
		rofcore::logging::error << "[cgreterm][enqueue] unable to find tap device" << std::endl;
	}

	rofcore::cpacketpool::get_instance().release_pkt(pkt);
}



void
cgreterm::enqueue(rofcore::cnetdev *netdev, std::vector<rofl::cpacket*> pkts)
{
	for (std::vector<rofl::cpacket*>::iterator
			it = pkts.begin(); it != pkts.end(); ++it) {
		enqueue(netdev, *it);
	}
}



void
cgreterm_in4::redirect_gre_port(rofl::crofdpt& dpt, bool enable)
{
	try {
		rofl::openflow::cofflowmod fm(dpt.get_version());

		if (enable) {
			if (not flags.test(FLAG_GRE_PORT_REDIRECTED)) {
				fm.set_command(rofl::openflow::OFPFC_ADD);
			} else {
				fm.set_command(rofl::openflow::OFPFC_MODIFY_STRICT);
			}
		} else {
			fm.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		}

		fm.set_idle_timeout(0);
		fm.set_hard_timeout(0);
		fm.set_priority(0xe000);
		fm.set_cookie(cookie_gre_port_redirect);
		fm.set_table_id(eth_ofp_table_id);

		fm.set_match().set_in_port(gre_portno);

		rofl::cindex index(0);
		fm.set_instructions().set_inst_apply_actions().set_actions().
				set_action_output(index).set_port_no(rofl::openflow::OFPP_CONTROLLER);
		fm.set_instructions().set_inst_apply_actions().set_actions().
				set_action_output(index).set_max_len(1526);

		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		flags.set(FLAG_GRE_PORT_REDIRECTED);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[cgreterm_in4][redirect_gre_port] dpt not found" << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::error << "[cgreterm_in4][redirect_gre_port] control channel is down" << std::endl;
	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::error << "[cgreterm_in4][redirect_gre_port] control channel congested" << std::endl;
	} catch (rofl::RoflException& e) {
		rofcore::logging::error << "[cgreterm_in4][redirect_gre_port] unexpected exception caught: " << e.what() << std::endl;
	}
}



void
cgreterm_in6::redirect_gre_port(rofl::crofdpt& dpt, bool enable)
{
	try {
		rofl::openflow::cofflowmod fm(dpt.get_version());

		if (enable) {
			if (not flags.test(FLAG_GRE_PORT_REDIRECTED)) {
				fm.set_command(rofl::openflow::OFPFC_ADD);
			} else {
				fm.set_command(rofl::openflow::OFPFC_MODIFY_STRICT);
			}
		} else {
			fm.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		}

		fm.set_idle_timeout(0);
		fm.set_hard_timeout(0);
		fm.set_priority(0xe000);
		fm.set_cookie(cookie_gre_port_redirect);
		fm.set_table_id(eth_ofp_table_id);

		fm.set_match().set_in_port(gre_portno);

		rofl::cindex index(0);
		fm.set_instructions().set_inst_apply_actions().set_actions().
				set_action_output(index).set_port_no(rofl::openflow::OFPP_CONTROLLER);
		fm.set_instructions().set_inst_apply_actions().set_actions().
				set_action_output(index).set_max_len(1526);

		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		flags.set(FLAG_GRE_PORT_REDIRECTED);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[cgreterm_in6][redirect_gre_port] dpt not found" << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::error << "[cgreterm_in6][redirect_gre_port] control channel is down" << std::endl;
	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::error << "[cgreterm_in6][redirect_gre_port] control channel congested" << std::endl;
	} catch (rofl::RoflException& e) {
		rofcore::logging::error << "[cgreterm_in6][redirect_gre_port] unexpected exception caught: " << e.what() << std::endl;
	}
}



void
cgreterm_in4::redirect_gre_tunnel(rofl::crofdpt& dpt, bool enable)
{
	try {
		rofl::openflow::cofflowmod fm(dpt.get_version());

		if (enable) {
			if (not flags.test(FLAG_GRE_TUNNEL_REDIRECTED)) {
				fm.set_command(rofl::openflow::OFPFC_ADD);
			} else {
				fm.set_command(rofl::openflow::OFPFC_MODIFY_STRICT);
			}
		} else {
			fm.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		}

		fm.set_idle_timeout(0);
		fm.set_hard_timeout(0);
		fm.set_priority(0xe000);
		fm.set_cookie(cookie_gre_tunnel_redirect);
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
		fm.set_instructions().set_inst_apply_actions().set_actions().
				set_action_output(index).set_port_no(rofl::openflow::OFPP_CONTROLLER);
		fm.set_instructions().set_inst_apply_actions().set_actions().
				set_action_output(index).set_max_len(1526);

		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		flags.set(FLAG_GRE_TUNNEL_REDIRECTED);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[cgreterm_in4][redirect_gre_tunnel] dpt not found" << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::error << "[cgreterm_in4][redirect_gre_tunnel] control channel is down" << std::endl;
	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::error << "[cgreterm_in4][redirect_gre_tunnel] control channel congested" << std::endl;
	} catch (rofl::RoflException& e) {
		rofcore::logging::error << "[cgreterm_in4][redirect_gre_tunnel] unexpected exception caught: " << e.what() << std::endl;
	}
}



void
cgreterm_in6::redirect_gre_tunnel(rofl::crofdpt& dpt, bool enable)
{
	try {
		rofl::openflow::cofflowmod fm(dpt.get_version());

		if (enable) {
			if (not flags.test(FLAG_GRE_TUNNEL_REDIRECTED)) {
				fm.set_command(rofl::openflow::OFPFC_ADD);
			} else {
				fm.set_command(rofl::openflow::OFPFC_MODIFY_STRICT);
			}
		} else {
			fm.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		}

		fm.set_idle_timeout(0);
		fm.set_hard_timeout(0);
		fm.set_priority(0xe000);
		fm.set_cookie(cookie_gre_tunnel_redirect);
		fm.set_table_id(gre_ofp_table_id);

		fm.set_match().set_eth_type(rofl::fipv6frame::IPV6_ETHER);
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
		fm.set_instructions().set_inst_apply_actions().set_actions().
				set_action_output(index).set_port_no(rofl::openflow::OFPP_CONTROLLER);
		fm.set_instructions().set_inst_apply_actions().set_actions().
				set_action_output(index).set_max_len(1526);

		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		flags.set(FLAG_GRE_TUNNEL_REDIRECTED);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[cgreterm_in6][redirect_gre_tunnel] dpt not found" << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::error << "[cgreterm_in6][redirect_gre_tunnel] control channel is down" << std::endl;
	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::error << "[cgreterm_in6][redirect_gre_tunnel] control channel congested" << std::endl;
	} catch (rofl::RoflException& e) {
		rofcore::logging::error << "[cgreterm_in6][redirect_gre_tunnel] unexpected exception caught: " << e.what() << std::endl;
	}
}






