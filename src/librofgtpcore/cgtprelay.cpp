/*
 * cgtprelay.cpp
 *
 *  Created on: 21.08.2014
 *      Author: andreas
 */

#include "cgtprelay.hpp"
#include "cgtpcore.hpp"

using namespace rofgtp;

/*static*/std::map<rofl::cdpid, cgtprelay*> cgtprelay::gtprelays;

void
cgtprelay::handle_read(
		rofl::csocket& socket)
{
	unsigned int pkts_rcvd_in_round = 0;
	rofl::cmemory *mem = new rofl::cmemory(1518);

	try {
		rofcore::logging::debug << "[cgtprelay][handle_read] " << std::endl;
		int flags = 0;
		rofl::csockaddr from;

		int rc = socket.recv(mem->somem(), mem->memlen(), flags, from);
		mem->resize(rc);
		rofl::fgtpuframe gtpu(mem->somem(), mem->memlen());

		switch (from.get_family()) {
		case AF_INET: try {

			// create label-in for received GTP message
			rofgtp::caddress_gtp_in4 gtp_src_addr(
					rofl::caddress_in4(from.ca_s4addr, sizeof(struct sockaddr_in)),
					rofgtp::cport(from.ca_s4addr, sizeof(struct sockaddr_in)));

			rofgtp::caddress_gtp_in4 gtp_dst_addr(
					rofl::caddress_in4(socket.get_laddr().ca_s4addr, sizeof(struct sockaddr_in)),
					rofgtp::cport(socket.get_laddr().ca_s4addr, sizeof(struct sockaddr_in)));

			rofgtp::clabel_in4 label_in(gtp_src_addr, gtp_dst_addr, rofgtp::cteid(gtpu.get_teid()));

			rofcore::logging::debug << "[cgtprelay][handle_read] label-in: " << std::endl << label_in;

			// find associated label-out for label-in
			const rofgtp::clabel_in4& label_out =
					cgtpcore::get_gtp_core(dpid).
							get_relay_in4(label_in).get_label_out();

			rofcore::logging::debug << "[cgtprelay][handle_read] label-out: " << std::endl << label_out;

			// set TEID for outgoing packet
			gtpu.set_teid(label_out.get_teid().get_value());

			// forward GTP message
			set_socket(label_out.get_saddr()).send(mem,
					rofl::csockaddr(label_out.get_daddr().get_addr(), label_out.get_daddr().get_port().get_value()));

			// set OFP shortcut into datapath
			cgtpcore::set_gtp_core(dpid).set_relay_in4(label_in).handle_dpt_open(rofl::crofdpt::get_dpt(dpid));

		} catch (eGtpCoreNotFound& e) {
			rofcore::logging::debug << "[cgtprelay][handle_read] gtpcore not found" << std::endl;
		} catch (eRelayNotFound& e) {
			rofcore::logging::debug << "[cgtprelay][handle_read] relay_in4 not found" << std::endl;
		} break;
		case AF_INET6: try {

			// create label-in for received GTP message
			rofgtp::caddress_gtp_in6 gtp_src_addr(
					rofl::caddress_in6(from.ca_s6addr, sizeof(struct sockaddr_in6)),
					rofgtp::cport(from.ca_s6addr, sizeof(struct sockaddr_in6)));

			rofgtp::caddress_gtp_in6 gtp_dst_addr(
					rofl::caddress_in6(socket.get_laddr().ca_s6addr, sizeof(struct sockaddr_in6)),
					rofgtp::cport(socket.get_laddr().ca_s6addr, sizeof(struct sockaddr_in6)));

			rofgtp::clabel_in6 label_in(gtp_src_addr, gtp_dst_addr, rofgtp::cteid(gtpu.get_teid()));

			rofcore::logging::debug << "[cgtprelay][handle_read] label-in: " << std::endl << label_in;

			// find associated label-out for label-in
			const rofgtp::clabel_in6& label_out =
					cgtpcore::get_gtp_core(dpid).
							get_relay_in6(label_in).get_label_out();

			rofcore::logging::debug << "[cgtprelay][handle_read] label-out: " << std::endl << label_out;

			// set TEID for outgoing packet
			gtpu.set_teid(label_out.get_teid().get_value());

			// forward GTP message
			set_socket(label_out.get_saddr()).send(mem,
					rofl::csockaddr(label_out.get_daddr().get_addr(), label_out.get_daddr().get_port().get_value()));

			// set OFP shortcut into datapath
			cgtpcore::set_gtp_core(dpid).set_relay_in6(label_in).handle_dpt_open(rofl::crofdpt::get_dpt(dpid));

		} catch (eGtpCoreNotFound& e) {
			rofcore::logging::debug << "[cgtprelay][handle_read] gtpcore not found" << std::endl;
		} catch (eRelayNotFound& e) {
			rofcore::logging::debug << "[cgtprelay][handle_read] relay_in6 not found" << std::endl;
		} break;
		default: {

		};
		}


	} catch (rofl::eSocketRxAgain& e) {
		rofcore::logging::debug << "[cgtprelay][handle_read] eSocketRxAgain: no further data available on socket" << std::endl;
		delete mem;
	} catch (rofl::eSysCall& e) {
		rofcore::logging::warn << "[cgtprelay][handle_read] failed to read from socket: " << e << std::endl;
		delete mem;
	} catch (rofl::RoflException& e) {
		rofcore::logging::warn << "[cgtprelay][handle_read] dropping invalid message: " << e << std::endl;
		delete mem;
	}

}
