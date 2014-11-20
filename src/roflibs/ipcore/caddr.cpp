/*
 * caddr.cpp
 *
 *  Created on: 03.07.2013
 *      Author: andreas
 */


#include "caddr.hpp"
#include "cipcore.hpp"
#include "roflibs/ethcore/cethcore.hpp"

using namespace roflibs::ip;


void
caddr_in4::handle_dpt_open(rofl::crofdpt& dpt)
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

		fe.set_buffer_id(rofl::openflow::base::get_ofp_no_buffer(dpt.get_version()));
		fe.set_idle_timeout(0);
		fe.set_hard_timeout(0);
		fe.set_priority(0x7000);
		fe.set_table_id(in_ofp_table_id);
		fe.set_flags(rofl::openflow13::OFPFF_SEND_FLOW_REM);

		rofl::cindex index(0);


		// redirect ARP packets to control plane
		fe.set_match().clear();


		// ... and the link's dpt representation (clink) needed for OFP related data
		const roflibs::ip::clink& dpl =
				cipcore::get_ip_core(dpid).get_link(ifindex);

		// local VLAN associated with interface
		uint16_t vid = dpl.get_vlan_vid();

#if 0
		uint32_t ofp_port_no = dpt.get_ports().get_port(cipcore::get_ip_core(dpid).get_link(ifindex).get_devname()).get_port_no();
		fe.set_match().set_in_port(ofp_port_no); // only for ARP (bound to Ethernet segment)
#endif
		fe.set_match().set_vlan_vid(vid | rofl::openflow::OFPVID_PRESENT);
		fe.set_match().set_eth_type(rofl::farpv4frame::ARPV4_ETHER);
		fe.set_match().set_arp_tpa(rofcore::cnetlink::get_instance().get_links().get_link(ifindex).get_addrs_in4().get_addr(adindex).get_local_addr());
		fe.set_cookie(cookie_arp);
		fe.set_instructions().clear();
		fe.set_instructions().add_inst_apply_actions().set_actions().
				set_action_output(index).set_port_no(rofl::openflow::OFPP_CONTROLLER);
		fe.set_instructions().set_inst_apply_actions().set_actions().
				set_action_output(index).set_max_len(1526);
		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

		// redirect ICMPv4 packets to control plane
		fe.set_match().clear();
		//fe.set_match().set_vlan_vid(vid | rofl::openflow::OFPVID_PRESENT);
		fe.set_match().set_eth_type(rofl::fipv4frame::IPV4_ETHER);
		fe.set_match().set_ipv4_dst(rofcore::cnetlink::get_instance().get_links().get_link(ifindex).get_addrs_in4().get_addr(adindex).get_local_addr());
		fe.set_match().set_ip_proto(rofl::ficmpv4frame::ICMPV4_IP_PROTO);
		fe.set_cookie(cookie_icmpv4);
		fe.set_instructions().clear();
		fe.set_instructions().add_inst_apply_actions().set_actions().
				set_action_output(index).set_port_no(rofl::openflow::OFPP_CONTROLLER);
		fe.set_instructions().set_inst_apply_actions().set_actions().
				set_action_output(index).set_max_len(1526);
		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

		// move IPv4 packets to next table
		fe.set_match().clear();
		//fe.set_match().set_vlan_vid(vid | rofl::openflow::OFPVID_PRESENT);
		fe.set_match().set_eth_type(rofl::fipv4frame::IPV4_ETHER);
		fe.set_match().set_ipv4_dst(rofcore::cnetlink::get_instance().get_links().get_link(ifindex).get_addrs_in4().get_addr(adindex).get_local_addr());
		fe.set_cookie(cookie_ipv4);
		fe.set_instructions().clear();
		fe.set_instructions().add_inst_goto_table().set_table_id(in_ofp_table_id+1);
		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

		state = STATE_ATTACHED;

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[rofip][caddr_in4][handle_dpt_open] unable to find data path" << e.what() << std::endl;
	} catch (rofcore::eNetLinkNotFound& e) {
		rofcore::logging::error << "[rofip][caddr_in4][handle_dpt_open] unable to find netlink link" << e.what() << std::endl;
	} catch (rofcore::crtlink::eRtLinkNotFound& e) {
		rofcore::logging::error << "[rofip][caddr_in4][handle_dpt_open] unable to find link" << e.what() << std::endl;
	} catch (rofcore::crtaddr::eRtAddrNotFound& e) {
		rofcore::logging::error << "[rofip][caddr_in4][handle_dpt_open] unable to find address" << e.what() << std::endl;
	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::error << "[rofip][caddr_in4][handle_dpt_open] control channel congested" << e.what() << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::error << "[rofip][caddr_in4][handle_dpt_open] control channel is down" << e.what() << std::endl;
	} catch (...) {
		rofcore::logging::error << "[rofip][caddr_in4][handle_dpt_open] unexpected error" << std::endl;
	}
}



