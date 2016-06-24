/*
 * cnetdev.cc
 *
 *  Created on: 24.06.2013
 *      Author: andreas
 */

#include "cnetdev.hpp"

#include <fcntl.h>

using namespace rofcore;

void cnetdev_owner::enqueue(ctapdev *netdev, rofl::cpacket *pkt) {
  if (0 != pkt) {
    cpacketpool::get_instance().release_pkt(pkt);
  }
}

void cnetdev_owner::enqueue(ctapdev *netdev,
                            std::vector<rofl::cpacket *> pkts) {
  for (std::vector<rofl::cpacket *>::iterator it = pkts.begin();
       it != pkts.end(); ++it) {
    cpacketpool::get_instance().release_pkt(*it);
  }
}
