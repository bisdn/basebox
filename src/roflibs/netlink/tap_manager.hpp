/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <deque>
#include <string>
#include <vector>
#include <map>

#include <rofl/common/cpacket.h>

#include "roflibs/netlink/ctapdev.hpp"
#include "roflibs/netlink/sai.hpp"

namespace rofcore {

class tap_callback {
public:
  //  virtual ~tap_callback(){};
  virtual int enqueue_to_switch(uint32_t port_id, rofl::cpacket *) = 0;
};

class tap_manager final {

public:
  tap_manager(){};
  ~tap_manager();

  /**
   * create tap devices for each unique name in queue.
   *
   * @return: pairs of ID,dev_name of the created devices
   */
  std::map<uint32_t, int>
  register_tapdevs(std::deque<nbi::port_notification_data> &, tap_callback &);

  uint32_t get_tapdev_id(uint32_t port_id) const {
    return port_id_to_tapdev_id.at(port_id);
  }

  void start();

  void destroy_tapdevs();

  ctapdev &get_dev(int i) { return *devs[i]; }

private:
  tap_manager(const tap_manager &other) = delete; // non construction-copyable
  tap_manager &operator=(const tap_manager &) = delete; // non copyable

  int create_tapdev(uint32_t port_id, const std::string &port_name,
                    tap_callback &callback);

  std::vector<ctapdev *> devs;
  std::map<uint32_t, int> port_id_to_tapdev_id;
};

} // namespace rofcore