void
caddr_in4::handle_dpt_close(rofl::crofdpt& dpt)
{
	try {
		rofl::openflow::cofflowmod fe(dpt.get_version());

		fe.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		fe.set_priority(0x7000);
		fe.set_table_id(in_ofp_table_id);

		// redirect ARP packets to control plane
		fe.set_match().clear();

		// ... and the link's dpt representation (clink) needed for OFP related data
		const roflibs::ip::clink& dpl =
				cipcore::get_ip_core(dpid).get_link(ifindex);

		// local VLAN associated with interface
		uint16_t vid = dpl.get_vlan_vid();

#if 0
		uint32_t ofp_port_no = dpt.get_ports().get_port(cipcore::get_ip_core(dpid).get_link(ifindex).get_devname()).get_port_no();
		fe.set_match().set_in_port(ofp_port_no); // only for ARP (bound to Ethernet segment)
#endif
		fe.set_match().set_vlan_vid(vid | rofl::openflow::OFPVID_PRESENT);
		fe.set_match().set_eth_type(rofl::farpv4frame::ARPV4_ETHER);
		fe.set_match().set_arp_tpa(rofcore::cnetlink::get_instance().get_links().get_link(ifindex).get_addrs_in4().get_addr(adindex).get_local_addr());
		fe.set_cookie(cookie_arp);
		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

		// redirect ICMPv4 packets to control plane
		fe.set_match().clear();
		//fe.set_match().set_vlan_vid(vid | rofl::openflow::OFPVID_PRESENT);
		fe.set_match().set_eth_type(rofl::fipv4frame::IPV4_ETHER);
		fe.set_match().set_ipv4_dst(rofcore::cnetlink::get_instance().get_links().get_link(ifindex).get_addrs_in4().get_addr(adindex).get_local_addr());
		fe.set_match().set_ip_proto(rofl::ficmpv4frame::ICMPV4_IP_PROTO);
		fe.set_cookie(cookie_icmpv4);
		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

		// redirect IPv4 packets to control plane
		fe.set_match().clear();
		//fe.set_match().set_vlan_vid(vid | rofl::openflow::OFPVID_PRESENT);
		fe.set_match().set_eth_type(rofl::fipv4frame::IPV4_ETHER);
		fe.set_match().set_ipv4_dst(rofcore::cnetlink::get_instance().get_links().get_link(ifindex).get_addrs_in4().get_addr(adindex).get_local_addr());
		fe.set_cookie(cookie_ipv4);
		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

		state = STATE_DETACHED;

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[rofip][caddr_in4][handle_dpt_close] unable to find data path" << e.what() << std::endl;
	} catch (rofcore::eNetLinkNotFound& e) {
		rofcore::logging::error << "[rofip][caddr_in4][handle_dpt_close] unable to find link" << e.what() << std::endl;
	} catch (rofcore::crtlink::eRtLinkNotFound& e) {
		rofcore::logging::error << "[rofip][caddr_in4][handle_dpt_close] unable to find address" << e.what() << std::endl;
	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::error << "[rofip][caddr_in4][handle_dpt_close] control channel congested" << e.what() << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::error << "[rofip][caddr_in4][handle_dpt_close] control channel is down" << e.what() << std::endl;
	} catch (...) {
		rofcore::logging::error << "[rofip][caddr_in4][handle_dpt_close] unexpected error" << std::endl;

	}
}



void
caddr_in4::handle_packet_in(
		rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_packet_in& msg)
{
	rofcore::logging::debug << "[caddr_in4][handle_packet_in] pkt received: " << std::endl << msg;
	// store packet in ethcore and thus, tap devices
	roflibs::eth::cethcore::set_eth_core(dpt.get_dpid()).handle_packet_in(dpt, auxid, msg);
}



