/*
 * cppcap.cc
 *
 *  Created on: 11.09.2013
 *      Author: andreas
 */

#include "cppcap.h"

using namespace rutils;


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

	std::string s_filter("ip proto udp and port 587");

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
	}

	tid = 0;

	return 0;
}
