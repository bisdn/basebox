#pragma once

#include <memory>
#include <grpc++/grpc++.h>

#include "ofdpa.grpc.pb.h"

namespace basebox {

class ofdpa_client {
public:
  ofdpa_client(std::shared_ptr<grpc::Channel> channel)
      : stub_(ofdpa::OfdpaRpc::NewStub(channel)) {}

  ofdpa::OfdpaStatus::OfdpaStatusCode
  ofdpaTunnelTenantCreate(uint32_t tunnel_id,
                          uint32_t vni) { // TODO maybe use ofdpa_datatypes here

    ::ofdpa::TunnelTenantCreate request;
    request.set_tunnel_id(tunnel_id);

    ::ofdpa::OfdpaTunnelTenantConfig *config = request.mutable_config();
    config->set_proto(::ofdpa::OFDPA_TUNNEL_PROTO_VXLAN);
    config->set_virtual_network_id(vni);

    ::ofdpa::OfdpaStatus response;
    ::grpc::ClientContext context;

    ::grpc::Status rv =
        stub_->ofdpaTunnelTenantCreate(&context, request, &response);

    if (not rv.ok()) {
      // LOG status
      return ofdpa::OfdpaStatus::OFDPA_E_RPC;
    }

    return response.status();
  }

private:
  std::unique_ptr<ofdpa::OfdpaRpc::Stub> stub_;
};

} // namespace basebox
