/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <deque>
#include <string>
#include <map>
#include <memory>
#include <mutex>

#include "sai.h"
#include <linux/ethtool.h>

extern "C" {
struct rtnl_link;
}

namespace basebox {

class cnetlink;
class port_manager;

class switch_callback {
public:
  virtual ~switch_callback() = default;
  virtual int enqueue_to_switch(uint32_t port_id, basebox::packet *) = 0;
};

class port_manager {

public:
  port_manager(){};
  virtual ~port_manager(){};

  virtual int create_portdev(uint32_t port_id, const std::string &port_name,
                             const rofl::caddress_ll &hwaddr,
                             switch_callback &callback) = 0;

  virtual int destroy_portdev(uint32_t port_id,
                              const std::string &port_name) = 0;

  virtual int enqueue(uint32_t port_id, basebox::packet *pkt) = 0;

  std::map<std::string, uint32_t> get_registered_ports() const {
    std::lock_guard<std::mutex> lock(tn_mutex);
    return port_names2id;
  }

  uint32_t get_port_id(int ifindex) const noexcept {
    // XXX TODO add assert wrt threading
    auto it = ifindex_to_id.find(ifindex);
    if (it == ifindex_to_id.end()) {
      return 0;
    } else {
      return it->second;
    }
  }

  int get_ifindex(uint32_t port_id) const noexcept {
    // XXX TODO add assert wrt threading
    auto it = id_to_ifindex.find(port_id);
    if (it == id_to_ifindex.end()) {
      return 0;
    } else {
      return it->second;
    }
  }

  void clear() noexcept {
    std::lock_guard<std::mutex> lock(tn_mutex);
    ifindex_to_id.clear();
    id_to_ifindex.clear();
  }

  virtual int change_port_status(const std::string name, bool status) = 0;
  virtual int set_port_speed(const std::string name, uint32_t speed,
                             uint8_t duplex) = 0;

  // access from northbound (cnetlink)
  virtual bool portdev_removed(rtnl_link *link) = 0;
  virtual bool portdev_ready(rtnl_link *link) = 0;
  virtual int update_mtu(rtnl_link *link) = 0;

protected:
  port_manager(const port_manager &other) = delete; // non construction-copyable
  port_manager &operator=(const port_manager &) = delete; // non copyable

  // locked access
  mutable std::mutex tn_mutex; // tap names mutex
  std::map<std::string, uint32_t> port_names2id;

  // only accessible from cnetlink
  std::map<int, uint32_t> ifindex_to_id;
  std::map<uint32_t, int> id_to_ifindex;
};

} // namespace basebox
