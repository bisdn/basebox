/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <glog/logging.h>
#include "roflibs/netlink/tap_manager.hpp"

namespace rofcore {

tap_manager::~tap_manager() { destroy_tapdevs(); }

int tap_manager::create_tapdev(uint32_t port_id, const std::string &port_name,
                               tap_callback &cb) {
  int r = 0;
  auto it = devs.find(port_id);
  if (it == devs.end()) {
    ctapdev *dev;
    try {
      dev = new ctapdev(cb, port_name, port_id);
      devs.insert(std::make_pair(port_id, dev));
      dev->tap_open();
    } catch (std::exception &e) {
      LOG(ERROR) << __FUNCTION__ << ": failed to create tapdev " << port_name;
      r = -EINVAL;
    }
  } else {
    LOG(INFO) << __FUNCTION__ << ": " << port_name
              << " with port_id=" << port_id << " already existing";
  }
  return r;
}

int tap_manager::destroy_tapdev(uint32_t port_id,
                                const std::string &port_name) {
  auto it = devs.find(port_id);
  if (it == devs.end()) {
    return 0;
  }

  auto dev = it->second;
  devs.erase(it);
  delete dev;

  return 0;
}

void tap_manager::destroy_tapdevs() {
  std::map<uint32_t, ctapdev *> ddevs;
  ddevs.swap(devs);
  for (auto &dev : ddevs) {
    delete dev.second;
  }
}

} // namespace rofcore
