/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "roflibs/netlink/cnetlink.hpp"
#include "roflibs/of-dpa/cbasebox.hpp"

static bool validate_port(const char *flagname, google::int32 value) {
  if (value > 0 && value < 32768) // value is ok
    return true;
  return false;
}

DEFINE_int32(port, 6653, "Listening port");

int main(int argc, char **argv) {

  if (!google::RegisterFlagValidator(&FLAGS_port, &validate_port)) {
    std::cerr << "Failed to register port validator" << std::endl;
    exit(1);
  }

  gflags::SetUsageMessage("");
  gflags::SetVersionString(PACKAGE_VERSION);

  gflags::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  rofl::openflow::cofhello_elem_versionbitmap versionbitmap;
  versionbitmap.add_ofp_version(rofl::openflow13::OFP_VERSION);
  LOG(INFO) << "[baseboxd][main] using OpenFlow version-bitmap:" << std::endl
            << versionbitmap;

  // start netlink
  rofcore::nbi *nbi = &rofcore::cnetlink::get_instance();
  // XXX FIXME move tap_manager here
  std::unique_ptr<basebox::cbasebox> box(
      new basebox::cbasebox(nbi, versionbitmap));


  rofl::csockaddr baddr(AF_INET, std::string("0.0.0.0"),
                        FLAGS_port);
  box->dpt_sock_listen(baddr);

  while (basebox::cbasebox::running()) {
    try {
      // Launch main I/O loop
      struct timeval tv;
      tv.tv_sec = 10;
      tv.tv_usec = 0;
      select(0, NULL, NULL, NULL, &tv);

    } catch (std::exception &e) {
      std::cerr << "exception caught, what: " << e.what() << std::endl;
    }
  }

  return EXIT_SUCCESS;
}
