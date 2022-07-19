/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>

#include <rofl/common/caddress.h>

namespace basebox {

class ctapdev {
  int fd; // tap device file descriptor
  std::string devname;
  rofl::caddress_ll hwaddr;

public:
  /**
   *
   * @param devname
   */
  ctapdev(std::string const &devname, const rofl::caddress_ll &hwaddr);

  /**
   *
   */
  virtual ~ctapdev();

  const std::string &get_devname() const { return devname; }

  const rofl::caddress_ll &get_hwaddr() const { return hwaddr; }

  /**
   * @brief	open tapX device
   */
  void tap_open();

  /**
   * @brief	close tapX device
   *
   */
  void tap_close();

  int get_fd() const { return fd; }
};

} // end of namespace basebox
