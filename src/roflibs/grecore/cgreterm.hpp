/*
 * cgreterm.hpp
 *
 *  Created on: 18.08.2014
 *      Author: andreas
 */

#ifndef CGRETERM_HPP_
#define CGRETERM_HPP_

#include <bitset>
#include <vector>
#include <string>
#include <ostream>
#include <exception>

#include <rofl/common/crofdpt.h>
#include <rofl/common/cdptid.h>
#include <rofl/common/openflow/cofmatch.h>
#include <rofl/common/protocols/fvlanframe.h>
#include <rofl/common/protocols/fipv4frame.h>
#include <rofl/common/protocols/fipv6frame.h>
#include <rofl/common/protocols/fudpframe.h>
#include <rofl/common/openflow/experimental/actions/gre_actions.h>
#include <rofl/common/crandom.h>

#include "roflibs/netlink/clogging.hpp"
#include "roflibs/netlink/ccookiebox.hpp"
#include "roflibs/netlink/ctapdev.hpp"

namespace roflibs {
namespace gre {

class eGreTermBase : public std::runtime_error {
public:
  eGreTermBase(const std::string &__arg) : std::runtime_error(__arg){};
};
class eGreTermNotFound : public eGreTermBase {
public:
  eGreTermNotFound(const std::string &__arg) : eGreTermBase(__arg){};
};
class eLinkNoDptAttached : public eGreTermBase {
public:
  eLinkNoDptAttached(const std::string &__arg) : eGreTermBase(__arg){};
};
class eLinkTapDevNotFound : public eGreTermBase {
public:
  eLinkTapDevNotFound(const std::string &__arg) : eGreTermBase(__arg){};
};

/*
 * -ingress- means: send traffic into the tunnel
 * -egress-  means: strip the tunnel and send traffic to the external world
 */
class cgreterm : public rofcore::cnetdev_owner,
                 public roflibs::common::openflow::ccookie_owner {
public:
  /**
   *
   */
  cgreterm()
      : state(STATE_DETACHED), eth_ofp_table_id(0), gre_ofp_table_id(0),
        ip_fwd_ofp_table_id(0), idle_timeout(DEFAULT_IDLE_TIMEOUT),
        gre_portno(0), gre_key(0), gretap(0),
        cookie_gre_port_redirect(
            roflibs::common::openflow::ccookie_owner::acquire_cookie()),
        cookie_gre_port_shortcut(
            roflibs::common::openflow::ccookie_owner::acquire_cookie()),
        cookie_gre_tunnel_redirect(
            roflibs::common::openflow::ccookie_owner::acquire_cookie()),
        cookie_gre_tunnel_shortcut(
            roflibs::common::openflow::ccookie_owner::acquire_cookie()){};

  /**
   *
   */
  ~cgreterm() {
    if (gretap) {
      delete gretap;
      gretap = (rofcore::ctapdev *)0;
    }
  };

  /**
   *
   */
  cgreterm(const rofl::cdptid &dptid, uint8_t eth_ofp_table_id,
           uint8_t gre_ofp_table_id, uint8_t ip_fwd_ofp_table_id,
           uint32_t gre_portno, uint32_t gre_key);

  /**
   *
   */
  cgreterm(const cgreterm &greterm)
      : cookie_gre_port_redirect(
            roflibs::common::openflow::ccookie_owner::acquire_cookie()),
        cookie_gre_port_shortcut(
            roflibs::common::openflow::ccookie_owner::acquire_cookie()),
        cookie_gre_tunnel_redirect(
            roflibs::common::openflow::ccookie_owner::acquire_cookie()),
        cookie_gre_tunnel_shortcut(
            roflibs::common::openflow::ccookie_owner::acquire_cookie()) {
    *this = greterm;
  };

  /**
   *
   */
  cgreterm &operator=(const cgreterm &greterm) {
    if (this == &greterm)
      return *this;
    state = greterm.state;
    dptid = greterm.dptid;
    eth_ofp_table_id = greterm.eth_ofp_table_id;
    gre_ofp_table_id = greterm.gre_ofp_table_id;
    ip_fwd_ofp_table_id = greterm.ip_fwd_ofp_table_id;
    idle_timeout = greterm.idle_timeout;
    gre_portno = greterm.gre_portno;
    gre_key = greterm.gre_key;
    if (gretap) {
      delete gretap;
      gretap = (rofcore::ctapdev *)0;
    }
    // do not copy cookies here!
    return *this;
  };

public:
  /**
   *
   */
  const uint32_t &get_gre_key() const { return gre_key; };

  /**
   *
   */
  const uint32_t &get_gre_portno() const { return gre_portno; };

public:
  /**
   *
   */
  virtual void enqueue(rofcore::cnetdev *netdev, rofl::cpacket *pkt);

  /**
   *
   */
  virtual void enqueue(rofcore::cnetdev *netdev,
                       std::vector<rofl::cpacket *> pkts);

public:
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

private:
  /**
   *
   */
  virtual void gre_port_redirect(rofl::crofdpt &dpt, bool enable) = 0;

  /**
   *
   */
  virtual void gre_port_shortcut(rofl::crofdpt &dpt, bool enable) = 0;

  /**
   *
   */
  virtual void gre_tunnel_redirect(rofl::crofdpt &dpt, bool enable) = 0;

  /**
   *
   */
  virtual void gre_tunnel_shortcut(rofl::crofdpt &dpt, bool enable) = 0;

protected:
  /**
   *
   */
  void execute(std::string const &executable, std::vector<std::string> argv,
               std::vector<std::string> envp);

public:
  friend std::ostream &operator<<(std::ostream &os, const cgreterm &greterm) {
    os << rofcore::indent(0) << "<cgreterm key:0x" << std::hex
       << (unsigned int)greterm.get_gre_key() << std::dec << " >" << std::endl;
    return os;
  };

protected:
  enum ofp_state_t {
    STATE_DETACHED = 1,
    STATE_ATTACHED = 2,
  };
  enum ofp_state_t state;

  enum cgreterm_flags_t {
    FLAG_GRE_PORT_REDIRECTED = (1 << 0),
    FLAG_GRE_TUNNEL_REDIRECTED = (1 << 1),
    FLAG_GRE_PORT_SHORTCUT = (1 << 2),
    FLAG_GRE_TUNNEL_SHORTCUT = (1 << 3),
  };
  std::bitset<32> flags;

  rofl::cdptid dptid;
  uint8_t eth_ofp_table_id; // OFP tableid for capturing plain ethernet frames
                            // (table 0)
  uint8_t gre_ofp_table_id; // OFP tableid for installing GRE-pop entries
  uint8_t ip_fwd_ofp_table_id; // OFP tableid for forwarding IP datagrams
  static const int DEFAULT_IDLE_TIMEOUT = 15; // seconds
  int idle_timeout;

  std::string gre_tap_portname;
  std::string gre_tun_portname;
  uint32_t gre_portno; // portno of ethernet port
  uint32_t gre_key;    // GRE key according to IETF RFC 2890
  static const uint8_t GRE_IP_PROTO = 47;
  static const uint16_t GRE_PROT_TYPE_TRANSPARENT_ETHERNET_BRIDGING = 0x6558;

  static std::string script_path_init;
  static std::string script_path_term;

  rofcore::ctapdev *gretap;
  uint64_t cookie_gre_port_redirect;
  uint64_t cookie_gre_port_shortcut;
  uint64_t cookie_gre_tunnel_redirect;
  uint64_t cookie_gre_tunnel_shortcut;
};

class cgreterm_in4 : public cgreterm {
public:
  /**
   *
   */
  cgreterm_in4(){};

