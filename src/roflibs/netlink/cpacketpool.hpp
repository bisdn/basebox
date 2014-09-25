/*
 * cpacketpool.h
 *
 *  Created on: 24.06.2013
 *      Author: andreas
 */

#ifndef CPACKETPOOL_H_
#define CPACKETPOOL_H_ 1

#include <exception>
#include <vector>
#include <set>

#include <rofl/common/cpacket.h>

namespace rofcore
{

class ePacketPoolBase 			: public std::exception {};
class ePacketPoolExhausted	 	: public ePacketPoolBase {};
class ePacketPoolInval			: public ePacketPoolBase {};
class ePacketPoolInvalPkt		: public ePacketPoolInval {};

class cpacketpool
{
	static cpacketpool 				*packetpool;
	cpacketpool(
			unsigned int n_pkts = 256,
			unsigned int pkt_size = 1518);
	cpacketpool(cpacketpool const& packetpool);
	~cpacketpool();

	std::vector<rofl::cpacket*> 	pktpool;
	std::set<rofl::cpacket*>		idlepool;

public:

	static cpacketpool&
	get_instance(
			unsigned int n_pkts = 256,
			unsigned int pkt_size = 1518);

	rofl::cpacket*
	acquire_pkt();

	void
	release_pkt(
			rofl::cpacket* pkt);
};

}; // end of namespace vmcore

#endif /* CPACKETPOOL_H_ */
