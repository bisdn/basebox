/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <deque>
#include <exception>

#include <rofl/common/cthread.hpp>
#include <rofl/common/cpacket.h>

namespace basebox {

class ctapdev {
  int fd; // tap device file descriptor
  std::string devname;

public:
  /**
   *
   * @param devname
   */
  ctapdev(std::string const &devname);

  /**
   *
   */
  virtual ~ctapdev();

  const std::string &get_devname() const { return devname; }

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
