/*
 * cpacketpool.cc
 *
 *  Created on: 24.06.2013
 *      Author: andreas
 */

#include <cpacketpool.h>

using namespace ethercore;

cpacketpool* cpacketpool::packetpool = (cpacketpool*)0;

cpacketpool::cpacketpool(
		unsigned int n_pkts,
		unsigned int pkt_size)
{
	for (int i = 0; i < n_pkts; ++i) {
		rofl::cpacket *pkt = new rofl::cpacket(rofl::openflow::OFP_VERSION_UNKNOWN, pkt_size, /* in_port = */0, /* do_classify = */false);
		pktpool.push_back(pkt);
		idlepool.insert(pkt);
	}
}


cpacketpool::cpacketpool(cpacketpool const& packetpool)
{

}


cpacketpool::~cpacketpool()
{
	for (std::vector<rofl::cpacket*>::iterator
			it = pktpool.begin(); it != pktpool.end(); ++it) {
		delete (*it);
	}
	pktpool.clear();
}






cpacketpool&
cpacketpool::get_instance(
		unsigned int n_pkts,
		unsigned int pkt_size)
{
	if (0 == cpacketpool::packetpool) {
		cpacketpool::packetpool = new cpacketpool(n_pkts, pkt_size);
	}
	return *(cpacketpool::packetpool);
}



rofl::cpacket*
cpacketpool::acquire_pkt()
{
	if (idlepool.empty()) {
		throw ePacketPoolExhausted();
	}
	rofl::cpacket *pkt = *(idlepool.begin());
	idlepool.erase(pkt);
	return pkt;
}



void
cpacketpool::release_pkt(
			rofl::cpacket* pkt)
{
	if (0 == pkt) {
		throw ePacketPoolInvalPkt();
	}
	idlepool.insert(pkt);
}


