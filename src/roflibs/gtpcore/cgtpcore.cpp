/*
 * cgtpcore.cpp
 *
 *  Created on: 18.08.2014
 *      Author: andreas
 */

#include "cgtpcore.hpp"
#include "roflibs/ethcore/cethcore.hpp"

using namespace roflibs::gtp;

/*static*/std::map<rofl::cdptid, cgtpcore*> cgtpcore::gtpcores;


void
cgtpcore::add_gtp_relays()
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);

		cgtpcoredb& db = cgtpcoredb::get_gtpcoredb(std::string("file"));

		rofl::cdpid dpid = rofl::crofdpt::get_dpt(dptid).get_dpid();

		// install GTPv4 relays
		for (std::set<unsigned int>::const_iterator
				it = db.get_gtp_relay_ids(dpid).begin(); it != db.get_gtp_relay_ids(dpid).end(); ++it) {
			const cgtprelayentry& entry = db.get_gtp_relay(dpid, *it);

			switch (entry.get_incoming_label().get_version()) {
			case 4: {
				clabel_in4 inc_label(
						caddress_gtp_in4(
								rofl::caddress_in4(entry.get_incoming_label().get_src_addr()),
								cport(entry.get_incoming_label().get_src_port())),
						caddress_gtp_in4(
								rofl::caddress_in4(entry.get_incoming_label().get_dst_addr()),
								cport(entry.get_incoming_label().get_dst_port())),
						cteid(entry.get_incoming_label().get_teid()));

				clabel_in4 out_label(
						caddress_gtp_in4(
								rofl::caddress_in4(entry.get_outgoing_label().get_src_addr()),
								cport(entry.get_outgoing_label().get_src_port())),
						caddress_gtp_in4(
								rofl::caddress_in4(entry.get_outgoing_label().get_dst_addr()),
								cport(entry.get_outgoing_label().get_dst_port())),
						cteid(entry.get_outgoing_label().get_teid()));

				add_relay_in4(entry.get_relay_id(), inc_label, out_label);

			} break;
			case 6: {
				clabel_in6 inc_label(
						caddress_gtp_in6(
								rofl::caddress_in6(entry.get_incoming_label().get_src_addr()),
								cport(entry.get_incoming_label().get_src_port())),
						caddress_gtp_in6(
								rofl::caddress_in6(entry.get_incoming_label().get_dst_addr()),
								cport(entry.get_incoming_label().get_dst_port())),
						cteid(entry.get_incoming_label().get_teid()));

				clabel_in6 out_label(
						caddress_gtp_in6(
								rofl::caddress_in6(entry.get_outgoing_label().get_src_addr()),
								cport(entry.get_outgoing_label().get_src_port())),
						caddress_gtp_in6(
								rofl::caddress_in6(entry.get_outgoing_label().get_dst_addr()),
								cport(entry.get_outgoing_label().get_dst_port())),
						cteid(entry.get_outgoing_label().get_teid()));

				add_relay_in6(entry.get_relay_id(), inc_label, out_label);

			} break;
			default: {
				// TODO
			};
			}
		}

	} catch (rofl::eRofDptNotFound& e) {

	}
}


