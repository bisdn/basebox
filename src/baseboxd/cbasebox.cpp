/*
 * crofbase.cpp
 *
 *  Created on: 05.08.2014
 *      Author: andreas
 */

#include "cbasebox.hpp"
#include <assert.h>
#include "cunixenv.hpp"

using namespace basebox;

/*static*/ bool cbasebox::keep_on_running = true;

/*static*/ int cbasebox::run(int argc, char **argv) {
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

  rofcore::logging::set_debug_level(core_debug);

  rofl::openflow::cofhello_elem_versionbitmap versionbitmap;
  versionbitmap.add_ofp_version(rofl::openflow13::OFP_VERSION);
  rofcore::logging::notice << "[baseboxd][main] using OpenFlow version-bitmap:"
                           << std::endl
                           << versionbitmap;
  basebox::cbasebox &box = basebox::cbasebox::get_instance(versionbitmap);

  std::stringstream portno;
  if (env_parser.is_arg_set("port")) {
    portno << env_parser.get_arg("port").c_str();
  } else {
    portno << (int)6653;
  }

  rofl::csockaddr baddr(AF_INET, std::string("0.0.0.0"),
                        atoi(portno.str().c_str()));
  box.dpt_sock_listen(baddr);

  // start netlink
  (void)rofcore::cnetlink::get_instance();

  while (keep_on_running) {
    try {
      // Launch main I/O loop
      struct timespec ts;
      ts.tv_sec = 5;
      ts.tv_nsec = 0;
      pselect(0, NULL, NULL, NULL, &ts, NULL);

    } catch (std::exception &e) {
      std::cerr << "exception caught, what: " << e.what() << std::endl;
    }
  }

  return EXIT_SUCCESS;
}

/*static*/
void cbasebox::stop() {
  cbasebox::keep_on_running = false;
  //	thread.stop();
}

void cbasebox::handle_dpt_open(rofl::crofdpt &dpt) {

  if (rofl::openflow13::OFP_VERSION < dpt.get_version()) {
    rofcore::logging::error
        << "[cbasebox][handle_dpt_open] datapath "
        << "attached with invalid OpenFlow protocol version: "
        << (int)dpt.get_version() << std::endl;
    return;
  }

  if (sa) {
    delete sa;
    sa = NULL;
  }

#ifdef DEBUG
  dpt.set_conn(rofl::cauxid(0))
      .set_trace(true)
      .set_journal()
      .log_on_stderr(true)
      .set_max_entries(64);
  dpt.set_conn(rofl::cauxid(0))
      .set_tcp_journal()
      .log_on_stderr(true)
      .set_max_entries(16);
#endif

  rofcore::logging::debug << "[cbasebox][handle_dpt_open] dpid: "
                          << dpt.get_dpid().str() << std::endl;
  rofcore::logging::debug << "[cbasebox][handle_dpt_open] dpt: " << dpt
                          << std::endl;

  dpt.send_features_request(rofl::cauxid(0));
  dpt.send_desc_stats_request(rofl::cauxid(0), 0);
  dpt.send_port_desc_stats_request(rofl::cauxid(0), 0);

  // todo timeout?
}

void cbasebox::handle_dpt_close(const rofl::cdptid &dptid) {

  rofcore::logging::debug << "[cbasebox][handle_dpt_close] dpid: " << dpid.str()
                          << std::endl;

  // FIXME implemente close
  if (sa) {
    sa->handle_dpt_close(dptid);
  }
}

void cbasebox::handle_conn_terminated(rofl::crofdpt &dpt,
                                      const rofl::cauxid &auxid) {
  rofcore::logging::crit << "[cbasebox][" << __FUNCTION__
                         << "]: XXX not implemented" << std::endl;
}

void cbasebox::handle_conn_refused(rofl::crofdpt &dpt,
                                   const rofl::cauxid &auxid) {
  rofcore::logging::crit << "[cbasebox][" << __FUNCTION__
                         << "]: XXX not implemented" << std::endl;
}

void cbasebox::handle_conn_failed(rofl::crofdpt &dpt,
                                  const rofl::cauxid &auxid) {
  rofcore::logging::crit << "[cbasebox][" << __FUNCTION__
                         << "]: XXX not implemented" << std::endl;
}

void cbasebox::handle_conn_negotiation_failed(rofl::crofdpt &dpt,
                                              const rofl::cauxid &auxid) {
  rofcore::logging::crit << "[cbasebox][" << __FUNCTION__
                         << "]: XXX not implemented" << std::endl;
}

