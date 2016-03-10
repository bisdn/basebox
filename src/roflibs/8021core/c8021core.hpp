/*
 * c8021core.hpp
 *
 *  Created on: 03.08.2014
 *      Author: andreas
 */

#ifndef C8021CORE_HPP_
#define C8021CORE_HPP_

#include <sys/types.h>
#include <sys/wait.h>

#include <inttypes.h>
#include <iostream>
#include <map>
#include <rofl/common/crofbase.h>
#include <rofl/common/crofdpt.h>
#include <rofl/common/thread_helper.h>
#include <rofl/common/protocols/fetherframe.h>
#include <rofl/common/protocols/fvlanframe.h>

#include "roflibs/netlink/cnetlink.hpp"
#include "roflibs/netlink/clogging.hpp"
#include "roflibs/netlink/ctundev.hpp"
#include "roflibs/netlink/ctapdev.hpp"
#include "roflibs/netlink/ccookiebox.hpp"

namespace roflibs {
namespace c8021 {

class e8021CoreBase : public std::runtime_error {
public:
  e8021CoreBase(const std::string &__arg) : std::runtime_error(__arg){};
};
class e8021CoreNotFound : public e8021CoreBase {
public:
  e8021CoreNotFound(const std::string &__arg) : e8021CoreBase(__arg){};
};
class e8021CoreExists : public e8021CoreBase {
public:
  e8021CoreExists(const std::string &__arg) : e8021CoreBase(__arg){};
};
class eLinkNoDptAttached : public e8021CoreBase {
public:
  eLinkNoDptAttached(const std::string &__arg) : e8021CoreBase(__arg){};
};
class eLinkTapDevNotFound : public e8021CoreBase {
public:
  eLinkTapDevNotFound(const std::string &__arg) : e8021CoreBase(__arg){};
};

class c8021core : public rofcore::cnetdev_owner,
                  public rofcore::cnetlink_common_observer,
                  public roflibs::common::openflow::ccookie_owner {
public:
  /**
   *
   */
  static std::set<uint64_t> &get_8021_cores() { return dpids; };

  /**
   *
   */
  static c8021core &add_8021_core(const rofl::cdptid &dptid,
                                  uint8_t table_id_eth_in,
                                  uint8_t table_id_eth_src,
                                  uint8_t table_id_eth_local,
                                  uint8_t table_id_eth_dst,
                                  uint16_t default_pvid = DEFAULT_PVID) {
    if (c8021core::c8021cores.find(dptid) != c8021core::c8021cores.end()) {
      delete c8021core::c8021cores[dptid];
    }
    c8021core::c8021cores[dptid] =
        new c8021core(dptid, table_id_eth_in, table_id_eth_src,
                      table_id_eth_local, table_id_eth_dst, default_pvid);
    dpids.insert(rofl::crofdpt::get_dpt(dptid).get_dpid().get_uint64_t());
    return *(c8021core::c8021cores[dptid]);
  };

  /**
   *
   */
  static c8021core &set_8021_core(const rofl::cdptid &dptid,
                                  uint8_t table_id_eth_in,
                                  uint8_t table_id_eth_src,
                                  uint8_t table_id_eth_local,
                                  uint8_t table_id_eth_dst,
                                  uint16_t default_pvid = DEFAULT_PVID) {
    if (c8021core::c8021cores.find(dptid) == c8021core::c8021cores.end()) {
      c8021core::c8021cores[dptid] =
          new c8021core(dptid, table_id_eth_in, table_id_eth_src,
                        table_id_eth_local, table_id_eth_dst, default_pvid);
      dpids.insert(rofl::crofdpt::get_dpt(dptid).get_dpid().get_uint64_t());
    }
    return *(c8021core::c8021cores[dptid]);
  };

  /**
   *
   */
  static c8021core &set_8021_core(const rofl::cdptid &dptid) {
    if (c8021core::c8021cores.find(dptid) == c8021core::c8021cores.end()) {
      throw e8021CoreNotFound("c8021core::set_8021_core() dpid not found");
    }
    return *(c8021core::c8021cores[dptid]);
  };

  /**
   *
   */
  static const c8021core &get_8021_core(const rofl::cdptid &dptid) {
    if (c8021core::c8021cores.find(dptid) == c8021core::c8021cores.end()) {
      throw e8021CoreNotFound("c8021core::get_8021_core() dpid not found");
    }
    return *(c8021core::c8021cores.at(dptid));
  };

  /**
   *
   */
  static void drop_8021_core(const rofl::cdptid &dptid) {
    if (c8021core::c8021cores.find(dptid) == c8021core::c8021cores.end()) {
      return;
    }
    dpids.erase(rofl::crofdpt::get_dpt(dptid).get_dpid().get_uint64_t());
    delete c8021core::c8021cores[dptid];
    c8021core::c8021cores.erase(dptid);
  };

  /**
   *
   */
  static bool has_8021_core(const rofl::cdptid &dptid) {
    return (
        not(c8021core::c8021cores.find(dptid) == c8021core::c8021cores.end()));
  };

private:
  /**
   *
   */
  c8021core(const rofl::cdptid &dptid, uint8_t table_id_eth_in,
            uint8_t table_id_eth_src, uint8_t table_id_eth_local,
            uint8_t table_id_eth_dst, uint16_t default_pvid = DEFAULT_PVID);

  /**
   *
   */
  virtual ~c8021core();

public:
  /**
   *
   */
  const rofl::cdpid &get_dpid() const {
    return rofl::crofdpt::get_dpt(dptid).get_dpid();
  };

  /**
   *
   */
  uint16_t get_default_pvid() const { return default_pvid; };

public:
public:
  /**
   *
   */
  void handle_dpt_open();

  /**
   *
   */
  void handle_dpt_close();

  /**
   *
   */
  virtual void handle_packet_in(rofl::crofdpt &dpt, const rofl::cauxid &auxid,
                                rofl::openflow::cofmsg_packet_in &msg);

  /**
   *
   */
  virtual void handle_flow_removed(rofl::crofdpt &dpt,
                                   const rofl::cauxid &auxid,
                                   rofl::openflow::cofmsg_flow_removed &msg);

  /**
   *
   */
  void handle_port_status(rofl::crofdpt &dpt, const rofl::cauxid &auxid,
                          rofl::openflow::cofmsg_port_status &msg);

  /**
   *
   */
  void handle_error_message(rofl::crofdpt &dpt, const rofl::cauxid &auxid,
                            rofl::openflow::cofmsg_error &msg);

public:
  /*
   * from ctapdev
   */
  virtual void enqueue(rofcore::cnetdev *netdev, rofl::cpacket *pkt);

  virtual void enqueue(rofcore::cnetdev *netdev,
                       std::vector<rofl::cpacket *> pkts);

private:
  /**
   *
   */
  void clear_tap_devs(const rofl::cdptid &dpid) {
    while (not devs[dpid].empty()) {
      std::map<std::string, rofcore::ctapdev *>::iterator it =
          devs[dpid].begin();
      try {
        hook_port_down(rofl::crofdpt::get_dpt(it->second->get_dptid()),
                       it->second->get_devname());
      } catch (rofl::eRofDptNotFound &e) {
      }
      drop_tap_dev(dpid, it->first);
    }
  };

  /**
   *
   */
  rofcore::ctapdev &add_tap_dev(const rofl::cdptid &dpid,
                                const std::string &devname, uint16_t pvid,
                                const rofl::caddress_ll &hwaddr) {
    if (devs[dpid].find(devname) != devs[dpid].end()) {
      delete devs[dpid][devname];
    }
    devs[dpid][devname] =
        new rofcore::ctapdev(this, dpid, devname, pvid, hwaddr);
    return *(devs[dpid][devname]);
  };

  /**
   *
   */
  rofcore::ctapdev &set_tap_dev(const rofl::cdptid &dpid,
                                const std::string &devname, uint16_t pvid,
                                const rofl::caddress_ll &hwaddr) {
    if (devs[dpid].find(devname) == devs[dpid].end()) {
      devs[dpid][devname] =
          new rofcore::ctapdev(this, dpid, devname, pvid, hwaddr);
    }
    return *(devs[dpid][devname]);
  };

  /**
   *
   */
  rofcore::ctapdev &set_tap_dev(const rofl::cdptid &dpid,
                                const std::string &devname) {
    if (devs[dpid].find(devname) == devs[dpid].end()) {
      throw rofcore::eTunDevNotFound(
          "cbasebox::set_tap_dev() devname not found");
    }
    return *(devs[dpid][devname]);
  };

  /**
   *
   */
  rofcore::ctapdev &set_tap_dev(const rofl::cdptid &dpid,
                                const rofl::caddress_ll &hwaddr) {
    std::map<std::string, rofcore::ctapdev *>::iterator it;
    if ((it = find_if(devs[dpid].begin(), devs[dpid].end(),
                      rofcore::ctapdev::ctapdev_find_by_hwaddr(
                          dpid, hwaddr))) == devs[dpid].end()) {
      throw rofcore::eTunDevNotFound(
          "cbasebox::set_tap_dev() hwaddr not found");
    }
    return *(it->second);
  };

  /**
   *
   */
  const rofcore::ctapdev &get_tap_dev(const rofl::cdptid &dpid,
                                      const std::string &devname) const {
    if (devs.find(dpid) == devs.end()) {
      throw rofcore::eTunDevNotFound("cbasebox::get_tap_dev() dpid not found");
    }
    if (devs.at(dpid).find(devname) == devs.at(dpid).end()) {
      throw rofcore::eTunDevNotFound(
          "cbasebox::get_tap_dev() devname not found");
    }
    return *(devs.at(dpid).at(devname));
  };

  /**
   *
   */
  const rofcore::ctapdev &get_tap_dev(const rofl::cdptid &dpid,
                                      const rofl::caddress_ll &hwaddr) const {
    if (devs.find(dpid) == devs.end()) {
      throw rofcore::eTunDevNotFound("cbasebox::get_tap_dev() dpid not found");
    }
    std::map<std::string, rofcore::ctapdev *>::const_iterator it;
    if ((it = find_if(devs.at(dpid).begin(), devs.at(dpid).end(),
                      rofcore::ctapdev::ctapdev_find_by_hwaddr(
                          dpid, hwaddr))) == devs.at(dpid).end()) {
      throw rofcore::eTunDevNotFound(
          "cbasebox::get_tap_dev() hwaddr not found");
    }
    return *(it->second);
  };

  /**
   *
   */
  void drop_tap_dev(const rofl::cdptid &dpid, const std::string &devname) {
    if (devs[dpid].find(devname) == devs[dpid].end()) {
      return;
    }
    delete devs[dpid][devname];
    devs[dpid].erase(devname);
  };

  /**
   *
   */
  void drop_tap_dev(const rofl::cdptid &dpid, const rofl::caddress_ll &hwaddr) {
    std::map<std::string, rofcore::ctapdev *>::const_iterator it;
    if ((it = find_if(devs[dpid].begin(), devs[dpid].end(),
                      rofcore::ctapdev::ctapdev_find_by_hwaddr(
                          dpid, hwaddr))) == devs[dpid].end()) {
      return;
    }
    delete it->second;
    devs[dpid].erase(it->first);
  };

  /**
   *
   */
  bool has_tap_dev(const rofl::cdptid &dpid, const std::string &devname) const {
    if (devs.find(dpid) == devs.end()) {
      return false;
    }
    return (not(devs.at(dpid).find(devname) == devs.at(dpid).end()));
  };

  /**
   *
   */
  bool has_tap_dev(const rofl::cdptid &dpid,
                   const rofl::caddress_ll &hwaddr) const {
    if (devs.find(dpid) == devs.end()) {
      return false;
    }
    std::map<std::string, rofcore::ctapdev *>::const_iterator it;
    if ((it = find_if(devs.at(dpid).begin(), devs.at(dpid).end(),
                      rofcore::ctapdev::ctapdev_find_by_hwaddr(
                          dpid, hwaddr))) == devs.at(dpid).end()) {
      return false;
    }
    return true;
  };

private:
  /*
   * event specific hooks
   */
  void hook_port_up(const rofl::crofdpt &dpt, std::string const &devname);

  void hook_port_down(const rofl::crofdpt &dpt, std::string const &devname);

  void execute(std::string const &executable, std::vector<std::string> argv,
               std::vector<std::string> envp);

public:
  friend std::ostream &operator<<(std::ostream &os, const c8021core &core) {
    rofl::RwLock rwlock(core.vlans_rwlock, rofl::RwLock::RWLOCK_READ);
    os << rofcore::indent(0)
       << "<c8021core default-pvid: " << (int)core.get_default_pvid() << " >"
       << std::endl;
    rofcore::indent i(2);
    for (std::map<rofl::cdptid,
                  std::map<std::string, rofcore::ctapdev *>>::const_iterator
             it = core.devs.begin();
         it != core.devs.end(); ++it) {
      for (std::map<std::string, rofcore::ctapdev *>::const_iterator jt =
               it->second.begin();
           jt != it->second.end(); ++jt) {
        os << *(jt->second);
      }
    }
    // rofcore::indent i(2);
    os << core.get_dpid();
    return os;
  };

private:
  enum c8021core_state_t {
    STATE_IDLE = 1,
    STATE_DETACHED = 2,
    STATE_QUERYING = 3,
    STATE_ATTACHED = 4,
  };

  std::map<rofl::cdptid, std::map<std::string, rofcore::ctapdev *>> devs;
  static const uint16_t DEFAULT_PVID = 1;

  static std::string script_path_port_up;
  static std::string script_path_port_down;

  c8021core_state_t state;
  rofl::cdptid dptid;
  uint64_t cookie_grp_addr;
  uint64_t cookie_miss_entry_src;
  uint64_t cookie_miss_entry_local;
  uint64_t cookie_redirect_inject;
  uint16_t default_pvid;
  mutable rofl::PthreadRwLock vlans_rwlock;
  std::set<uint32_t> group_ids;
  uint8_t table_id_eth_in;
  uint8_t table_id_eth_src;
  uint8_t table_id_eth_local;
  uint8_t table_id_eth_dst;

  static std::set<uint64_t> dpids;
  static std::map<rofl::cdptid, c8021core *> c8021cores;

private:
};

}; // end of namespace ethernet
}; // end of namespace roflibs

#endif /* CETHCORE_HPP_ */
