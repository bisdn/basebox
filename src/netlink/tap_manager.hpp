/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <deque>
#include <string>
#include <map>
#include <memory>
#include <mutex>

#include "sai.hpp"

namespace basebox {

class cnetlink;
class ctapdev;
class tap_io;
class tap_manager;

class switch_callback {
public:
  virtual ~switch_callback() = default;
  virtual int enqueue_to_switch(uint32_t port_id, basebox::packet *) = 0;
};

class tap_manager final {

public:
  tap_manager(cnetlink *nl);
  ~tap_manager();

  int create_tapdev(uint32_t port_id, const std::string &port_name,
                    switch_callback &callback);

  int destroy_tapdev(uint32_t port_id, const std::string &port_name);

  int enqueue(int fd, basebox::packet *pkt);

  std::map<std::string, uint32_t> get_registered_ports() const {
    std::lock_guard<std::mutex> lock(tn_mutex);
    return tap_names2id;
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

  int get_fd(uint32_t port_id) const noexcept;

  void tap_dev_ready(int ifindex, const std::string &name);
  void tap_dev_removed(int ifindex);

private:
  tap_manager(const tap_manager &other) = delete; // non construction-copyable
  tap_manager &operator=(const tap_manager &) = delete; // non copyable

  // locked access
  mutable std::mutex tn_mutex; // tap names mutex
  std::map<std::string, uint32_t> tap_names2id;
  std::map<std::string, int> tap_names2fds;

  // only accessible from southbound
  std::map<uint32_t, ctapdev *> tap_devs; // southbound id:tap_device
  std::deque<uint32_t> port_deleted;

  // only accessible from cnetlink
  std::map<int, uint32_t> ifindex_to_id;
  std::map<uint32_t, int> id_to_ifindex;

  std::unique_ptr<tap_io> io;
  cnetlink *nl;
};

} // namespace basebox
