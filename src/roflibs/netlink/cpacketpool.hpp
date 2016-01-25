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
#include <rofl/common/locking.hpp>

namespace rofcore
{

class ePacketPoolBase 			: public std::runtime_error {
public:
	ePacketPoolBase(const std::string& __arg) : std::runtime_error(__arg) {};
};
class ePacketPoolExhausted	 	: public ePacketPoolBase {
public:
	ePacketPoolExhausted(const std::string& __arg) : ePacketPoolBase(__arg) {};
};
class ePacketPoolInval			: public ePacketPoolBase {
public:
	ePacketPoolInval(const std::string& __arg) : ePacketPoolBase(__arg) {};
};
class ePacketPoolInvalPkt		: public ePacketPoolInval {
public:
	ePacketPoolInvalPkt(const std::string& __arg) : ePacketPoolInval(__arg) {};
};

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

	// lock for peer controllers
	mutable rofl::crwlock                 pool_rwlock;

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
