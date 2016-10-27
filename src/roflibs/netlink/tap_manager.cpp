/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "roflibs/netlink/tap_manager.hpp"
#include "roflibs/netlink/cnetlink.hpp"

namespace rofcore {

tap_manager::~tap_manager() { destroy_tapdevs(); }

void tap_manager::start() {
  for (auto dev : devs) {
    dev->tap_open();
  }
}

std::map<uint32_t, int> tap_manager::register_tapdevs(
    std::deque<nbi::port_notification_data> &notifications, tap_callback &cb) {
  int i = 0;

  for (auto &ntfi : notifications) {
    i = create_tapdev(ntfi.port_id, ntfi.name, cb);

    if (i < 0) {
      LOG(FATAL) << __FUNCTION__ << ": failed to create tapdev";
    }
  }

  return port_id_to_tapdev_id;
}

int tap_manager::create_tapdev(uint32_t port_id, const std::string &port_name,
                               tap_callback &cb) {
  int r;
  auto it = port_id_to_tapdev_id.find(port_id);
  if (it != port_id_to_tapdev_id.end()) {
    r = it->second;
  } else {
    ctapdev *dev;
    try {
      dev = new ctapdev(cb, port_name, port_id);
    } catch (std::exception &e) {
      // TODO log error
      return -EINVAL;
    }
    r = devs.size();
    devs.push_back(dev);

    cnetlink::get_instance().register_link(port_id, port_name);
    port_id_to_tapdev_id.insert(std::make_pair(port_id, r));
  }
  return r;
}

void tap_manager::destroy_tapdevs() {
  std::vector<ctapdev *> ddevs = std::move(devs);
  for (auto &dev : ddevs) {
    delete dev;
  }
  port_id_to_tapdev_id.clear();
}

} // namespace rofcore
