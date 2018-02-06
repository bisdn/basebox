#pragma once

#include <grpc++/grpc++.h>

#include "services-definition.grpc.pb.h"

namespace basebox {

// forward declarations
class switch_interface;
class tap_manager;

class NetworkStats final : public ::api::NetworkStatistics::Service {
public:
  typedef ::api::Empty Empty;

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
