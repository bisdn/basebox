/*
 * cgtprelay.cpp
 *
 *  Created on: 21.08.2014
 *      Author: andreas
 */

#include "cgtprelay.hpp"
#include "cgtpcore.hpp"

using namespace roflibs::gtp;

/*static*/std::map<rofl::cdptid, cgtprelay*> cgtprelay::gtprelays;



void
cgtprelay::add_gtp_termdevs()
{
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);

		cgtpcoredb& db = cgtpcoredb::get_gtpcoredb(std::string("file"));

		rofl::cdpid dpid = rofl::crofdpt::get_dpt(dptid).get_dpid();

		// install GTPv4 termdevs
		for (std::set<unsigned int>::const_iterator
				it = db.get_gtp_term_ids(dpid).begin(); it != db.get_gtp_term_ids(dpid).end(); ++it) {
			const cgtptermentry& entry = db.get_gtp_term(dpid, *it);

			switch (entry.get_inject_filter().get_version()) {
			case 4: {
				set_termdev(entry.get_inject_filter().get_devname()).
						add_prefix_in4(rofcore::cprefix_in4(
								rofl::caddress_in4(entry.get_inject_filter().get_route()),
								rofl::caddress_in4(entry.get_inject_filter().get_netmask())));
			} break;
			case 6: {
				set_termdev(entry.get_inject_filter().get_devname()).
						add_prefix_in6(rofcore::cprefix_in6(
								rofl::caddress_in6(entry.get_inject_filter().get_route()),
								rofl::caddress_in6(entry.get_inject_filter().get_netmask())));
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
				roflibs::gtp::caddress_gtp_in4 gtp_src_addr(
						rofl::caddress_in4(from.ca_s4addr, sizeof(struct sockaddr_in)),
						roflibs::gtp::cport(from.ca_s4addr, sizeof(struct sockaddr_in)));

				roflibs::gtp::caddress_gtp_in4 gtp_dst_addr(
						rofl::caddress_in4(socket.get_laddr().ca_s4addr, sizeof(struct sockaddr_in)),
						roflibs::gtp::cport(socket.get_laddr().ca_s4addr, sizeof(struct sockaddr_in)));

				roflibs::gtp::clabel_in4 label_in(gtp_src_addr, gtp_dst_addr, roflibs::gtp::cteid(gtpu.get_teid()));

				rofcore::logging::debug << "[cgtprelay][handle_read] label-incoming/label-egress: " << std::endl << label_in;

				if (cgtpcore::get_gtp_core(dptid).has_relay_in4(label_in)) {

					const roflibs::gtp::crelay_in4& relay =
							cgtpcore::get_gtp_core(dptid).get_relay_in4(label_in);

					// find associated label-out for label-in
					const roflibs::gtp::clabel_in4& label_out = relay.get_label_out();

					rofcore::logging::debug << "[cgtprelay][handle_read][relay] found relay: " << std::endl << relay;

					// set TEID for outgoing packet
					gtpu.set_teid(label_out.get_teid().get_value());

					// forward GTP message
					set_socket_in4(label_out.get_saddr()).send(mem,
							rofl::csockaddr(label_out.get_daddr().get_addr(), label_out.get_daddr().get_port().get_value()));

					// set OFP shortcut into datapath
					cgtpcore::set_gtp_core(dptid).set_relay_in4(label_in).handle_dpt_open();

				} else
				/* this is the incoming label == egress label */
				if (cgtpcore::get_gtp_core(dptid).has_term_in4(/*egress label*/label_in)) {

					// find associated term point for label-in
					roflibs::gtp::cterm_in4& term =
							cgtpcore::set_gtp_core(dptid).set_term_in4(label_in);

					rofcore::logging::debug << "[cgtprelay][handle_read][term] found termination point: " << std::endl << term;

					rofl::cpacket* pkt = rofcore::cpacketpool::get_instance().acquire_pkt();

					pkt->assign(mem->somem(), mem->memlen());

					pkt->tag_remove(sizeof(rofl::fgtpuframe::gtpu_base_hdr_t));

					set_termdev(term.get_devname()).enqueue(pkt);

					// set OFP shortcut into datapath
					term.handle_dpt_open_egress();

				} else {
					rofcore::logging::debug << "[cgtprelay][handle_read] no relay or termination point found" << std::endl;

				}

			} catch (eGtpCoreNotFound& e) {
				rofcore::logging::debug << "[cgtprelay][handle_read] gtpcore not found" << std::endl;
				delete mem;
			} catch (eRelayNotFound& e) {
				rofcore::logging::debug << "[cgtprelay][handle_read] relay_in4 not found" << std::endl;
				delete mem;
			} catch (eTermNotFound& e) {
				rofcore::logging::debug << "[cgtprelay][handle_read] term_in4 not found" << std::endl;
				delete mem;
			}
		} break;
		case AF_INET6: {
			try {
				// create label-in for received GTP message
				roflibs::gtp::caddress_gtp_in6 gtp_src_addr(
						rofl::caddress_in6(from.ca_s6addr, sizeof(struct sockaddr_in6)),
						roflibs::gtp::cport(from.ca_s6addr, sizeof(struct sockaddr_in6)));

				roflibs::gtp::caddress_gtp_in6 gtp_dst_addr(
						rofl::caddress_in6(socket.get_laddr().ca_s6addr, sizeof(struct sockaddr_in6)),
						roflibs::gtp::cport(socket.get_laddr().ca_s6addr, sizeof(struct sockaddr_in6)));

				roflibs::gtp::clabel_in6 label_in(gtp_src_addr, gtp_dst_addr, roflibs::gtp::cteid(gtpu.get_teid()));

				rofcore::logging::debug << "[cgtprelay][handle_read] label-incoming/label-egress: " << std::endl << label_in;

				if (cgtpcore::get_gtp_core(dptid).has_relay_in6(label_in)) {

					const roflibs::gtp::crelay_in6& relay =
							cgtpcore::get_gtp_core(dptid).get_relay_in6(label_in);

					// find associated label-out for label-in
					const roflibs::gtp::clabel_in6& label_out = relay.get_label_out();

					rofcore::logging::debug << "[cgtprelay][handle_read][relay] found relay: " << std::endl << relay;

					// set TEID for outgoing packet
					gtpu.set_teid(label_out.get_teid().get_value());

					// forward GTP message
					set_socket_in6(label_out.get_saddr()).send(mem,
							rofl::csockaddr(label_out.get_daddr().get_addr(), label_out.get_daddr().get_port().get_value()));

					// set OFP shortcut into datapath
					cgtpcore::set_gtp_core(dptid).set_relay_in6(label_in).handle_dpt_open();

				} else
				/* this is the incoming label == egress label */
				if (cgtpcore::get_gtp_core(dptid).has_term_in6(label_in)) {

					// find associated term point for label-in
					roflibs::gtp::cterm_in6& term =
							cgtpcore::set_gtp_core(dptid).set_term_in6(label_in);

					rofcore::logging::debug << "[cgtprelay][handle_read][term] found termination point: " << std::endl << term;

					rofl::cpacket* pkt = rofcore::cpacketpool::get_instance().acquire_pkt();

					pkt->assign(mem->somem(), mem->memlen());

					pkt->tag_remove(sizeof(rofl::fgtpuframe::gtpu_base_hdr_t));

					set_termdev(term.get_devname()).enqueue(pkt);

					// set OFP shortcut into datapath
					term.handle_dpt_open_egress();

				} else {
					rofcore::logging::debug << "[cgtprelay][handle_read] no relay or termination point found" << std::endl;

				}

			} catch (eGtpCoreNotFound& e) {
				rofcore::logging::debug << "[cgtprelay][handle_read] gtpcore not found" << std::endl;
				delete mem;
			} catch (eRelayNotFound& e) {
				rofcore::logging::debug << "[cgtprelay][handle_read] relay_in6 not found" << std::endl;
				delete mem;
			} catch (eTermNotFound& e) {
				rofcore::logging::debug << "[cgtprelay][handle_read] term_in6 not found" << std::endl;
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
cgtprelay::enqueue(rofcore::cnetdev *netdev, std::vector<rofl::cpacket*> pkts)
{
	for (std::vector<rofl::cpacket*>::iterator
			it = pkts.begin(); it != pkts.end(); ++it) {
		enqueue(netdev, *it);
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

		if (not cgtpcore::get_gtp_core(dptid).has_term_in4(netdev->get_devname())) {
			rofcore::cpacketpool::get_instance().release_pkt(pkt);
			return;
		}

		cterm_in4& term = cgtpcore::set_gtp_core(dptid).set_term_in4(netdev->get_devname());
		rofcore::logging::debug << "[rofgtp][cgtprelay][enqueue_in4] cterm_in4 (src->dst) found" << std::endl << term;


		uint8_t ip_version = ((*(pkt->soframe()) & 0xf0) >> 4);
		switch (ip_version) {
		case rofl::fipv4frame::IPV4_IP_PROTO: {
			rofl::fipv4frame iphv4(pkt->soframe(), pkt->length());
			term.set_peer(iphv4.get_ipv4_src());
			// set OFP shortcut into datapath
			term.handle_dpt_open_ingress(iphv4.get_ipv4_src());

		} break;
		case rofl::fipv6frame::IPV6_IP_PROTO: {
			rofl::fipv6frame iphv6(pkt->soframe(), pkt->length());
			term.set_peer(iphv6.get_ipv6_src());
			// set OFP shortcut into datapath
			term.handle_dpt_open_ingress(iphv6.get_ipv6_src());

		} break;
		}


		size_t orig_len = pkt->length();

		pkt->tag_insert(sizeof(rofl::fgtpuframe::gtpu_base_hdr_t));

		rofl::fgtpuframe gtpu(pkt->soframe(), pkt->length());

		gtpu.set_version(rofl::fgtpuframe::GTPU_VERS_1);
		gtpu.set_pt_flag(true);
		gtpu.set_msg_type(255);
		gtpu.set_teid(term.get_label_ingress().get_teid().get_value());
		gtpu.set_length(orig_len);

		rofcore::logging::debug << "[rofgtp][cgtprelay][enqueue_in4] tagged frame" << std::endl << *pkt;

		rofl::cmemory* mem = new rofl::cmemory(pkt->soframe(), pkt->length());

		pkt->tag_remove(sizeof(rofl::fgtpuframe::gtpu_base_hdr_t));

		rofl::csockaddr to(term.get_label_ingress().get_daddr().get_addr(),
								term.get_label_ingress().get_daddr().get_port().get_value());

		set_socket_in4(term.get_label_ingress().get_saddr()).send(mem, to);


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

		if (cgtpcore::get_gtp_core(dptid).has_term_in6(netdev->get_devname())) {
			rofcore::cpacketpool::get_instance().release_pkt(pkt);
			return;
		}

		cterm_in6& term = cgtpcore::set_gtp_core(dptid).set_term_in6(netdev->get_devname());
		rofcore::logging::debug << "[rofgtp][cgtprelay][enqueue_in6] cterm_in6 (src->dst) found" << std::endl << term;


		uint8_t ip_version = ((*(pkt->soframe()) & 0xf0) >> 4);
		switch (ip_version) {
		case rofl::fipv4frame::IPV4_IP_PROTO: {
			rofl::fipv4frame iphv4(pkt->soframe(), pkt->length());
			term.set_peer(iphv4.get_ipv4_src());
			// set OFP shortcut into datapath
			term.handle_dpt_open_ingress(iphv4.get_ipv4_src());

		} break;
		case rofl::fipv6frame::IPV6_IP_PROTO: {
			rofl::fipv6frame iphv6(pkt->soframe(), pkt->length());
			term.set_peer(iphv6.get_ipv6_src());
			// set OFP shortcut into datapath
			term.handle_dpt_open_ingress(iphv6.get_ipv6_src());

		} break;
		}


		size_t orig_len = pkt->length();

		pkt->tag_insert(sizeof(rofl::fgtpuframe::gtpu_base_hdr_t));

		rofl::fgtpuframe gtpu(pkt->soframe(), pkt->length());

		gtpu.set_version(rofl::fgtpuframe::GTPU_VERS_1);
		gtpu.set_pt_flag(true);
		gtpu.set_msg_type(255);
		gtpu.set_teid(term.get_label_ingress().get_teid().get_value());
		gtpu.set_length(orig_len);

		rofcore::logging::debug << "[rofgtp][cgtprelay][enqueue_in6] tagged frame" << std::endl << *pkt;

		rofl::cmemory* mem = new rofl::cmemory(pkt->soframe(), pkt->length());

		pkt->tag_remove(sizeof(rofl::fgtpuframe::gtpu_base_hdr_t));

		rofl::csockaddr to(term.get_label_ingress().get_daddr().get_addr(),
								term.get_label_ingress().get_daddr().get_port().get_value());

		set_socket_in6(term.get_label_ingress().get_saddr()).send(mem, to);


	} catch (eGtpRelayNotFound& e) {
		rofcore::logging::debug << "[rofgtp][cgtprelay][enqueue_in6] socket not found" << std::endl;
	} catch (eGtpCoreNotFound& e) {
		rofcore::logging::debug << "[rofgtp][cgtprelay][enqueue_in6] gtpcore not found" << std::endl;
	}

	// release_pkt() is called in method ::enqueue()
}



void
cgtprelay::addr_in4_created(unsigned int ifindex, uint16_t adindex)
{
	try {
		const rofcore::crtaddr_in4& addr = rofcore::cnetlink::get_instance().
				get_links().get_link(ifindex).get_addrs_in4().get_addr(adindex);

		add_socket_in4(roflibs::gtp::caddress_gtp_in4(addr.get_local_addr(),
				roflibs::gtp::cport(roflibs::gtp::cgtpcore::DEFAULT_GTPU_PORT)));

	} catch (roflibs::ip::eLinkNotFound& e) {
		// ignore addresses assigned to non-datapath ports
	} catch (rofcore::crtlink::eRtLinkNotFound& e) {
		rofcore::logging::debug << "[cbasebox][addr_in4_created] link not found" << std::endl;
	} catch (rofcore::crtaddr::eRtAddrNotFound& e) {
		rofcore::logging::debug << "[cbasebox][addr_in4_created] address not found" << std::endl;
	} catch (rofl::eSysCall& e) {
			// ...
	}
}



void
cgtprelay::addr_in4_deleted(unsigned int ifindex, uint16_t adindex)
{
	try {
		const rofcore::crtaddr_in4& addr = rofcore::cnetlink::get_instance().
				get_links().get_link(ifindex).get_addrs_in4().get_addr(adindex);

		drop_socket_in4(roflibs::gtp::caddress_gtp_in4(addr.get_local_addr(),
				roflibs::gtp::cport(roflibs::gtp::cgtpcore::DEFAULT_GTPU_PORT)));

	} catch (roflibs::ip::eLinkNotFound& e) {
		// ignore addresses assigned to non-datapath ports
	} catch (rofcore::crtlink::eRtLinkNotFound& e) {
		rofcore::logging::debug << "[cbasebox][addr_in4_deleted] link not found" << std::endl;
	} catch (rofcore::crtaddr::eRtAddrNotFound& e) {
		rofcore::logging::debug << "[cbasebox][addr_in4_deleted] address not found" << std::endl;
	} catch (rofl::eSysCall& e) {
			// ...
	}
}



void
cgtprelay::addr_in6_created(unsigned int ifindex, uint16_t adindex)
{
	try {
		const rofcore::crtaddr_in6& addr = rofcore::cnetlink::get_instance().
				get_links().get_link(ifindex).get_addrs_in6().get_addr(adindex);

		add_socket_in6(roflibs::gtp::caddress_gtp_in6(addr.get_local_addr(),
				roflibs::gtp::cport(roflibs::gtp::cgtpcore::DEFAULT_GTPU_PORT)));

	} catch (roflibs::ip::eLinkNotFound& e) {
		// ignore addresses assigned to non-datapath ports
	} catch (rofcore::crtlink::eRtLinkNotFound& e) {
		rofcore::logging::debug << "[cbasebox][addr_in6_created] link not found" << std::endl;
	} catch (rofcore::crtaddr::eRtAddrNotFound& e) {
		rofcore::logging::debug << "[cbasebox][addr_in6_created] address not found" << std::endl;
	} catch (rofl::eSysCall& e) {
		// ...
	}
}



void
cgtprelay::addr_in6_deleted(unsigned int ifindex, uint16_t adindex)
{
	try {
		const rofcore::crtaddr_in6& addr = rofcore::cnetlink::get_instance().
				get_links().get_link(ifindex).get_addrs_in6().get_addr(adindex);

		drop_socket_in6(roflibs::gtp::caddress_gtp_in6(addr.get_local_addr(),
				roflibs::gtp::cport(roflibs::gtp::cgtpcore::DEFAULT_GTPU_PORT)));

	} catch (roflibs::ip::eLinkNotFound& e) {
		// ignore addresses assigned to non-datapath ports
	} catch (rofcore::crtlink::eRtLinkNotFound& e) {
		rofcore::logging::debug << "[cbasebox][addr_in6_deleted] link not found" << std::endl;
	} catch (rofcore::crtaddr::eRtAddrNotFound& e) {
		rofcore::logging::debug << "[cbasebox][addr_in6_deleted] address not found" << std::endl;
	} catch (rofl::eSysCall& e) {
			// ...
	}
}



void
cgtprelay::handle_timeout(
		int opaque, void* data)
{
	switch (opaque) {
	case TIMER_KEEP_ALIVE: {
		do_keep_alive();
		register_timer(TIMER_KEEP_ALIVE, rofl::ctimespec(8));
	} break;
	}
}



void
cgtprelay::do_keep_alive()
{
	for (std::set<unsigned int>::const_iterator
			it = cgtpcore::get_gtp_core(dptid).get_term_in4_ids().begin();
				it != cgtpcore::get_gtp_core(dptid).get_term_in4_ids().end(); ++it) try {
		const cterm_in4& term = cgtpcore::get_gtp_core(dptid).get_term_in4(*it);

		rofl::cmemory* mem = new rofl::cmemory(32);

		rofl::csockaddr to(term.get_label_ingress().get_daddr().get_addr(),
								term.get_label_ingress().get_daddr().get_port().get_value());

		set_socket_in4(term.get_label_ingress().get_saddr()).send(mem, to);

		for (std::map<rofl::caddress_in4, uint64_t>::const_iterator
				it = term.get_peers_in4().begin(); it != term.get_peers_in4().end(); ++it) {
			const rofl::caddress_in4& peer = it->first;

			// hack => send an empty frame also to our peer entity, just to keep the ARP entry alive
			// btw., OpenFlow is a crappy framework ...

			rofl::cmemory* mem = new rofl::cmemory(32);

			set_socket_in4(term.get_label_ingress().get_saddr()).send(mem, rofl::csockaddr(peer, 2152));
		}

	} catch (eGtpRelayNotFound& e) {};

	for (std::set<unsigned int>::const_iterator
			it = cgtpcore::get_gtp_core(dptid).get_term_in6_ids().begin();
				it != cgtpcore::get_gtp_core(dptid).get_term_in6_ids().end(); ++it) try {
		const cterm_in6& term = cgtpcore::get_gtp_core(dptid).get_term_in6(*it);

		rofl::cmemory* mem = new rofl::cmemory(32);

		rofl::csockaddr to(term.get_label_ingress().get_daddr().get_addr(),
								term.get_label_ingress().get_daddr().get_port().get_value());

		set_socket_in6(term.get_label_ingress().get_saddr()).send(mem, to);

		for (std::map<rofl::caddress_in6, uint64_t>::const_iterator
				it = term.get_peers_in6().begin(); it != term.get_peers_in6().end(); ++it) {
			const rofl::caddress_in6& peer = it->first;

			// hack => send an empty frame also to our peer entity, just to keep the ARP entry alive
			// btw., OpenFlow is a crappy framework ...

			rofl::cmemory* mem = new rofl::cmemory(32);

			set_socket_in6(term.get_label_ingress().get_saddr()).send(mem, rofl::csockaddr(peer, 2152));
		}

	} catch (eGtpRelayNotFound& e) {};


}
