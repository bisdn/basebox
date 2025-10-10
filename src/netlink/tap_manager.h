// SPDX-FileCopyrightText: © 2016 BISDN GmbH
// SPDX-License-Identifier: MPL-2.0-no-copyleft-exception

#pragma once

#include <deque>
#include <string>
#include <map>
#include <memory>
#include <mutex>

#include "port_manager.h"
#include "sai.h"
#include <linux/ethtool.h>

extern "C" {
struct rtnl_link;
}

namespace basebox {

class ctapdev;
class tap_io;
class tap_manager;

class tap_manager final : public port_manager {

public:
  tap_manager();
  ~tap_manager();

  int create_portdev(uint32_t port_id, const std::string &port_name,
                     const rofl::caddress_ll &hwaddr,
                     switch_callback &callback);

  int destroy_portdev(uint32_t port_id, const std::string &port_name);

  int enqueue(uint32_t port_id, basebox::packet *pkt);

  int change_port_status(const std::string name, bool status);
  int set_port_speed(const std::string name, uint32_t speed, uint8_t duplex);
  int set_offloaded(rtnl_link *link, bool offloaded);

  // access from northbound (cnetlink)
  bool portdev_removed(rtnl_link *link);
  bool portdev_ready(rtnl_link *link);
  int update_mtu(rtnl_link *link);

private:
  tap_manager(const tap_manager &other) = delete; // non construction-copyable
  tap_manager &operator=(const tap_manager &) = delete; // non copyable

  std::map<std::string, int> tap_names2fds;

  // only accessible from southbound
  std::map<uint32_t, ctapdev *> tap_devs; // port id:tap_device
  std::deque<uint32_t> port_deleted;

  std::unique_ptr<tap_io> io;

  int recreate_tapdev(int ifindex, const std::string &portname,
                      const rofl::caddress_ll &hwaddr);

  int get_fd(uint32_t port_id) const noexcept;
};

} // namespace basebox
