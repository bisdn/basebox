/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <glog/logging.h>

#include "packetpool.hpp"

using namespace basebox;

packetpool::packetpool(unsigned int n_pkts, unsigned int pkt_size) {
  for (unsigned int i = 0; i < n_pkts; ++i) {
    rofl::cpacket *pkt = new rofl::cpacket(pkt_size);
    pktpool.push_back(pkt);
    idlepool.push_back(pkt);
    VLOG(3) << __FUNCTION__ << ": created packet " << pkt;
  }

  pktpool.shrink_to_fit();
  idlepool.shrink_to_fit();
}

packetpool::packetpool(packetpool const &pool) {}

packetpool::~packetpool() {
  for (std::vector<rofl::cpacket *>::iterator it = pktpool.begin();
       it != pktpool.end(); ++it) {
    delete (*it);
  }
  pktpool.clear();
}

packetpool &packetpool::get_instance() {
  static packetpool instance;
  return instance;
}

rofl::cpacket *packetpool::acquire_pkt() {
  rofl::AcquireReadWriteLock rwlock(pool_rwlock);
  if (idlepool.empty()) {
    throw ePacketPoolExhausted(
        "packetpool::acquire_pkt() packetpool exhausted");
  }
  rofl::cpacket *pkt = idlepool.front();
  idlepool.pop_front();
  VLOG(3) << __FUNCTION__ << ": pkt=" << pkt;
  return pkt;
}

void packetpool::release_pkt(rofl::cpacket *pkt) {
  assert(pkt);
  pkt->clear();
  VLOG(3) << __FUNCTION__ << ": pkt=" << pkt;
  {
    rofl::AcquireReadWriteLock rwlock(pool_rwlock);
    idlepool.push_back(pkt);
  }
}
