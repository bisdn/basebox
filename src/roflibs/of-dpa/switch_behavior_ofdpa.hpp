#ifndef SRC_ROFLIBS_OF_DPA_SWITCH_BEHAVIOR_OFDPA_HPP_
#define SRC_ROFLIBS_OF_DPA_SWITCH_BEHAVIOR_OFDPA_HPP_

#include <string>

#include <rofl/common/locking.hpp>

#include "baseboxd/switch_behavior.hpp"

#include "roflibs/of-dpa/ofdpa_bridge.hpp"

#include "roflibs/netlink/cnetlink.hpp"
#include "roflibs/netlink/cnetlink_observer.hpp"
#include "roflibs/netlink/tap_manager.hpp"

namespace basebox {

class eSwitchBehaviorBaseErr : public std::runtime_error {
public:
  eSwitchBehaviorBaseErr(const std::string &__arg)
      : std::runtime_error(__arg) {}
};

class eLinkNoDptAttached : public eSwitchBehaviorBaseErr {
public:
  eLinkNoDptAttached(const std::string &__arg)
      : eSwitchBehaviorBaseErr(__arg) {}
};

class eLinkTapDevNotFound : public eSwitchBehaviorBaseErr {
public:
  eLinkTapDevNotFound(const std::string &__arg)
      : eSwitchBehaviorBaseErr(__arg) {}
};

class switch_behavior_ofdpa
    : public switch_behavior,
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

public:
  switch_behavior_ofdpa(rofl::crofdpt &dpt);

  virtual ~switch_behavior_ofdpa();

  void init(rofl::crofdpt &dpt) override;

  virtual int get_switch_type() { return 1; }

  virtual void handle_packet_in(rofl::crofdpt &dpt, const rofl::cauxid &auxid,
                                rofl::openflow::cofmsg_packet_in &msg);

  virtual void handle_flow_removed(rofl::crofdpt &dpt,
                                   const rofl::cauxid &auxid,
                                   rofl::openflow::cofmsg_flow_removed &msg);

  virtual void
  handle_experimenter_message(rofl::crofdpt &dpt, const rofl::cauxid &auxid,
                              rofl::openflow::cofmsg_experimenter &msg);

private:
  rofcore::tap_manager *tap_man;
  rofl::rofl_ofdpa_fm_driver fm_driver;
  ofdpa_bridge bridge;
  rofl::crofdpt &dpt;
  std::map<int, uint32_t> port_id_to_of_port;
  std::map<uint32_t, int> of_port_to_port_id;

  void handle_srcmac_table(const rofl::crofdpt &dpt,
                           rofl::openflow::cofmsg_packet_in &msg);

  void handle_acl_policy_table(const rofl::crofdpt &dpt,
                               rofl::openflow::cofmsg_packet_in &msg);

  void handle_bridging_table_rm(const rofl::crofdpt &dpt,
                                rofl::openflow::cofmsg_flow_removed &msg);

  void send_full_state(rofl::crofdpt &dpt);

  /* IO */
  int enqueue(rofcore::ctapdev *netdev, rofl::cpacket *pkt) override;

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
};
}
/* namespace basebox */

#endif /* SRC_ROFLIBS_OF_DPA_SWITCH_BEHAVIOR_OFDPA_HPP_ */
