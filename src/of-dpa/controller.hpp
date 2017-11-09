/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef CROFBASE_HPP_
#define CROFBASE_HPP_

#include <sys/types.h>
#include <sys/wait.h>

#include <exception>
#include <iostream>
#include <mutex>
#include <set>
#include <string>

#include <glog/logging.h>
#include <rofl/common/crofbase.h>
#include <rofl/common/crofdpt.h>
#include <rofl/ofdpa/rofl_ofdpa_fm_driver.hpp>

#include "sai.hpp"

namespace basebox {

class eBaseBoxBase : public std::runtime_error {
public:
  eBaseBoxBase(const std::string &__arg) : std::runtime_error(__arg) {}
};

class controller : public rofl::crofbase,
                   public virtual rofl::cthread_env,
                   public basebox::switch_interface {

  enum ExperimenterMessageType {
    QUERY_FLOW_ENTRIES, ///< query flow entries from controller
    RECEIVED_FLOW_ENTRIES_QUERY
  };

  enum ExperimenterId {
    BISDN = 0xFF0000B0 ///< should be registered as ONF-Managed Experimenter ID
                       ///(OUI)
  };

  basebox::nbi *nbi;

  controller(const controller &) = delete;
  controller &operator=(const controller &) = delete;

public:
  controller(basebox::nbi *nbi,
             const rofl::openflow::cofhello_elem_versionbitmap &versionbitmap =
                 rofl::openflow::cofhello_elem_versionbitmap())
      : nbi(nbi), bb_thread(this) {
    nbi->register_switch(this);
    rofl::crofbase::set_versionbitmap(versionbitmap);
    bb_thread.start();
  }

  ~controller() override {}

protected:
  void handle_conn_established(rofl::crofdpt &dpt,
                               const rofl::cauxid &auxid) override {}

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

  void handle_conn_congestion_occurred(rofl::crofdpt &dpt,
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

  void request_port_stats();

  void handle_port_stats_reply(
      rofl::crofdpt &dpt, const rofl::cauxid &auxid,
      rofl::openflow::cofmsg_port_stats_reply &msg) override;

  void handle_timeout(rofl::cthread &thread, uint32_t timer_id) override;

public:
  // switch_interface
  int l2_addr_remove_all_in_vlan(uint32_t port, uint16_t vid) noexcept override;
  int l2_addr_add(uint32_t port, uint16_t vid, const rofl::cmacaddr &mac,
                  bool filtered) noexcept override;
  int l2_addr_remove(uint32_t port, uint16_t vid,
                     const rofl::cmacaddr &mac) noexcept override;

  int ingress_port_vlan_accept_all(uint32_t port) noexcept override;
  int ingress_port_vlan_drop_accept_all(uint32_t port) noexcept override;
  int ingress_port_vlan_add(uint32_t port, uint16_t vid,
                            bool pvid) noexcept override;
  int ingress_port_vlan_remove(uint32_t port, uint16_t vid,
                               bool pvid) noexcept override;

  int egress_port_vlan_accept_all(uint32_t port) noexcept override;
  int egress_port_vlan_drop_accept_all(uint32_t port) noexcept override;
  int egress_port_vlan_add(uint32_t port, uint16_t vid,
                           bool untagged) noexcept override;
  int egress_port_vlan_remove(uint32_t port, uint16_t vid,
                              bool untagged) noexcept override;

  int get_statistics(uint64_t port_no, uint32_t number_of_counters,
                     const sai_port_stat_t *counter_ids,
                     uint64_t *counters) override {
    LOG(FATAL) << ": not implemented";
  }

  /* IO */
  int enqueue(uint32_t port_id, rofl::cpacket *pkt) noexcept override;

  int subscribe_to(enum swi_flags flags) noexcept override;

  int get_statistics(uint64_t port_no, uint32_t number_of_counters, const sai_port_stat_t *counter_ids, uint64_t *counters);

  /* print this */
  friend std::ostream &operator<<(std::ostream &os, const controller &box) {
    os << "<controller>" << std::endl;
    return os;
  }

private:
  rofl::cdptid dptid;
  rofl::rofl_ofdpa_fm_driver fm_driver;
  std::map<uint16_t, std::set<uint32_t>> l2_domain;
  std::mutex conn_mutex;
  rofl::cthread bb_thread;
  std::mutex stats_mutex;
  rofl::openflow::cofportstatsarray stats_array;

  enum timer_t {
    /* handle_timeout will be called as well from crofbase, hence we need some
       id head room */
    TIMER_port_stats_request = 10 // timer_id for querying port statistics
  };
  const int port_stats_request_interval = 2; // time in seconds

  /* OF handler */
  void handle_srcmac_table(rofl::crofdpt &dpt,
                           rofl::openflow::cofmsg_packet_in &msg);

  void handle_acl_policy_table(rofl::crofdpt &dpt,
                               rofl::openflow::cofmsg_packet_in &msg);

  void handle_bridging_table_rm(rofl::crofdpt &dpt,
                                rofl::openflow::cofmsg_flow_removed &msg);
}; // class controller

} // end of namespace basebox

#endif /* CROFBASE_HPP_ */
