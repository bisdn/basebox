/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef CROFBASE_HPP_
#define CROFBASE_HPP_

#define OF_DPA

#include <sys/types.h>
#include <sys/wait.h>

#include <string>
#include <iostream>
#include <exception>

#include <rofl/common/crofbase.h>

#include "roflibs/netlink/cnetlink_observer.hpp"
#include "roflibs/netlink/tap_manager.hpp"
#include "roflibs/of-dpa/ofdpa_bridge.hpp"

namespace basebox {

class eBaseBoxBase : public std::runtime_error {
public:
  eBaseBoxBase(const std::string &__arg) : std::runtime_error(__arg) {}
};

static rofl::crofdpt invalid(NULL, rofl::cdptid(0));

class cbasebox : public rofl::crofbase,
                 public virtual rofl::cthread_env,
                 public rofcore::tap_callback,
                 public rofcore::auto_reg_cnetlink_common_observer {

  enum ExperimenterMessageType {
    QUERY_FLOW_ENTRIES, ///< query flow entries from controller
    RECEIVED_FLOW_ENTRIES_QUERY
  };

  enum ExperimenterId {
    BISDN = 0xFF0000B0 ///< should be registered as ONF-Managed Experimenter ID
                       ///(OUI)
  };

  static bool keep_on_running;
  rofl::cthread thread;

  /**
   *
   */
  cbasebox(const rofl::openflow::cofhello_elem_versionbitmap &versionbitmap =
               rofl::openflow::cofhello_elem_versionbitmap())
      : thread(this), fm_driver(), bridge(fm_driver) {
    rofl::crofbase::set_versionbitmap(versionbitmap);
    thread.start();
    tap_man = new rofcore::tap_manager();
  }

  /**
   *
   */
  ~cbasebox() override { delete tap_man; }

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

  static bool running() { return keep_on_running; }

  static void stop() { keep_on_running = false; };

protected:
  void handle_wakeup(rofl::cthread &thread) override;

  void handle_conn_established(rofl::crofdpt &dpt,
                               const rofl::cauxid &auxid) override {
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

  void handle_dpt_open(rofl::crofdpt &dpt) override;

  void handle_dpt_close(const rofl::cdptid &dptid) override;

  void handle_conn_terminated(rofl::crofdpt &dpt,
                              const rofl::cauxid &auxid) override;

  void handle_conn_refused(rofl::crofdpt &dpt,
                           const rofl::cauxid &auxid) override;

  void handle_conn_failed(rofl::crofdpt &dpt,
                          const rofl::cauxid &auxid) override;

  void handle_conn_negotiation_failed(rofl::crofdpt &dpt,
                                      const rofl::cauxid &auxid) override;

  void handle_conn_congestion_occured(rofl::crofdpt &dpt,
                                      const rofl::cauxid &auxid) override;

  void handle_conn_congestion_solved(rofl::crofdpt &dpt,
                                     const rofl::cauxid &auxid) override;

  void
  handle_features_reply(rofl::crofdpt &dpt, const rofl::cauxid &auxid,
                        rofl::openflow::cofmsg_features_reply &msg) override;

  void handle_desc_stats_reply(
      rofl::crofdpt &dpt, const rofl::cauxid &auxid,
      rofl::openflow::cofmsg_desc_stats_reply &msg) override;

  void handle_packet_in(rofl::crofdpt &dpt, const rofl::cauxid &auxid,
                        rofl::openflow::cofmsg_packet_in &msg) override;

  void handle_flow_removed(rofl::crofdpt &dpt, const rofl::cauxid &auxid,
                           rofl::openflow::cofmsg_flow_removed &msg) override;

  void handle_port_status(rofl::crofdpt &dpt, const rofl::cauxid &auxid,
                          rofl::openflow::cofmsg_port_status &msg) override;

  void handle_error_message(rofl::crofdpt &dpt, const rofl::cauxid &auxid,
                            rofl::openflow::cofmsg_error &msg) override;

  void handle_port_desc_stats_reply(
      rofl::crofdpt &dpt, const rofl::cauxid &auxid,
      rofl::openflow::cofmsg_port_desc_stats_reply &msg) override;

  void handle_port_desc_stats_reply_timeout(rofl::crofdpt &dpt,
                                            uint32_t xid) override;

  void handle_experimenter_message(
      rofl::crofdpt &dpt, const rofl::cauxid &auxid,
      rofl::openflow::cofmsg_experimenter &msg) override;

public:
  friend std::ostream &operator<<(std::ostream &os, const cbasebox &box) {
    os << rofcore::indent(0) << "<cbasebox>" << std::endl;
    return os;
  }

private:
  rofl::cdptid dptid;
  rofcore::tap_manager *tap_man;
  rofl::rofl_ofdpa_fm_driver fm_driver;
  ofdpa_bridge bridge;
  std::map<int, uint32_t> port_id_to_of_port;
  std::map<uint32_t, int> of_port_to_port_id;

  /* IO */
  int enqueue(rofcore::ctapdev *netdev, rofl::cpacket *pkt) override;

  /* OF handler */
  void handle_srcmac_table(rofl::crofdpt &dpt,
                           rofl::openflow::cofmsg_packet_in &msg);

  void handle_acl_policy_table(rofl::crofdpt &dpt,
                               rofl::openflow::cofmsg_packet_in &msg);

  void handle_bridging_table_rm(rofl::crofdpt &dpt,
                                rofl::openflow::cofmsg_flow_removed &msg);

  void init(rofl::crofdpt &dpt);

  void send_full_state(rofl::crofdpt &dpt);

  /* netlink */
  void link_created(unsigned int ifindex) noexcept override;

  void link_updated(const rofcore::crtlink &newlink) noexcept override;

  void link_deleted(unsigned int ifindex) noexcept override;

  void neigh_ll_created(unsigned int ifindex,
                        uint16_t nbindex) noexcept override;

  void neigh_ll_updated(unsigned int ifindex,
                        uint16_t nbindex) noexcept override;

  void neigh_ll_deleted(unsigned int ifindex,
                        uint16_t nbindex) noexcept override;
}; // class cbasebox

} // end of namespace basebox

#endif /* CROFBASE_HPP_ */
