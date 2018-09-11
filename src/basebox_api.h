/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <memory>

namespace basebox {

// forward declarations
class NetworkStats;
class switch_interface;
class tap_manager;

class ApiServer final {
public:
  ApiServer(std::shared_ptr<switch_interface> swi,
            std::shared_ptr<tap_manager> tap_man);
  void runGRPCServer();
  ~ApiServer();

private:
  NetworkStats *stats;
};

} // namespace basebox
