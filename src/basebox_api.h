// SPDX-FileCopyrightText: © 2017 BISDN GmbH
// SPDX-License-Identifier: MPL-2.0-no-copyleft-exception

#pragma once

#include <memory>

namespace basebox {

// forward declarations
class NetworkStats;
class switch_interface;
class port_manager;

class ApiServer final {
public:
  ApiServer(std::shared_ptr<switch_interface> swi,
            std::shared_ptr<port_manager> port_man);
  void runGRPCServer();
  ~ApiServer();

private:
  NetworkStats *stats;
};

} // namespace basebox
