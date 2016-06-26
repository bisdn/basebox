#include <deque>
#include <string>
#include <vector>

#include <rofl/common/cpacket.h>

#include "roflibs/netlink/ctapdev.hpp"

#pragma once

namespace rofcore {

class tap_callback {
public:
  virtual ~tap_callback(){};
  virtual int enqueue(rofcore::ctapdev *, rofl::cpacket *) = 0;
};

class tap_manager {

public:
  tap_manager(){};
  ~tap_manager();

  /**
   * create tap devices for each unique name in queue.
   *
   * @return: pairs of ID,dev_name of the created devices
   */
  std::deque<std::pair<int, std::string>>
  create_tapdevs(std::deque<std::string> &, tap_callback &);

  int create_tapdev(const std::string &, tap_callback &);

  void destroy_tapdevs();

  ctapdev &get_dev(int i) { return *devs[i]; }

private:
  tap_manager(const tap_manager &other) = delete; // non construction-copyable
  tap_manager &operator=(const tap_manager &) = delete; // non copyable

  std::vector<ctapdev*> devs;
};

} // namespace rofcore
