/*
 * cbasebox.hpp
 *
 *  Created on: 05.08.2014
 *      Author: andreas
 */

#ifndef CROFBASE_HPP_
#define CROFBASE_HPP_

#define OF_DPA

#include <sys/types.h>
#include <sys/wait.h>

#include <string>
#include <iostream>
#include <exception>
#include <rofl/common/crofbase.h>

#include "roflibs/netlink/clogging.hpp"
#include "roflibs/netlink/cnetlink.hpp"
#include "roflibs/netlink/ctundev.hpp"
#include "roflibs/netlink/ccookiebox.hpp"

#include <baseboxd/switch_behavior.hpp>

namespace basebox {

class eBaseBoxBase : public std::runtime_error {
public:
  eBaseBoxBase(const std::string &__arg) : std::runtime_error(__arg) {}
};

static rofl::crofdpt invalid(NULL, rofl::cdptid(0));

class cbasebox : public rofl::crofbase, public virtual rofl::cthread_env {

  static bool keep_on_running;
  rofl::cthread thread;

  /**
   *
   */
  cbasebox(const rofl::openflow::cofhello_elem_versionbitmap &versionbitmap =
               rofl::openflow::cofhello_elem_versionbitmap())
      : thread(this), sa(switch_behavior_fabric::get_behavior(-1, invalid)) {
    rofl::crofbase::set_versionbitmap(versionbitmap);
    thread.start();
  }

  /**
   *
   */
  virtual ~cbasebox() {}

  /**
   *
   */
  cbasebox(const cbasebox &ethbase);

public:
  /**
   *
   */
  static cbasebox &get_instance(
      const rofl::openflow::cofhello_elem_versionbitmap &versionbitmap =
          rofl::openflow::cofhello_elem_versionbitmap()) {
    static cbasebox box(versionbitmap);
    return box;
  }

  /**
   *
   */
  static int run(int argc, char **argv);

  /**
   *
   */
  static void stop();

protected:
  virtual void handle_wakeup(rofl::cthread &thread) {}

  virtual void handle_conn_established(rofl::crofdpt &dpt,
                                       const rofl::cauxid &auxid) {
    dpt.set_conn(auxid).set_trace(true);

    crofbase::add_ctl(rofl::cctlid(0))
        .set_conn(rofl::cauxid(0))
        .set_trace(true)
        .set_journal()
        .log_on_stderr(true)
        .set_max_entries(64);

    crofbase::set_ctl(rofl::cctlid(0))
        .set_conn(rofl::cauxid(0))
        .set_tcp_journal()
        .log_on_stderr(true)
        .set_max_entries(16);
  }

  virtual void handle_dpt_open(rofl::crofdpt &dpt);

  virtual void handle_dpt_close(const rofl::cdptid &dptid);

  virtual void handle_conn_terminated(rofl::crofdpt &dpt,
                                      const rofl::cauxid &auxid);

  virtual void handle_conn_refused(rofl::crofdpt &dpt,
                                   const rofl::cauxid &auxid);

  virtual void handle_conn_failed(rofl::crofdpt &dpt,
                                  const rofl::cauxid &auxid);

  virtual void handle_conn_negotiation_failed(rofl::crofdpt &dpt,
                                              const rofl::cauxid &auxid);

  virtual void handle_conn_congestion_occured(rofl::crofdpt &dpt,
                                              const rofl::cauxid &auxid);

  virtual void handle_conn_congestion_solved(rofl::crofdpt &dpt,
                                             const rofl::cauxid &auxid);

  virtual void
  handle_features_reply(rofl::crofdpt &dpt, const rofl::cauxid &auxid,
                        rofl::openflow::cofmsg_features_reply &msg);

  virtual void
  handle_desc_stats_reply(rofl::crofdpt &dpt, const rofl::cauxid &auxid,
                          rofl::openflow::cofmsg_desc_stats_reply &msg);

  virtual void handle_packet_in(rofl::crofdpt &dpt, const rofl::cauxid &auxid,
                                rofl::openflow::cofmsg_packet_in &msg);

  virtual void handle_flow_removed(rofl::crofdpt &dpt,
                                   const rofl::cauxid &auxid,
                                   rofl::openflow::cofmsg_flow_removed &msg);

  virtual void handle_port_status(rofl::crofdpt &dpt, const rofl::cauxid &auxid,
                                  rofl::openflow::cofmsg_port_status &msg);

  virtual void handle_error_message(rofl::crofdpt &dpt,
                                    const rofl::cauxid &auxid,
                                    rofl::openflow::cofmsg_error &msg);

  virtual void handle_port_desc_stats_reply(
      rofl::crofdpt &dpt, const rofl::cauxid &auxid,
      rofl::openflow::cofmsg_port_desc_stats_reply &msg);

  virtual void handle_port_desc_stats_reply_timeout(rofl::crofdpt &dpt,
                                                    uint32_t xid);

public:
  friend std::ostream &operator<<(std::ostream &os, const cbasebox &box) {
    os << rofcore::indent(0) << "<cbasebox>" << std::endl;
    return os;
  }

private:
  rofl::cdpid dpid;

  // behavior of the switch (currently only a single switch)
  switch_behavior *sa;
};

} // end of namespace ethcore

#endif /* CROFBASE_HPP_ */
