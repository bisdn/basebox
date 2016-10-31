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

class tap_manager final {

public:
  tap_manager(){};
  ~tap_manager();

  int create_tapdev(uint32_t port_id, const std::string &port_name,
                    tap_callback &callback);

  int destroy_tapdev(uint32_t port_id, const std::string &port_name);

  void destroy_tapdevs();

  ctapdev *get_dev(uint32_t port_id) { return devs.at(port_id); }

private:
  tap_manager(const tap_manager &other) = delete; // non construction-copyable
  tap_manager &operator=(const tap_manager &) = delete; // non copyable

  std::map<uint32_t, ctapdev *> devs;
};

} // namespace rofcore
