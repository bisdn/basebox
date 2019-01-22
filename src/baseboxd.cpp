/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "basebox_api.h"
#include "netlink/cnetlink.hpp"
#include "netlink/nbi_impl.hpp"
#include "netlink/tap_manager.hpp"
#include "of-dpa/controller.hpp"
#include "version.h"

DECLARE_string(tryfromenv); // from gflags
DEFINE_int32(port, 6653, "Listening port");
DEFINE_int32(ofdpa_grpc_port, 50051, "Listening port of ofdpa gRPC server");

static bool validate_port(const char *flagname, gflags::int32 value) {
  VLOG(3) << __FUNCTION__ << ": flagname=" << flagname << ", value=" << value;
  if (value > 0 && value <= UINT16_MAX) // value is ok
    return true;
  return false;
}

int main(int argc, char **argv) {
  using basebox::cnetlink;
  using basebox::controller;
  using basebox::nbi_impl;
  using basebox::tap_manager;

  if (!gflags::RegisterFlagValidator(&FLAGS_port, &validate_port)) {
    std::cerr << "Failed to register port validator" << std::endl;
    exit(1);
  }

  if (!gflags::RegisterFlagValidator(&FLAGS_ofdpa_grpc_port, &validate_port)) {
    std::cerr << "Failed to register port validator 2" << std::endl;
    exit(1);
  }

  // all variables can be set from env
  FLAGS_tryfromenv = std::string("port,ofdpa_grpc_port");
  gflags::SetUsageMessage("");
  gflags::SetVersionString(PROJECT_VERSION);

  // init
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  rofl::openflow::cofhello_elem_versionbitmap versionbitmap;
  versionbitmap.add_ofp_version(rofl::openflow13::OFP_VERSION);
  LOG(INFO) << __FUNCTION__
            << ": using OpenFlow version-bitmap: " << versionbitmap;

  std::shared_ptr<cnetlink> nl(new cnetlink());
  std::shared_ptr<tap_manager> tap_man(new tap_manager(nl));
  std::unique_ptr<nbi_impl> nbi(new nbi_impl(nl, tap_man));
  std::shared_ptr<controller> box(
      new controller(std::move(nbi), versionbitmap, FLAGS_ofdpa_grpc_port));

  rofl::csockaddr baddr(AF_INET, std::string("0.0.0.0"), FLAGS_port);
  box->dpt_sock_listen(baddr);

  basebox::ApiServer grpcConnector(box, tap_man);
  grpcConnector.runGRPCServer();

  LOG(INFO) << "bye";
  return EXIT_SUCCESS;
}