void cbasebox::handle_conn_congestion_occured(rofl::crofdpt &dpt,
                                              const rofl::cauxid &auxid) {
  rofcore::logging::crit << "[cbasebox][" << __FUNCTION__
                         << "]: XXX not implemented" << std::endl;
}

void cbasebox::handle_conn_congestion_solved(rofl::crofdpt &dpt,
                                             const rofl::cauxid &auxid) {
  rofcore::logging::crit << "[cbasebox][" << __FUNCTION__
                         << "]: XXX not implemented" << std::endl;
}

void cbasebox::handle_features_reply(
    rofl::crofdpt &dpt, const rofl::cauxid &auxid,
    rofl::openflow::cofmsg_features_reply &msg) {
  rofcore::logging::debug << "[cbasebox][handle_features_reply] dpid: "
                          << dpt.get_dpid().str() << std::endl
                          << msg;
}

void cbasebox::handle_desc_stats_reply(
    rofl::crofdpt &dpt, const rofl::cauxid &auxid,
    rofl::openflow::cofmsg_desc_stats_reply &msg) {
  rofcore::logging::debug << "[cbasebox][handle_desc_stats_reply] dpt: "
                          << std::endl
                          << dpt << std::endl
                          << msg;
}

void cbasebox::handle_packet_in(rofl::crofdpt &dpt, const rofl::cauxid &auxid,
                                rofl::openflow::cofmsg_packet_in &msg) {
  rofcore::logging::debug << "[cbasebox][handle_packet_in] dpid: "
                          << dpt.get_dpid().str()
                          << " pkt received: " << std::endl
                          << msg;
  if (sa) {
    sa->handle_packet_in(dpt, auxid, msg);
  }
}

void cbasebox::handle_flow_removed(rofl::crofdpt &dpt,
                                   const rofl::cauxid &auxid,
                                   rofl::openflow::cofmsg_flow_removed &msg) {
  rofcore::logging::debug << "[cbasebox][handle_flow_removed] dpid: "
                          << dpt.get_dpid().str()
                          << " pkt received: " << std::endl
                          << msg;

  if (sa) {
    sa->handle_flow_removed(dpt, auxid, msg);
  }
}

void cbasebox::handle_port_status(rofl::crofdpt &dpt, const rofl::cauxid &auxid,
                                  rofl::openflow::cofmsg_port_status &msg) {
  rofcore::logging::debug << "[cbasebox][handle_port_status] dpid: "
                          << dpt.get_dpid().str()
                          << " pkt received: " << std::endl
                          << msg;

  if (sa) {
    sa->handle_port_status(dpt, auxid, msg);
  }
}

void cbasebox::handle_error_message(rofl::crofdpt &dpt,
                                    const rofl::cauxid &auxid,
                                    rofl::openflow::cofmsg_error &msg) {
  rofcore::logging::info << "[cbasebox][handle_error_message] dpid: "
                         << dpt.get_dpid().str()
                         << " pkt received: " << std::endl
                         << msg;

  if (sa) {
    sa->handle_error_message(dpt, auxid, msg);
  }
}

void cbasebox::handle_port_desc_stats_reply(
    rofl::crofdpt &dpt, const rofl::cauxid &auxid,
    rofl::openflow::cofmsg_port_desc_stats_reply &msg) {

  rofcore::logging::debug << "[cbasebox][handle_port_desc_stats_reply] dpid: "
                          << dpt.get_dpid().str()
                          << " pkt received: " << std::endl
                          << msg;

  /* init behavior */ // todo behavior based on features/descs/stats
  switch_behavior *tmp = this->sa;
  this->sa = switch_behavior_fabric::get_behavior(1, dpt);
  assert(this->sa);
  this->sa->init();

  if (tmp) {
    delete tmp;
  }
}

void cbasebox::handle_port_desc_stats_reply_timeout(rofl::crofdpt &dpt,
                                                    uint32_t xid) {

  rofcore::logging::debug
      << "[cbasebox][handle_port_desc_stats_reply_timeout] dpid: "
      << dpt.get_dpid().str() << std::endl;
}

void cbasebox::handle_experimenter_message(
    rofl::crofdpt &dpt, const rofl::cauxid &auxid,
    rofl::openflow::cofmsg_experimenter &msg) {

  rofcore::logging::debug << "[cbasebox][" << __FUNCTION__
                          << "] dpid: " << dpt.get_dpid().str()
                          << " pkt received: " << std::endl
                          << msg;
  if (sa) {
    sa->handle_experimenter_message(dpt, auxid, msg);
  }
}
