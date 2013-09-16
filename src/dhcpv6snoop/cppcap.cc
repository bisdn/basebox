/*
 * cppcap.cc
 *
 *  Created on: 11.09.2013
 *      Author: andreas
 */

#include "cppcap.h"

using namespace rutils;

using namespace dhcpv6snoop;

cppcap::cppcap() :
		tid(0),
		keep_going(false),
		pcap_handle(0)
{
	memset(pcap_errbuf, 0, sizeof(pcap_errbuf));
}



cppcap::~cppcap()
{
	if (0 != tid) {
		keep_going = false;
		if (pthread_join(tid, 0) < 0) {
			fprintf(stderr, "error terminating thread, aborting\n");
		}
	}
}



void
cppcap::start(const std::string& devname)
{
	int rc = 0;

	this->devname = devname;

	keep_going = true;

	if ((rc = pthread_create(&tid, (pthread_attr_t*)0, &run_pcap_thread, this)) < 0) {
		throw ePcapThreadFailed();
	}
}



void
cppcap::stop()
{
	keep_going = false;
}



void*
cppcap::run_pcap_thread(void *arg)
{
	cppcap *ppcap = static_cast<cppcap*>( arg );

	return ppcap->do_capture();
}



void*
cppcap::do_capture()
{
	int rc = 0;

	if ((pcap_handle = pcap_open_live(devname.c_str(), 1518, 1, 100, pcap_errbuf)) == 0) {
		fprintf(stderr, "error opening device %s, aborting capture ...\n", devname.c_str());
		return 0;
	}

	std::string s_filter("udp port 547");

	if ((rc = pcap_compile(pcap_handle, &bpfp, s_filter.c_str(), 1, PCAP_NETMASK_UNKNOWN)) < 0) {
		fprintf(stderr, "error compiling filter for pcap, aborting capture ...\n");
		return 0;
	}

	if ((rc = pcap_setfilter(pcap_handle, &bpfp)) < 0) {
		fprintf(stderr, "error setting filter, aborting capture ...\n");
		return 0;
	}

restart:
	while (keep_going) {

		const u_char* data = (const u_char*)0;

		struct pcap_pkthdr phdr;

		if ((data = pcap_next(pcap_handle, &phdr)) == (const u_char*)0) {
			goto restart;
		}

		fprintf(stderr, "read packet with caplen: %d\n", phdr.caplen);


		unsigned int offset = /* Ethernet */14 + /*IPv6*/40 + /*UDP*/8;

		dhcpv6snoop::cdhcpmsg msg((uint8_t*)data + offset, phdr.caplen - offset);

		switch (msg.get_msg_type()) {
		/*RELAY-FORW*/ case 12:
		/*RELAY-REPLY*/case 13: {
			handle_dhcpv6_relay_msg((uint8_t*)data, phdr.caplen);
		} break;
		/* default: all client/server messages */
		default: {
			handle_dhcpv6_clisrv_msg((uint8_t*)data, phdr.caplen);
		} break;

		}
	}

	tid = 0;

	return 0;
}



void
cppcap::handle_dhcpv6_relay_msg(uint8_t *buf, size_t buflen)
{
	unsigned int offset = /* Ethernet */14 + /*IPv6*/40 + /*UDP*/8;

	dhcpv6snoop::cdhcpmsg_relay msg((uint8_t*)buf + offset, buflen - offset);
	std::cerr << msg << std::endl;
}



void
cppcap::handle_dhcpv6_clisrv_msg(uint8_t *buf, size_t buflen)
{
	unsigned int ipv6_offset = /* Ethernet */14;
	rofl::fipv6frame ipv6((uint8_t*)buf + ipv6_offset, buflen - ipv6_offset);
	std::cerr << ipv6 << std::endl;


	unsigned int dhcp_offset = /* Ethernet */14 + /*IPv6*/40 + /*UDP*/8;
	dhcpv6snoop::cdhcpmsg_clisrv msg((uint8_t*)buf + dhcp_offset, buflen - dhcp_offset);
	std::cerr << msg << std::endl;


	// directory structure => ${VARETCDIR}/dhcpv6snoop/clientid/serverid/prefix
	switch (msg.get_msg_type()) {
	case dhcpv6snoop::cdhcpmsg::REPLY: {
		cdhcp_option_clientid clientid = dynamic_cast<cdhcp_option_clientid&>( msg.get_option(cdhcp_option::DHCP_OPTION_CLIENTID) );
		std::cerr << "CLIENT-ID: " << clientid << std::endl;;

		cdhcp_option_ia_pd iapd = dynamic_cast<cdhcp_option_ia_pd&>( msg.get_option(cdhcp_option::DHCP_OPTION_IA_PD) );
		std::cerr << "IA PREFIX DELEGATION: " << iapd << std::endl;

		cdhcp_option_ia_prefix iaprefix = dynamic_cast<cdhcp_option_ia_prefix&>( iapd.get_option(cdhcp_option::DHCP_OPTION_IA_PREFIX) );
		std::cerr << "IA PREFIX: " << iaprefix << std::endl;

		cdhcp_option_serverid serverid = dynamic_cast<cdhcp_option_serverid&>( msg.get_option(cdhcp_option::DHCP_OPTION_SERVERID) );
		std::cerr << "SERVER-ID: " << serverid << std::endl;;

	} break;
	default: {
		// do nothing
	} break;
	}
}



