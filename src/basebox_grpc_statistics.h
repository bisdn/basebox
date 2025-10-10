// SPDX-FileCopyrightText: © 2017 BISDN GmbH
//
// SPDX-License-Identifier: MPL-2.0-no-copyleft-exception

#pragma once

#include <grpcpp/grpcpp.h>

#include "statistics/statistics-service.grpc.pb.h"

namespace basebox {

// forward declarations
class switch_interface;
class port_manager;

class NetworkStats final : public ::api::NetworkStatistics::Service {
public:
  typedef ::empty::Empty Empty;

  NetworkStats(std::shared_ptr<switch_interface> swi,
               std::shared_ptr<port_manager> port_man);

  virtual ~NetworkStats(){};

  ::grpc::Status
  GetStatistics(::grpc::ServerContext *context, const Empty *request,
                openconfig_interfaces::Interfaces *response) override;

private:
  std::shared_ptr<switch_interface> swi;
  std::shared_ptr<port_manager> port_man;
};

} // namespace basebox
