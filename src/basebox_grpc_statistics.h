/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <grpcpp/grpcpp.h>

#include "statistics/statistics-service.grpc.pb.h"

namespace basebox {

// forward declarations
class switch_interface;
class tap_manager;

class NetworkStats final : public ::api::NetworkStatistics::Service {
public:
  typedef ::empty::Empty Empty;

  NetworkStats(std::shared_ptr<switch_interface> swi,
               std::shared_ptr<tap_manager> tap_man);

  virtual ~NetworkStats(){};

  ::grpc::Status
  GetStatistics(::grpc::ServerContext *context, const Empty *request,
                openconfig_interfaces::Interfaces *response) override;

private:
  std::shared_ptr<switch_interface> swi;
  std::shared_ptr<tap_manager> tap_man;
};

} // namespace basebox
