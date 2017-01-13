/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <glog/logging.h>

#include "cpacketpool.hpp"

using namespace rofcore;

cpacketpool::cpacketpool(unsigned int n_pkts, unsigned int pkt_size) {
  for (unsigned int i = 0; i < n_pkts; ++i) {
    rofl::cpacket *pkt = new rofl::cpacket(pkt_size);
    pktpool.push_back(pkt);
    idlepool.push_back(pkt);
    VLOG(3) << __FUNCTION__ << ": created packet " << pkt;
  }

  pktpool.shrink_to_fit();
  idlepool.shrink_to_fit();
}

cpacketpool::cpacketpool(cpacketpool const &packetpool) {}

cpacketpool::~cpacketpool() {
  for (std::vector<rofl::cpacket *>::iterator it = pktpool.begin();
       it != pktpool.end(); ++it) {
    delete (*it);
  }
  pktpool.clear();
}

cpacketpool &cpacketpool::get_instance() {
  static cpacketpool instance;
  return instance;
}

rofl::cpacket *cpacketpool::acquire_pkt() {
  rofl::AcquireReadWriteLock rwlock(pool_rwlock);
  if (idlepool.empty()) {
    throw ePacketPoolExhausted(
        "cpacketpool::acquire_pkt() packetpool exhausted");
  }
  rofl::cpacket *pkt = idlepool.front();
  idlepool.pop_front();
  VLOG(3) << __FUNCTION__ << ": pkt=" << pkt;
  return pkt;
}

void cpacketpool::release_pkt(rofl::cpacket *pkt) {
  assert(pkt);
  pkt->clear();
  VLOG(3) << __FUNCTION__ << ": pkt=" << pkt;
  {
    rofl::AcquireReadWriteLock rwlock(pool_rwlock);
    idlepool.push_back(pkt);
  }
}
