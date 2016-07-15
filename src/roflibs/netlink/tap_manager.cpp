/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "roflibs/netlink/tap_manager.hpp"
#include "roflibs/netlink/cnetlink.hpp"

namespace rofcore {

tap_manager::~tap_manager() { destroy_tapdevs(); }

void tap_manager::start() {
  cnetlink::get_instance().start();
  for (auto dev : devs) {
    dev->tap_open();
  }
}

void tap_manager::stop() { cnetlink::get_instance().stop(); }

std::deque<std::pair<int, std::string>>
tap_manager::register_tapdevs(std::deque<std::string> &port_names,
                              tap_callback &cb) {
  std::deque<std::pair<int, std::string>> r;
  int i = 0;

  for (auto &port_name : port_names) {
    i = create_tapdev(port_name, cb);

    if (i < 0) {
      destroy_tapdevs();
      r.clear();
      break;
    } else {
      r.push_back(std::make_pair(i, std::move(port_name)));
    }
  }

  return r;
}

int tap_manager::create_tapdev(const std::string &port_name, tap_callback &cb) {
  int r;
  auto it = devname_to_spot.find(port_name);
  if (it != devname_to_spot.end()) {
    r = it->second;
  } else {
    ctapdev *dev;
    try {
      dev = new ctapdev(cb, port_name);
    } catch (std::exception &e) {
      // TODO log error
      return -EINVAL;
    }
    r = devs.size();
    devs.push_back(dev);

    cnetlink::get_instance().register_link(r, port_name);
    devname_to_spot.insert(std::make_pair(port_name, r));
  }
  return r;
}

void tap_manager::destroy_tapdevs() {
  std::vector<ctapdev *> ddevs = std::move(devs);
  for (auto &dev : ddevs) {
    delete dev;
  }
  devname_to_spot.clear();
}

} // namespace rofcore