  /**
   *
   */
  ~cgreterm_in4() {
    try {
      if (STATE_ATTACHED == state) {
        handle_dpt_close(rofl::crofdpt::get_dpt(dptid));
      }
    } catch (rofl::eRofDptNotFound &e) {
    };
  };

  /**
   *
   */
  cgreterm_in4(const rofl::cdptid &dptid, uint8_t eth_ofp_table_id,
               uint8_t gre_ofp_table_id, uint8_t ip_fwd_ofp_table_id,
               const rofl::caddress_in4 &laddr, const rofl::caddress_in4 &raddr,
               uint32_t gre_portno, uint32_t gre_key)
      : cgreterm(dptid, eth_ofp_table_id, gre_ofp_table_id, ip_fwd_ofp_table_id,
                 gre_portno, gre_key),
        laddr(laddr), raddr(raddr){};

  /**
   *
   */
  cgreterm_in4(const cgreterm_in4 &greterm) { *this = greterm; };

  /**
   *
   */
  cgreterm_in4 &operator=(const cgreterm_in4 &greterm) {
    if (this == &greterm)
      return *this;
    cgreterm::operator=(greterm);
    laddr = greterm.laddr;
    raddr = greterm.raddr;
    return *this;
  };

  /**
   *
   */
  bool operator<(const cgreterm_in4 &greterm) const {
    return (raddr < greterm.raddr);
  };

public:
  /**
   *
   */
  const rofl::caddress_in4 &get_local_addr() const { return laddr; };

  /**
   *
   */
  const rofl::caddress_in4 &get_remote_addr() const { return raddr; };

public:
  /**
   *
   */
  void handle_dpt_open(rofl::crofdpt &dpt) {
    gre_port_redirect(dpt, true);
    gre_tunnel_redirect(dpt, true);

    state = STATE_ATTACHED;
    hook_init();
  };

