#pragma once

#include <grpc++/server.h>
#include "sai.hpp"

namespace basebox {

class NetworkStats;

class ApiServer final {
public:
  ApiServer(std::shared_ptr<switch_interface> swi);
  void runGRPCServer();
  ~ApiServer();

private:
  NetworkStats *stats;
  std::unique_ptr<::grpc::Server> server;
};

} // namespace basebox
