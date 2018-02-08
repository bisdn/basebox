#pragma once

#include <memory>
#include <grpc++/grpc++.h>

#include "ofdpa.grpc.pb.h"

namespace basebox {

class ofdpa_client {
public:
  ofdpa_client(std::shared_ptr<grpc::Channel> channel)
      : stub_(ofdpa::OfdpaRpc::NewStub(channel)) {}

private:
  std::unique_ptr<ofdpa::OfdpaRpc::Stub> stub_;
};

} // namespace basebox