  /**
   *
   */
  void handle_dpt_close(rofl::crofdpt &dpt) {
    hook_term();
    state = STATE_DETACHED;

    if (flags.test(FLAG_GRE_PORT_REDIRECTED)) {
      gre_port_redirect(dpt, false);
    }
    if (flags.test(FLAG_GRE_PORT_SHORTCUT)) {
      gre_port_shortcut(dpt, false);
    }
    if (flags.test(FLAG_GRE_TUNNEL_REDIRECTED)) {
      gre_tunnel_redirect(dpt, false);
    }
    if (flags.test(FLAG_GRE_TUNNEL_SHORTCUT)) {
      gre_tunnel_shortcut(dpt, false);
    }
  };

public:
  friend std::ostream &operator<<(std::ostream &os,
                                  const cgreterm_in4 &greterm) {
    os << dynamic_cast<const cgreterm &>(greterm);
    os << rofcore::indent(2)
       << "<cgreterm_in4 laddr:" << greterm.get_local_addr().str()
       << " <==> raddr:" << greterm.get_remote_addr().str() << " >"
       << std::endl;
    return os;
  };

private:
  /**
   *
   */
  virtual void gre_port_redirect(rofl::crofdpt &dpt, bool enable);

  /**
   *
   */
  virtual void gre_port_shortcut(rofl::crofdpt &dpt, bool enable);

  /**
   *
   */
  virtual void gre_tunnel_redirect(rofl::crofdpt &dpt, bool enable);

  /**
   *
   */
  virtual void gre_tunnel_shortcut(rofl::crofdpt &dpt, bool enable);

  /**
   *
   */
  void hook_init();

  /**
   *
   */
  void hook_term();

private:
  rofl::caddress_in4 laddr;
  rofl::caddress_in4 raddr;
};

class cgreterm_in6 : public cgreterm {
public:
  /**
   *
   */
  cgreterm_in6(){};

  /**
   *
   */
  ~cgreterm_in6() {
    try {
      if (STATE_ATTACHED == state) {
        handle_dpt_close(rofl::crofdpt::get_dpt(dptid));
      }
    } catch (rofl::eRofDptNotFound &e) {
    };
  };

  /**
   *
   */
  cgreterm_in6(const rofl::cdptid &dptid, uint8_t eth_ofp_table_id,
               uint8_t gre_ofp_table_id, uint8_t ip_fwd_ofp_table_id,
               const rofl::caddress_in6 &laddr, const rofl::caddress_in6 &raddr,
               uint32_t gre_portno, uint32_t gre_key)
      : cgreterm(dptid, eth_ofp_table_id, gre_ofp_table_id, ip_fwd_ofp_table_id,
                 gre_portno, gre_key),
        laddr(laddr), raddr(raddr){};

  /**
   *
   */
  cgreterm_in6(const cgreterm_in6 &greterm) { *this = greterm; };

  /**
   *
   */
  cgreterm_in6 &operator=(const cgreterm_in6 &greterm) {
    if (this == &greterm)
      return *this;
    cgreterm::operator=(greterm);
    laddr = greterm.laddr;
    raddr = greterm.raddr;
    return *this;
  };

  /**
   *
   */
  bool operator<(const cgreterm_in6 &greterm) const {
    return (raddr < greterm.raddr);
  };

public:
  /**
   *
   */
  const rofl::caddress_in6 &get_local_addr() const { return laddr; };

  /**
   *
   */
  const rofl::caddress_in6 &get_remote_addr() const { return raddr; };

public:
  /**
   *
   */
  void handle_dpt_open(rofl::crofdpt &dpt) {
    gre_port_redirect(dpt, true);
    gre_tunnel_redirect(dpt, true);

    state = STATE_ATTACHED;
    hook_init();
  };

  /**
   *
   */
  void handle_dpt_close(rofl::crofdpt &dpt) {
    hook_term();
    state = STATE_DETACHED;

    if (flags.test(FLAG_GRE_PORT_REDIRECTED)) {
      gre_port_redirect(dpt, false);
    }
    if (flags.test(FLAG_GRE_PORT_SHORTCUT)) {
      gre_port_shortcut(dpt, false);
    }
    if (flags.test(FLAG_GRE_TUNNEL_REDIRECTED)) {
      gre_tunnel_redirect(dpt, false);
    }
    if (flags.test(FLAG_GRE_TUNNEL_SHORTCUT)) {
      gre_tunnel_shortcut(dpt, false);
    }
  };

public:
  friend std::ostream &operator<<(std::ostream &os,
                                  const cgreterm_in6 &greterm) {
    os << dynamic_cast<const cgreterm &>(greterm);
    os << rofcore::indent(2)
       << "<cgreterm_in6 laddr:" << greterm.get_local_addr().str()
       << " <==> raddr:" << greterm.get_remote_addr().str() << " >"
       << std::endl;
    return os;
  };

private:
  /**
   *
   */
  virtual void gre_port_redirect(rofl::crofdpt &dpt, bool enable);

  /**
   *
   */
  virtual void gre_port_shortcut(rofl::crofdpt &dpt, bool enable);

  /**
   *
   */
  virtual void gre_tunnel_redirect(rofl::crofdpt &dpt, bool enable);

  /**
   *
   */
  virtual void gre_tunnel_shortcut(rofl::crofdpt &dpt, bool enable);

  /**
   *
   */
  void hook_init();

  /**
   *
   */
  void hook_term();

private:
  rofl::caddress_in6 laddr;
  rofl::caddress_in6 raddr;
};

}; // end of namespace gre
}; // end of namespace roflibs

#endif /* CGRETERM_HPP_ */