void
cgtpcore::add_gtp_terms()
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);

		cgtpcoredb& db = cgtpcoredb::get_gtpcoredb(std::string("file"));

		rofl::cdpid dpid = rofl::crofdpt::get_dpt(dptid).get_dpid();

		// install GTPv4 terms
		for (std::set<unsigned int>::const_iterator
				it = db.get_gtp_term_ids(dpid).begin(); it != db.get_gtp_term_ids(dpid).end(); ++it) {
			const cgtptermentry& entry = db.get_gtp_term(dpid, *it);

			switch (entry.get_ingress_label().get_version()) {
			case 4: {
				clabel_in4 ingress_label(
						caddress_gtp_in4(
								rofl::caddress_in4(entry.get_ingress_label().get_src_addr()),
								cport(entry.get_ingress_label().get_src_port())),
						caddress_gtp_in4(
								rofl::caddress_in4(entry.get_ingress_label().get_dst_addr()),
								cport(entry.get_ingress_label().get_dst_port())),
						cteid(entry.get_ingress_label().get_teid()));

				clabel_in4 egress_label(
						caddress_gtp_in4(
								rofl::caddress_in4(entry.get_egress_label().get_src_addr()),
								cport(entry.get_egress_label().get_src_port())),
						caddress_gtp_in4(
								rofl::caddress_in4(entry.get_egress_label().get_dst_addr()),
								cport(entry.get_egress_label().get_dst_port())),
						cteid(entry.get_egress_label().get_teid()));

				rofl::openflow::cofmatch match(rofl::crofdpt::get_dpt(dptid).get_version_negotiated());

				switch (entry.get_inject_filter().get_version()) {
				case 4: {
					match.set_eth_type(rofl::fipv4frame::IPV4_ETHER);
					match.set_ipv4_dst(
							rofl::caddress_in4(entry.get_inject_filter().get_dst_addr()),
							rofl::caddress_in4(entry.get_inject_filter().get_dst_mask()));
					match.set_ipv4_src(
							rofl::caddress_in4(entry.get_inject_filter().get_src_addr()),
							rofl::caddress_in4(entry.get_inject_filter().get_src_mask()));
				} break;
				case 6: {
					match.set_eth_type(rofl::fipv6frame::IPV6_ETHER);
					match.set_ipv6_dst(
							rofl::caddress_in6(entry.get_inject_filter().get_dst_addr()),
							rofl::caddress_in6(entry.get_inject_filter().get_dst_mask()));
					match.set_ipv6_src(
							rofl::caddress_in6(entry.get_inject_filter().get_src_addr()),
							rofl::caddress_in6(entry.get_inject_filter().get_src_mask()));
				} break;
				default: {
					// TODO
				};
				}

				add_term_in4(entry.get_term_id(), ingress_label, egress_label, match);

			} break;
			case 6: {
				clabel_in6 ingress_label(
						caddress_gtp_in6(
								rofl::caddress_in6(entry.get_ingress_label().get_src_addr()),
								cport(entry.get_ingress_label().get_src_port())),
						caddress_gtp_in6(
								rofl::caddress_in6(entry.get_ingress_label().get_dst_addr()),
								cport(entry.get_ingress_label().get_dst_port())),
						cteid(entry.get_ingress_label().get_teid()));

				clabel_in6 egress_label(
						caddress_gtp_in6(
								rofl::caddress_in6(entry.get_egress_label().get_src_addr()),
								cport(entry.get_egress_label().get_src_port())),
						caddress_gtp_in6(
								rofl::caddress_in6(entry.get_egress_label().get_dst_addr()),
								cport(entry.get_egress_label().get_dst_port())),
						cteid(entry.get_egress_label().get_teid()));

				rofl::openflow::cofmatch match(rofl::crofdpt::get_dpt(dptid).get_version_negotiated());

				switch (entry.get_inject_filter().get_version()) {
				case 4: {
					match.set_eth_type(rofl::fipv4frame::IPV4_ETHER);
					match.set_ipv4_dst(
							rofl::caddress_in4(entry.get_inject_filter().get_dst_addr()),
							rofl::caddress_in4(entry.get_inject_filter().get_dst_mask()));
					match.set_ipv4_src(
							rofl::caddress_in4(entry.get_inject_filter().get_src_addr()),
							rofl::caddress_in4(entry.get_inject_filter().get_src_mask()));
				} break;
				case 6: {
					match.set_eth_type(rofl::fipv6frame::IPV6_ETHER);
					match.set_ipv6_dst(
							rofl::caddress_in6(entry.get_inject_filter().get_dst_addr()),
							rofl::caddress_in6(entry.get_inject_filter().get_dst_mask()));
					match.set_ipv6_src(
							rofl::caddress_in6(entry.get_inject_filter().get_src_addr()),
							rofl::caddress_in6(entry.get_inject_filter().get_src_mask()));
				} break;
				default: {
					// TODO
				};
				}

				add_term_in6(entry.get_term_id(), ingress_label, egress_label, match);

			} break;
			default: {
				// TODO
			};
			}
		}

	} catch (rofl::eRofDptNotFound& e) {

	}
}


