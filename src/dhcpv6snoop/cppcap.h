/*
 * cppcap.h
 *
 *  Created on: 11.09.2013
 *      Author: andreas
 */

#ifndef CPPCAP_H_
#define CPPCAP_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <string.h>
#include <pcap.h>
#include <pthread.h>
#ifdef __cplusplus
}
#endif

#include <string>
#include <exception>


#include <rofl/common/protocols/fipv6frame.h>


#include "cdhcpmsg.h"
#include "cdhcpmsg_clisrv.h"
#include "cdhcpmsg_relay.h"

namespace rutils
{

class ePcapBase : public std::exception {};
class ePcapThreadFailed : public ePcapBase {};

class cppcap
{
	pthread_t			tid;
	bool 				keep_going;

	std::string 		devname;
	pcap_t 				*pcap_handle;
	struct bpf_program	bpfp;
	char 				pcap_errbuf[PCAP_ERRBUF_SIZE];

public:

	cppcap();

	~cppcap();

public:

	void
	start(std::string const& devname);

	void
	stop();

private:

	static void*
	run_pcap_thread(void *arg);

	void*
	do_capture();

	void
	handle_dhcpv6_relay_msg(uint8_t* buf, size_t buflen);

	void
	handle_dhcpv6_clisrv_msg(uint8_t* buf, size_t buflen);
};

}; // end of namespace

#endif /* CPPCAP_H_ */
