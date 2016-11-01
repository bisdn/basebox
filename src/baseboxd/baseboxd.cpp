/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "roflibs/netlink/nbi_impl.hpp"
#include "roflibs/netlink/tap_manager.hpp"
#include "roflibs/of-dpa/cbasebox.hpp"

static volatile sig_atomic_t got_SIGINT = 0;

static bool validate_port(const char *flagname, gflags::int32 value) {
  if (value > 0 && value < 32768) // value is ok
    return true;
  return false;
}

DEFINE_int32(port, 6653, "Listening port");

static void int_sig_handler(int sig) { got_SIGINT = 1; }

int main(int argc, char **argv) {
  bool running = true;

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

  // block sigint to establish handler
  sigset_t sigset;
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGINT);
  if (sigprocmask(SIG_BLOCK, &sigset, NULL) < 0) {
    LOG(FATAL) << __FUNCTION__ << ": sigprocmask failed to block SIGINT";
  }

  struct sigaction sa;
  sa.sa_handler = int_sig_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART; /* Restart functions if interrupted by handler */

  if (sigaction(SIGINT, &sa, NULL) < 0) {
    LOG(FATAL) << __FUNCTION__ << ": failed to set signal handler for SIGINT";
  }

  sigemptyset(&sigset);

  rofcore::nbi_impl *nbi = new rofcore::nbi_impl();
  std::unique_ptr<basebox::cbasebox> box(
      new basebox::cbasebox(nbi, versionbitmap));

  rofl::csockaddr baddr(AF_INET, std::string("0.0.0.0"), FLAGS_port);
  box->dpt_sock_listen(baddr);

  while (running) {
    try {
      // Launch main I/O loop
      struct timespec ts;
      ts.tv_sec = 10;
      ts.tv_nsec = 0;
      pselect(0, NULL, NULL, NULL, &ts, &sigset);

      if (got_SIGINT) {
        got_SIGINT = 0;
        running = false;
        LOG(INFO) << "received SIGINT, shutting down";
      }

    } catch (std::exception &e) {
      std::cerr << "exception caught, what: " << e.what() << std::endl;
    }
  }

  delete nbi;

  LOG(INFO) << "bye";
  return EXIT_SUCCESS;
}
