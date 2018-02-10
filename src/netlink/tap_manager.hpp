/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <deque>
#include <string>
#include <vector>
#include <map>
#include <mutex>

#include <rofl/common/cpacket.h>

#include "netlink/ctapdev.hpp"
#include "sai.hpp"

namespace basebox {

class tap_io;
class tap_manager;

class switch_callback {
public:
  virtual int enqueue_to_switch(uint32_t port_id, basebox::packet *) = 0;
};

class tap_io : public rofl::cthread_env {
  enum tap_io_event {
    TAP_IO_ADD,
    TAP_IO_REM,
  };

  rofl::cthread thread;
  std::deque<std::pair<int, basebox::packet *>> pout_queue;
  std::mutex pout_queue_mutex;

  std::deque<std::tuple<enum tap_io_event, int, uint32_t, switch_callback *>>
      events;
  std::mutex events_mutex;

  std::deque<std::pair<int, basebox::packet *>> pin_queue;
  std::map<int, std::pair<uint32_t, switch_callback *>> sw_cbs;

public:
  tap_io() : thread(this) { thread.start("tap_io"); };
  virtual ~tap_io();

  // port_id should be removed at some point and be rather data
  void register_tap(int fd, uint32_t port_id, switch_callback &cb);
  void unregister_tap(int fd, uint32_t port_id);
  void enqueue(int fd, basebox::packet *pkt);

protected:
  void handle_read_event(rofl::cthread &thread, int fd);
  void handle_write_event(rofl::cthread &thread, int fd);
  void handle_wakeup(rofl::cthread &thread) {
    handle_events();
    tx();
  }
  void handle_timeout(rofl::cthread &thread, uint32_t timer_id) {}

private:
  void tx();
  void handle_events();
};

class tap_manager final {

public:
  tap_manager() {}
  ~tap_manager();

  int create_tapdev(uint32_t port_id, const std::string &port_name,
                    switch_callback &callback);

  int destroy_tapdev(uint32_t port_id, const std::string &port_name);

  void destroy_tapdevs();

  int enqueue(uint32_t port_id, basebox::packet *pkt);

  std::map<std::string, uint32_t> get_registered_ports() const {
    std::lock_guard<std::mutex> lock(rp_mutex);
    return tap_names;
  }

  uint32_t get_port_id(int ifindex) const noexcept {
    auto it = ifindex_to_id.find(ifindex);
    if (it == ifindex_to_id.end()) {
      return 0;
    } else {
      return it->second;
    }
  }

  int get_ifindex(uint32_t port_id) const noexcept {
    auto it = id_to_ifindex.find(port_id);
    if (it == id_to_ifindex.end()) {
      return 0;
    } else {
      return it->second;
    }
  }

  void tap_dev_ready(int ifindex, const std::string &name);
  void tap_dev_removed(int ifindex);

private:
  tap_manager(const tap_manager &other) = delete; // non construction-copyable
  tap_manager &operator=(const tap_manager &) = delete; // non copyable

  std::map<uint32_t, ctapdev *> tap_devs; // southbound id:tap_device
  mutable std::mutex rp_mutex;
  std::map<std::string, uint32_t> tap_names;

  std::map<int, uint32_t> ifindex_to_id;
  std::map<uint32_t, int> id_to_ifindex;
  std::deque<uint32_t> port_deleted;

  basebox::tap_io io;
};

} // namespace basebox
