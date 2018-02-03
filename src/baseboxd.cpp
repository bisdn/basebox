/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "basebox_api.h"
#include "netlink/nbi_impl.hpp"
#include "netlink/tap_manager.hpp"
#include "of-dpa/controller.hpp"

static bool validate_port(const char *flagname, gflags::int32 value) {
  if (value > 0 && value <= UINT16_MAX) // value is ok
    return true;
  return false;
}

DEFINE_int32(port, 6653, "Listening port");

int main(int argc, char **argv) {
  if (!gflags::RegisterFlagValidator(&FLAGS_port, &validate_port)) {
    std::cerr << "Failed to register port validator" << std::endl;
    exit(1);
  }

  gflags::SetUsageMessage("");
  gflags::SetVersionString(PACKAGE_VERSION);

  gflags::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  rofl::openflow::cofhello_elem_versionbitmap versionbitmap;
  versionbitmap.add_ofp_version(rofl::openflow13::OFP_VERSION);
  LOG(INFO) << "using OpenFlow version-bitmap:" << std::endl << versionbitmap;

  basebox::nbi_impl *nbi = new basebox::nbi_impl();
  std::shared_ptr<basebox::controller> box(
      new basebox::controller(nbi, versionbitmap));

  rofl::csockaddr baddr(AF_INET, std::string("0.0.0.0"), FLAGS_port);
  box->dpt_sock_listen(baddr);

  basebox::ApiServer grpcConnector(box);
  grpcConnector.runGRPCServer();

  delete nbi;

  LOG(INFO) << "bye";
  return EXIT_SUCCESS;
}
