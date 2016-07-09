/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "cunixenv.hpp"
#include "roflibs/netlink/clogging.hpp"
#include "roflibs/netlink/cnetlink.hpp"
#include "roflibs/of-dpa/cbasebox.hpp"

int main(int argc, char **argv) {
  using rofcore::logging;

  rofl::cunixenv env_parser(argc, argv);

  /* update defaults */
  env_parser.add_option(
      rofl::coption(true, REQUIRED_ARGUMENT, 'p', "port", "set port", "6653"));

  // command line arguments
  env_parser.parse_args();

  /*
   * extract help flag
   */
  if (env_parser.is_arg_set("help")) {
    std::cout << env_parser.get_usage((char *)"baseboxd");
    exit(0);
  }

  /*
   * extract debug level
   */
  int rofl_debug = 0, core_debug = 0;
  if (env_parser.is_arg_set("debug")) {
    core_debug = rofl_debug = atoi(env_parser.get_arg("debug").c_str());
  }

  logging::set_debug_level(core_debug);

  rofl::openflow::cofhello_elem_versionbitmap versionbitmap;
  versionbitmap.add_ofp_version(rofl::openflow13::OFP_VERSION);
  logging::notice << "[baseboxd][main] using OpenFlow version-bitmap:"
                  << std::endl
                  << versionbitmap;

  // start netlink
  rofcore::nbi *nbi = &rofcore::cnetlink::get_instance();
  // XXX FIXME move tap_manager here
  std::unique_ptr<basebox::cbasebox> box(
      new basebox::cbasebox(nbi, versionbitmap));

  std::stringstream portno;
  if (env_parser.is_arg_set("port")) {
    portno << env_parser.get_arg("port").c_str();
  } else {
    portno << (int)6653;
  }

  rofl::csockaddr baddr(AF_INET, std::string("0.0.0.0"),
                        atoi(portno.str().c_str()));
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