void
caddr_in6::handle_dpt_open(rofl::crofdpt& dpt)
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

		fe.set_buffer_id(rofl::openflow::base::get_ofp_no_buffer(dpt.get_version()));
		fe.set_idle_timeout(0);
		fe.set_hard_timeout(0);
		fe.set_priority(0x7000);
		fe.set_table_id(in_ofp_table_id);
		fe.set_flags(rofl::openflow13::OFPFF_SEND_FLOW_REM);

		rofl::cindex index(0);

		// ... and the link's dpt representation (clink) needed for OFP related data
		const roflibs::ip::clink& dpl =
				cipcore::get_ip_core(dpid).get_link(ifindex);

		// local VLAN associated with interface
		uint16_t vid = dpl.get_vlan_vid();

		// redirect ICMPv6 packets to control plane
		fe.set_match().clear();
		//fe.set_match().set_vlan_vid(vid | rofl::openflow::OFPVID_PRESENT);
		fe.set_match().set_eth_type(rofl::fipv6frame::IPV6_ETHER);
		fe.set_match().set_ipv6_dst(rofcore::cnetlink::get_instance().get_links().get_link(ifindex).get_addrs_in6().get_addr(adindex).get_local_addr());
		fe.set_match().set_ip_proto(rofl::ficmpv6frame::ICMPV6_IP_PROTO);
		fe.set_cookie(cookie_icmpv6);
		fe.set_instructions().clear();
		fe.set_instructions().add_inst_apply_actions().set_actions().
				set_action_output(index).set_port_no(rofl::openflow::OFPP_CONTROLLER));
		fe.set_instructions().set_inst_apply_actions().set_actions().
				set_action_output(index).set_max_len(1526);
		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

		// move IPv6 packets to next table
		fe.set_match().clear();
		//fe.set_match().set_vlan_vid(vid | rofl::openflow::OFPVID_PRESENT);
		fe.set_match().set_eth_type(rofl::fipv6frame::IPV6_ETHER);
		fe.set_match().set_ipv6_dst(rofcore::cnetlink::get_instance().get_links().get_link(ifindex).get_addrs_in6().get_addr(adindex).get_local_addr());
		fe.set_cookie(cookie_ipv6);
		fe.set_instructions().clear();
		fe.set_instructions().add_inst_goto_table().set_table_id(in_ofp_table_id+1);
		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

		state = STATE_ATTACHED;

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[rofip][caddr_in6][handle_dpt_open] unable to find data path" << e.what() << std::endl;
	} catch (rofcore::eNetLinkNotFound& e) {
		rofcore::logging::error << "[rofip][caddr_in6][handle_dpt_open] unable to find link" << e.what() << std::endl;
	} catch (rofcore::crtlink::eRtLinkNotFound& e) {
		rofcore::logging::error << "[rofip][caddr_in6][handle_dpt_open] unable to find link" << e.what() << std::endl;
	} catch (rofcore::crtaddr::eRtAddrNotFound& e) {
		rofcore::logging::error << "[rofip][caddr_in6][handle_dpt_open] unable to find address" << e.what() << std::endl;
	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::error << "[rofip][caddr_in6][handle_dpt_open] control channel congested" << e.what() << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::error << "[rofip][caddr_in6][handle_dpt_open] control channel is down" << e.what() << std::endl;
	} catch (...) {
		rofcore::logging::error << "[rofip][caddr_in6][handle_dpt_open] unexpected error" << std::endl;
	}
}



void
caddr_in6::handle_dpt_close(rofl::crofdpt& dpt)
{
	try {
		rofl::openflow::cofflowmod fe(dpt.get_version());

		fe.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		fe.set_priority(0x7000);
		fe.set_table_id(in_ofp_table_id);

		// ... and the link's dpt representation (clink) needed for OFP related data
		const roflibs::ip::clink& dpl =
				cipcore::get_ip_core(dpid).get_link(ifindex);

		// local VLAN associated with interface
		uint16_t vid = dpl.get_vlan_vid();

		// redirect ICMPv6 packets to control plane
		fe.set_match().clear();
		//fe.set_match().set_vlan_vid(vid | rofl::openflow::OFPVID_PRESENT);
		fe.set_match().set_eth_type(rofl::fipv6frame::IPV6_ETHER);
		fe.set_match().set_ipv6_dst(rofcore::cnetlink::get_instance().get_links().get_link(ifindex).get_addrs_in6().get_addr(adindex).get_local_addr());
		fe.set_match().set_ip_proto(rofl::ficmpv6frame::ICMPV6_IP_PROTO);
		fe.set_cookie(cookie_icmpv6);
		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

		// redirect IPv6 packets to control plane
		fe.set_match().clear();
		//fe.set_match().set_vlan_vid(vid | rofl::openflow::OFPVID_PRESENT);
		fe.set_match().set_eth_type(rofl::fipv6frame::IPV6_ETHER);
		fe.set_match().set_ipv6_dst(rofcore::cnetlink::get_instance().get_links().get_link(ifindex).get_addrs_in6().get_addr(adindex).get_local_addr());
		fe.set_cookie(cookie_ipv6);
		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

		state = STATE_DETACHED;

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[rofip][caddr_in6][handle_dpt_close] unable to find data path" << e.what() << std::endl;
	} catch (rofcore::eNetLinkNotFound& e) {
		rofcore::logging::error << "[rofip][caddr_in6][handle_dpt_close] unable to find link" << e.what() << std::endl;
	} catch (rofcore::crtlink::eRtLinkNotFound& e) {
		rofcore::logging::error << "[rofip][caddr_in6][handle_dpt_close] unable to find address" << e.what() << std::endl;
	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::error << "[rofip][caddr_in6][handle_dpt_close] control channel congested" << e.what() << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::error << "[rofip][caddr_in6][handle_dpt_close] control channel is down" << e.what() << std::endl;
	} catch (...) {
		rofcore::logging::error << "[rofip][caddr_in6][handle_dpt_close] unexpected error" << std::endl;
	}
}



void
caddr_in6::handle_packet_in(
		rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_packet_in& msg)
{
	rofcore::logging::debug << "[caddr_in6][handle_packet_in] pkt received: " << std::endl << msg;
	// store packet in ethcore and thus, tap devices
	roflibs::eth::cethcore::set_eth_core(dpt.get_dpid()).handle_packet_in(dpt, auxid, msg);
}


