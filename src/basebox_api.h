#pragma once

#include <grpc++/server.h>

namespace basebox {

class NetworkStats;

class ApiServer final {
public:
  ApiServer();
  void runGRPCServer();
  ~ApiServer();

private:
  NetworkStats* stats; 
  std::unique_ptr<::grpc::Server> server;
  void initStructures();
};

} // namespace basebox
