#include "roflibs/netlink/tap_manager.hpp"

namespace rofcore {

tap_manager::~tap_manager() {
  destroy_tapdevs();
}

std::deque<std::pair<int, std::string>>
tap_manager::create_tapdevs(std::deque<std::string> &port_names, tap_callback &cb) {
  std::deque<std::pair<int, std::string>> r;

  for (auto &port_name : port_names) {
    int i = create_tapdev(port_name, cb);
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
  ctapdev *dev;
  try {
    dev = new ctapdev(cb, port_name);
  } catch (std::exception &e) {
    return -EINVAL;
  }
  int r = devs.size();
  devs.push_back(dev);

  return r;
}

void tap_manager::destroy_tapdevs() {
  for (auto &dev : devs) {
    delete dev;
  }
  devs.clear();
}

} // namespace rofcore
