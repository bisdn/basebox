/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "basebox_api.h"
#include "basebox_grpc_statistics.h"
#include "sai.h"

#include <glog/logging.h>
#include <google/protobuf/repeated_field.h>
#include <grpcpp/channel.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpc/grpc.h>

namespace basebox {

ApiServer::ApiServer(std::shared_ptr<switch_interface> swi,
                     std::shared_ptr<port_manager> port_man)
    : stats(new NetworkStats(swi, port_man)) {}

ApiServer::~ApiServer() { delete stats; }

void ApiServer::runGRPCServer() {
  std::string server_address("0.0.0.0:5000");
  ::grpc::ServerBuilder builder;

  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(stats);
  std::unique_ptr<::grpc::Server> server = builder.BuildAndStart();
  LOG(INFO) << "gRPC server listening on " << server_address;
  server->Wait();
}

} // namespace basebox