void
cgtpcore::handle_dpt_open()
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);

		rofl::openflow::cofflowmod fm(dpt.get_version_negotiated());
		switch (state) {
		case STATE_DETACHED: {
			fm.set_command(rofl::openflow::OFPFC_ADD);
		} break;
		case STATE_ATTACHED: {
			fm.set_command(rofl::openflow::OFPFC_MODIFY_STRICT);
		} break;
		}

		// install GTP UDP dst rule in GTP local-stage-table (ActionOutput Controller) IPv4
		fm.set_table_id(gtp_table_id);
		fm.set_priority(0x4000);
		fm.set_cookie(cookie_miss_entry_ipv4);
		fm.set_match().set_eth_type(rofl::fipv4frame::IPV4_ETHER);
		fm.set_match().set_ip_proto(rofl::fudpframe::UDP_IP_PROTO);
		fm.set_match().set_udp_dst(DEFAULT_GTPU_PORT);
		fm.set_instructions().drop_inst_goto_table();
		fm.set_instructions().add_inst_apply_actions().set_actions().
				add_action_output(rofl::cindex(0)).set_port_no(rofl::openflow::OFPP_CONTROLLER);
		fm.set_instructions().set_inst_apply_actions().set_actions().
				set_action_output(rofl::cindex(0)).set_max_len(1526);
		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		// install GTP UDP dst rule in GTP local-stage-table (ActionOutput Controller) IPv6
		fm.set_table_id(gtp_table_id);
		fm.set_priority(0x4000);
		fm.set_cookie(cookie_miss_entry_ipv6);
		fm.set_match().set_eth_type(rofl::fipv6frame::IPV6_ETHER);
		fm.set_match().set_ip_proto(rofl::fudpframe::UDP_IP_PROTO);
		fm.set_match().set_udp_dst(DEFAULT_GTPU_PORT);
		fm.set_instructions().drop_inst_goto_table();
		fm.set_instructions().add_inst_apply_actions().set_actions().
				add_action_output(rofl::cindex(0)).set_port_no(rofl::openflow::OFPP_CONTROLLER);
		fm.set_instructions().set_inst_apply_actions().set_actions().
				set_action_output(rofl::cindex(0)).set_max_len(1526);
		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		// relays and termination points are handled by the relaying and termination application, not here!

		state = STATE_ATTACHED;

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[rofgtp][cgtpcore][handle_dpt_open] unable to find data path" << e.what() << std::endl;
	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::error << "[rofgtp][cgtpcore][handle_dpt_open] control channel congested" << e.what() << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::error << "[rofgtp][cgtpcore][handle_dpt_open] control channel is down" << e.what() << std::endl;
	} catch (...) {
		rofcore::logging::error << "[rofgtp][cgtpcore][handle_dpt_open] unexpected error" << std::endl;
	}
}



void
cgtpcore::handle_dpt_close()
{
	try {
		state = STATE_DETACHED;

		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);

		rofl::openflow::cofflowmod fm(dpt.get_version_negotiated());
		fm.set_command(rofl::openflow::OFPFC_DELETE_STRICT);

		// remove GTP UDP dst rule in GTP local-stage-table (ActionOutput Controller) IPv4
		fm.set_table_id(gtp_table_id);
		fm.set_priority(0x4000);
		fm.set_cookie(cookie_miss_entry_ipv4);
		fm.set_match().set_eth_type(rofl::fipv4frame::IPV4_ETHER);
		fm.set_match().set_ip_proto(rofl::fudpframe::UDP_IP_PROTO);
		fm.set_match().set_udp_dst(DEFAULT_GTPU_PORT);
		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		// remove GTP UDP dst rule in GTP local-stage-table (ActionOutput Controller) IPv6
		fm.set_table_id(gtp_table_id);
		fm.set_priority(0x4000);
		fm.set_cookie(cookie_miss_entry_ipv6);
		fm.set_match().set_eth_type(rofl::fipv6frame::IPV6_ETHER);
		fm.set_match().set_ip_proto(rofl::fudpframe::UDP_IP_PROTO);
		fm.set_match().set_udp_dst(DEFAULT_GTPU_PORT);
		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		// relays and termination points are handled by the relaying and termination application, not here!

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[rofgtp][cgtpcore][handle_dpt_open] unable to find data path" << e.what() << std::endl;
	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::error << "[rofgtp][cgtpcore][handle_dpt_open] control channel congested" << e.what() << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::error << "[rofgtp][cgtpcore][handle_dpt_open] control channel is down" << e.what() << std::endl;
	} catch (...) {
		rofcore::logging::error << "[rofgtp][cgtpcore][handle_dpt_open] unexpected error" << std::endl;
	}
}



void
cgtpcore::handle_packet_in(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_packet_in& msg)
{
	rofcore::logging::debug << "[cgtpcore][handle_packet_in] pkt received: " << std::endl << msg;
	// store packet in ethcore and thus, tap devices
	roflibs::eth::cethcore::set_eth_core(dpt.get_dptid()).handle_packet_in(dpt, auxid, msg);
}



