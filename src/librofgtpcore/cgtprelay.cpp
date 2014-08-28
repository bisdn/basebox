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
		case AF_INET: {
			try {
				// create label-in for received GTP message
				rofgtp::caddress_gtp_in4 gtp_src_addr(
						rofl::caddress_in4(from.ca_s4addr, sizeof(struct sockaddr_in)),
						rofgtp::cport(from.ca_s4addr, sizeof(struct sockaddr_in)));

				rofgtp::caddress_gtp_in4 gtp_dst_addr(
						rofl::caddress_in4(socket.get_laddr().ca_s4addr, sizeof(struct sockaddr_in)),
						rofgtp::cport(socket.get_laddr().ca_s4addr, sizeof(struct sockaddr_in)));

				rofgtp::clabel_in4 label_in(gtp_src_addr, gtp_dst_addr, rofgtp::cteid(gtpu.get_teid()));

				rofcore::logging::debug << "[cgtprelay][handle_read] label-in: " << std::endl << label_in;

				if (cgtpcore::get_gtp_core(dpid).has_relay_in4(label_in)) {

					// find associated label-out for label-in
					const rofgtp::clabel_in4& label_out =
							cgtpcore::get_gtp_core(dpid).
									get_relay_in4(label_in).get_label_out();

					rofcore::logging::debug << "[cgtprelay][handle_read] label-out: " << std::endl << label_out;

					// set TEID for outgoing packet
					gtpu.set_teid(label_out.get_teid().get_value());

					// forward GTP message
					set_socket_in4(label_out.get_saddr()).send(mem,
							rofl::csockaddr(label_out.get_daddr().get_addr(), label_out.get_daddr().get_port().get_value()));

					// set OFP shortcut into datapath
					cgtpcore::set_gtp_core(dpid).set_relay_in4(label_in).handle_dpt_open(rofl::crofdpt::get_dpt(dpid));

				} else
				if (cgtpcore::get_gtp_core(dpid).has_term_in4(/*egress label*/label_in)) {

					// find associated tft-match for label-in
					const rofgtp::cterm_in4& term =
							cgtpcore::get_gtp_core(dpid).get_term_in4(label_in);

					rofl::cpacket* pkt = rofcore::cpacketpool::get_instance().acquire_pkt();

					pkt->assign(mem->somem(), mem->memlen());

					pkt->tag_remove(sizeof(rofl::fgtpuframe::gtpu_base_hdr_t));

					set_termdev("tun57").enqueue(pkt); // TODO

					// set OFP shortcut into datapath
					cgtpcore::set_gtp_core(dpid).set_term_in4(label_in).handle_dpt_open_egress(rofl::crofdpt::get_dpt(dpid));

				}

			} catch (eGtpCoreNotFound& e) {
				rofcore::logging::debug << "[cgtprelay][handle_read] gtpcore not found" << std::endl;
				delete mem;
			} catch (eRelayNotFound& e) {
				rofcore::logging::debug << "[cgtprelay][handle_read] relay_in4 not found" << std::endl;
				delete mem;
			}
		} break;
		case AF_INET6: {
			try {
				// create label-in for received GTP message
				rofgtp::caddress_gtp_in6 gtp_src_addr(
						rofl::caddress_in6(from.ca_s6addr, sizeof(struct sockaddr_in6)),
						rofgtp::cport(from.ca_s6addr, sizeof(struct sockaddr_in6)));

				rofgtp::caddress_gtp_in6 gtp_dst_addr(
						rofl::caddress_in6(socket.get_laddr().ca_s6addr, sizeof(struct sockaddr_in6)),
						rofgtp::cport(socket.get_laddr().ca_s6addr, sizeof(struct sockaddr_in6)));

				rofgtp::clabel_in6 label_in(gtp_src_addr, gtp_dst_addr, rofgtp::cteid(gtpu.get_teid()));

				rofcore::logging::debug << "[cgtprelay][handle_read] label-in: " << std::endl << label_in;

				if (cgtpcore::get_gtp_core(dpid).has_relay_in6(label_in)) {

					// find associated label-out for label-in
					const rofgtp::clabel_in6& label_out =
							cgtpcore::get_gtp_core(dpid).
									get_relay_in6(label_in).get_label_out();

					rofcore::logging::debug << "[cgtprelay][handle_read] label-out: " << std::endl << label_out;

					// set TEID for outgoing packet
					gtpu.set_teid(label_out.get_teid().get_value());

					// forward GTP message
					set_socket_in6(label_out.get_saddr()).send(mem,
							rofl::csockaddr(label_out.get_daddr().get_addr(), label_out.get_daddr().get_port().get_value()));

					// set OFP shortcut into datapath
					cgtpcore::set_gtp_core(dpid).set_relay_in6(label_in).handle_dpt_open(rofl::crofdpt::get_dpt(dpid));

				} else
				if (cgtpcore::get_gtp_core(dpid).has_term_in6(label_in)) {

					// find associated tft-match for label-in
					const rofgtp::cterm_in6& term =
							cgtpcore::get_gtp_core(dpid).get_term_in6(/*egress label*/label_in);

					rofl::cpacket* pkt = rofcore::cpacketpool::get_instance().acquire_pkt();

					pkt->assign(mem->somem(), mem->memlen());

					pkt->tag_remove(sizeof(rofl::fgtpuframe::gtpu_base_hdr_t));

					set_termdev("tun57").enqueue(pkt); // TODO

					// set OFP shortcut into datapath
					cgtpcore::set_gtp_core(dpid).set_term_in6(label_in).handle_dpt_open_egress(rofl::crofdpt::get_dpt(dpid));

				}

			} catch (eGtpCoreNotFound& e) {
				rofcore::logging::debug << "[cgtprelay][handle_read] gtpcore not found" << std::endl;
				delete mem;
			} catch (eRelayNotFound& e) {
				rofcore::logging::debug << "[cgtprelay][handle_read] relay_in6 not found" << std::endl;
				delete mem;
			}
		} break;
		default: {
			delete mem;
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



void
cgtprelay::enqueue(rofcore::cnetdev *netdev, rofl::cpacket* pkt)
{
	rofcore::logging::debug << "[rofgtp][cgtprelay][enqueue] msg received: " << std::endl << *pkt;

	// read the first byte (to be precise: the first nibble) of this packet and determine IPv4 or IPv6
	if (pkt->length() < sizeof(uint8_t)) {
		rofcore::cpacketpool::get_instance().release_pkt(pkt);
		return;
	}

	uint8_t ip_version = ((*(pkt->soframe()) & 0xf0) >> 4);

	switch (ip_version) {
	case IP_VERSION_4: enqueue_in4(netdev, pkt); break;
	case IP_VERSION_6: enqueue_in6(netdev, pkt); break;
	default: {
		rofcore::logging::debug << "[rofgtp][cgtprelay][enqueue] unknown packet received, ignoring" << std::endl;
	};
	}

	rofcore::cpacketpool::get_instance().release_pkt(pkt);
}



void
cgtprelay::enqueue(rofcore::cnetdev *netdev, std::vector<rofl::cpacket*> pkts)
{
	for (std::vector<rofl::cpacket*>::iterator
			it = pkts.begin(); it != pkts.end(); ++it) {
		enqueue(netdev, *it);
	}
}



void
cgtprelay::enqueue_in4(rofcore::cnetdev *netdev, rofl::cpacket* pkt)
{

	if (sizeof(struct rofl::fipv4frame::ipv4_hdr_t) > pkt->length()) {
		rofcore::logging::debug << "[rofgtp][cgtprelay][enqueue_in4] invalid packet received, dropping" << std::endl;
		return;
	}

	rofl::fipv4frame ipv4(pkt->soframe(), pkt->length());

	rofcore::logging::debug << "[rofgtp][cgtprelay][enqueue_in4] IPv4 packet received" << std::endl << ipv4;

	// try to find TFT with source and destination address
	try {

		rofl::openflow::cofmatch tft_match(rofl::crofdpt::get_dpt(dpid).get_version());
		tft_match.set_eth_type(rofl::fipv4frame::IPV4_ETHER);
		tft_match.set_ipv4_dst(ipv4.get_ipv4_dst());
		tft_match.set_ipv4_src(ipv4.get_ipv4_src());

		if (cgtpcore::get_gtp_core(dpid).has_term_in4(tft_match)) {
			cterm_in4& term = cgtpcore::set_gtp_core(dpid).set_term_in4(tft_match);
			rofcore::logging::debug << "[rofgtp][cgtprelay][enqueue_in4] cterm_in4 (src->dst) found" << std::endl << term;

			size_t orig_len = pkt->length();

			pkt->tag_insert(sizeof(rofl::fgtpuframe::gtpu_base_hdr_t));

			rofl::fgtpuframe gtpu(pkt->soframe(), pkt->length());

			gtpu.set_version(rofl::fgtpuframe::GTPU_VERS_1);
			gtpu.set_msg_type(255);
			gtpu.set_teid(term.get_label_ingress().get_teid().get_value());
			gtpu.set_length(orig_len);

			rofcore::logging::debug << "[rofgtp][cgtprelay][enqueue_in4] tagged frame" << std::endl << *pkt;

			rofl::cmemory* mem = new rofl::cmemory(pkt->soframe(), pkt->length());

			pkt->tag_remove(sizeof(rofl::fgtpuframe::gtpu_base_hdr_t));

			rofl::csockaddr to(term.get_label_ingress().get_daddr().get_addr(),
									term.get_label_ingress().get_daddr().get_port().get_value());

			set_socket_in4(term.get_label_ingress().get_saddr()).send(mem, to);

			// set OFP shortcut into datapath
			term.handle_dpt_open_ingress(rofl::crofdpt::get_dpt(dpid));

			return;
		}

		tft_match.clear();
		tft_match.set_eth_type(rofl::fipv4frame::IPV4_ETHER);
		tft_match.set_ipv4_dst(ipv4.get_ipv4_dst());

		if (cgtpcore::get_gtp_core(dpid).has_term_in4(tft_match)) {
			cterm_in4& term = cgtpcore::set_gtp_core(dpid).set_term_in4(tft_match);
			rofcore::logging::debug << "[rofgtp][cgtprelay][enqueue_in4] cterm_in4 (dst only) found" << std::endl << term;

			size_t orig_len = pkt->length();

			pkt->tag_insert(sizeof(rofl::fgtpuframe::gtpu_base_hdr_t));

			rofl::fgtpuframe gtpu(pkt->soframe(), pkt->length());

			gtpu.set_version(rofl::fgtpuframe::GTPU_VERS_1);
			gtpu.set_msg_type(255);
			gtpu.set_teid(term.get_label_ingress().get_teid().get_value());
			gtpu.set_length(orig_len);

			rofcore::logging::debug << "[rofgtp][cgtprelay][enqueue_in4] tagged frame" << std::endl << *pkt;

			rofl::cmemory* mem = new rofl::cmemory(pkt->soframe(), pkt->length());

			pkt->tag_remove(sizeof(rofl::fgtpuframe::gtpu_base_hdr_t));

			rofl::csockaddr to(term.get_label_ingress().get_daddr().get_addr(),
									term.get_label_ingress().get_daddr().get_port().get_value());

			set_socket_in4(term.get_label_ingress().get_saddr()).send(mem, to);

			// set OFP shortcut into datapath
			term.handle_dpt_open_ingress(rofl::crofdpt::get_dpt(dpid));

			return;
		}


	} catch (eGtpRelayNotFound& e) {
		rofcore::logging::debug << "[rofgtp][cgtprelay][enqueue_in4] socket not found" << std::endl;
	} catch (eGtpCoreNotFound& e) {
		rofcore::logging::debug << "[rofgtp][cgtprelay][enqueue_in4] gtpcore not found" << std::endl;
	}

	// release_pkt() is called in method ::enqueue()
}



void
cgtprelay::enqueue_in6(rofcore::cnetdev *netdev, rofl::cpacket* pkt)
{
	if (sizeof(struct rofl::fipv6frame::ipv6_hdr_t) > pkt->length()) {
		rofcore::logging::debug << "[rofgtp][cgtprelay][enqueue_in6] invalid packet received, dropping" << std::endl;
		return;
	}

	rofl::fipv6frame ipv6(pkt->soframe(), pkt->length());

	rofcore::logging::debug << "[rofgtp][cgtprelay][enqueue_in6] IPv6 packet received" << std::endl << ipv6;

	// try to find TFT with source and destination address
	try {

		rofl::openflow::cofmatch tft_match(rofl::crofdpt::get_dpt(dpid).get_version());
		tft_match.set_eth_type(rofl::fipv6frame::IPV6_ETHER);
		tft_match.set_ipv6_dst(ipv6.get_ipv6_dst());
		tft_match.set_ipv6_src(ipv6.get_ipv6_src());

		if (cgtpcore::get_gtp_core(dpid).has_term_in6(tft_match)) {
			cterm_in6& term = cgtpcore::set_gtp_core(dpid).set_term_in6(tft_match);
			rofcore::logging::debug << "[rofgtp][cgtprelay][enqueue_in6] cterm_in6 (src->dst) found" << std::endl << term;

			size_t orig_len = pkt->length();

			pkt->tag_insert(sizeof(rofl::fgtpuframe::gtpu_base_hdr_t));

			rofl::fgtpuframe gtpu(pkt->soframe(), pkt->length());

			gtpu.set_version(rofl::fgtpuframe::GTPU_VERS_1);
			gtpu.set_msg_type(255);
			gtpu.set_teid(term.get_label_ingress().get_teid().get_value());
			gtpu.set_length(orig_len);

			rofcore::logging::debug << "[rofgtp][cgtprelay][enqueue_in6] tagged frame" << std::endl << *pkt;

			rofl::cmemory* mem = new rofl::cmemory(pkt->soframe(), pkt->length());

			pkt->tag_remove(sizeof(rofl::fgtpuframe::gtpu_base_hdr_t));

			rofl::csockaddr to(term.get_label_ingress().get_daddr().get_addr(),
									term.get_label_ingress().get_daddr().get_port().get_value());

			set_socket_in6(term.get_label_ingress().get_saddr()).send(mem, to);

			// set OFP shortcut into datapath
			term.handle_dpt_open_ingress(rofl::crofdpt::get_dpt(dpid));

			return;
		}

		tft_match.clear();
		tft_match.set_eth_type(rofl::fipv6frame::IPV6_ETHER);
		tft_match.set_ipv6_dst(ipv6.get_ipv6_dst());

		if (cgtpcore::get_gtp_core(dpid).has_term_in6(tft_match)) {
			cterm_in6& term = cgtpcore::set_gtp_core(dpid).set_term_in6(tft_match);
			rofcore::logging::debug << "[rofgtp][cgtprelay][enqueue_in6] cterm_in6 (dst only) found" << std::endl << term;

			size_t orig_len = pkt->length();

			pkt->tag_insert(sizeof(rofl::fgtpuframe::gtpu_base_hdr_t));

			rofl::fgtpuframe gtpu(pkt->soframe(), pkt->length());

			gtpu.set_version(rofl::fgtpuframe::GTPU_VERS_1);
			gtpu.set_msg_type(255);
			gtpu.set_teid(term.get_label_ingress().get_teid().get_value());
			gtpu.set_length(orig_len);

			rofcore::logging::debug << "[rofgtp][cgtprelay][enqueue_in6] tagged frame" << std::endl << *pkt;

			rofl::cmemory* mem = new rofl::cmemory(pkt->soframe(), pkt->length());

			pkt->tag_remove(sizeof(rofl::fgtpuframe::gtpu_base_hdr_t));

			rofl::csockaddr to(term.get_label_ingress().get_daddr().get_addr(),
									term.get_label_ingress().get_daddr().get_port().get_value());

			set_socket_in6(term.get_label_ingress().get_saddr()).send(mem, to);

			// set OFP shortcut into datapath
			term.handle_dpt_open_ingress(rofl::crofdpt::get_dpt(dpid));

			return;
		}


	} catch (eGtpRelayNotFound& e) {
		rofcore::logging::debug << "[rofgtp][cgtprelay][enqueue_in6] socket not found" << std::endl;
	} catch (eGtpCoreNotFound& e) {
		rofcore::logging::debug << "[rofgtp][cgtprelay][enqueue_in6] gtpcore not found" << std::endl;
	}

	// release_pkt() is called in method ::enqueue()
}




