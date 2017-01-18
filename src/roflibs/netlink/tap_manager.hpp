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

#include "roflibs/netlink/ctapdev.hpp"
#include "roflibs/netlink/sai.hpp"

namespace rofcore {

class tap_io;
class tap_manager;

class switch_callback {
public:
  virtual int enqueue_to_switch(uint32_t port_id, rofl::cpacket *) = 0;
};

class tap_io : public rofl::cthread_env {
  enum tap_io_event {
    TAP_IO_ADD,
    TAP_IO_REM,
  };

  rofl::cthread thread;
  std::deque<std::pair<int, rofl::cpacket *>> pout_queue;
  std::mutex pout_queue_mutex;

  std::deque<std::tuple<enum tap_io_event, int, uint32_t, switch_callback *>>
      events;
  std::mutex events_mutex;

  std::deque<std::pair<int, rofl::cpacket *>> pin_queue;
  std::map<int, std::pair<uint32_t, switch_callback *>> sw_cbs;

public:
  tap_io() : thread(this) { thread.start(); };
  virtual ~tap_io();

  // port_id should be removed at some point and be rather data
  void register_tap(int fd, uint32_t port_id, switch_callback &cb);
  void unregister_tap(int fd, uint32_t port_id);
  void enqueue(int fd, rofl::cpacket *pkt);

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

  int enqueue(uint32_t port_id, rofl::cpacket *pkt);

private:
  tap_manager(const tap_manager &other) = delete; // non construction-copyable
  tap_manager &operator=(const tap_manager &) = delete; // non copyable

  std::map<uint32_t, ctapdev *> devs;

  rofcore::tap_io io;
};

} // namespace rofcore
