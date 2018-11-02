/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "basebox_api.h"
#include "netlink/nbi_impl.hpp"
#include "netlink/tap_manager.hpp"
#include "netlink/tap_manager.hpp"
#include "of-dpa/controller.hpp"
#include "version.h"

DECLARE_string(tryfromenv); // from gflags
DEFINE_int32(port, 6653, "Listening port");

static bool validate_port(const char *flagname, gflags::int32 value) {
  VLOG(3) << __FUNCTION__ << ": flagname=" << flagname << ", value=" << value;
  if (value > 0 && value <= UINT16_MAX) // value is ok
    return true;
  return false;
}

int main(int argc, char **argv) {
  using basebox::controller;
  using basebox::nbi_impl;
  using basebox::tap_manager;

  if (!gflags::RegisterFlagValidator(&FLAGS_port, &validate_port)) {
    std::cerr << "Failed to register port validator" << std::endl;
    exit(1);
  }

  // all variables can be set from env
  FLAGS_tryfromenv = std::string("port");
  gflags::SetUsageMessage("");
  gflags::SetVersionString(PROJECT_VERSION);

  // init
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  rofl::openflow::cofhello_elem_versionbitmap versionbitmap;
  versionbitmap.add_ofp_version(rofl::openflow13::OFP_VERSION);
  LOG(INFO) << __FUNCTION__
            << ": using OpenFlow version-bitmap: " << versionbitmap;

  nbi_impl *nbi = new nbi_impl();
  std::shared_ptr<controller> box(new controller(nbi, versionbitmap));

  rofl::csockaddr baddr(AF_INET, std::string("0.0.0.0"), FLAGS_port);
  box->dpt_sock_listen(baddr);

  basebox::ApiServer grpcConnector(box, nbi->get_tapmanager());
  grpcConnector.runGRPCServer();

  delete nbi;

  LOG(INFO) << "bye";
  return EXIT_SUCCESS;
}
