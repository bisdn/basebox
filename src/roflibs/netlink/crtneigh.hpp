/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <ostream>
#include <cinttypes>

#include <netlink/route/neighbour.h>
#include <linux/neighbour.h>

#include <rofl/common/caddress.h>

#include "clogging.hpp"

namespace rofcore {

class crtneigh {
public:
  class eRtNeighBase : public std::runtime_error {
  public:
    eRtNeighBase(const std::string &__arg) : std::runtime_error(__arg) {}
  };
  class eRtNeighNotFound : public eRtNeighBase {
  public:
    eRtNeighNotFound(const std::string &__arg) : eRtNeighBase(__arg) {}
  };
  class eRtNeighExists : public eRtNeighBase {
  public:
    eRtNeighExists(const std::string &__arg) : eRtNeighBase(__arg) {}
  };

public:
  crtneigh()
      : state(0), flags(0), ifindex(0),
        lladdr(rofl::cmacaddr("00:00:00:00:00:00")), family(0), type(0),
        vlan(0) {}

  virtual ~crtneigh(){};

  crtneigh(const crtneigh &rtneigh) { *this = rtneigh; }

  crtneigh &operator=(const crtneigh &neigh) {
    if (this == &neigh)
      return *this;

    state = neigh.state;
    flags = neigh.flags;
    ifindex = neigh.ifindex;
    lladdr = neigh.lladdr;
    family = neigh.family;
    type = neigh.type;
    vlan = neigh.vlan;

    return *this;
  }

  crtneigh(struct rtnl_neigh *neigh)
      : state(0), flags(0), ifindex(0),
        lladdr(rofl::cmacaddr("00:00:00:00:00:00")), family(0), type(0) {
    char s_buf[128];

    state = rtnl_neigh_get_state(neigh);
    flags = rtnl_neigh_get_flags(neigh);
    ifindex = rtnl_neigh_get_ifindex(neigh);
    family = rtnl_neigh_get_family(neigh);
    type = rtnl_neigh_get_type(neigh);
    vlan = rtnl_neigh_get_vlan(neigh);

    memset(s_buf, 0, sizeof(s_buf));
    nl_addr2str(rtnl_neigh_get_lladdr(neigh), s_buf, sizeof(s_buf));
    if (std::string(s_buf) != std::string("none"))
      lladdr = rofl::cmacaddr(
          nl_addr2str(rtnl_neigh_get_lladdr(neigh), s_buf, sizeof(s_buf)));
    else
      lladdr = rofl::cmacaddr("00:00:00:00:00:00");
  }

  bool operator==(const crtneigh &rtneigh) {
    return ((ifindex == rtneigh.ifindex) && (family == rtneigh.family) &&
            (type == rtneigh.type) && (vlan == rtneigh.vlan) &&
            (lladdr == rtneigh.lladdr));
  }

public:
  int get_state() const { return state; }

  std::string get_state_s() const {
    std::string str;

    switch (state) {
    case NUD_INCOMPLETE:
      str = std::string("INCOMPLETE");
      break;
    case NUD_REACHABLE:
      str = std::string("REACHABLE");
      break;
    case NUD_STALE:
      str = std::string("STALE");
      break;
    case NUD_DELAY:
      str = std::string("DELAY");
      break;
    case NUD_PROBE:
      str = std::string("PROBE");
      break;
    case NUD_FAILED:
      str = std::string("FAILED");
      break;
    default:
      str = std::string("UNKNOWN");
      break;
    }

    return str;
  }

  unsigned int get_flags() const { return flags; }

  int get_ifindex() const { return ifindex; }

  const rofl::cmacaddr &get_lladdr() const { return lladdr; }

  int get_family() const { return family; }

  int get_type() const { return type; }

  int get_vlan() const { return vlan; }

public:
  friend std::ostream &operator<<(std::ostream &os, crtneigh const &neigh) {
    os << rofcore::indent(0) << "<crtneigh: >" << std::endl;
    os << rofcore::indent(2) << "<state: " << neigh.state << " >" << std::endl;
    os << rofcore::indent(2) << "<flags: " << neigh.flags << " >" << std::endl;
    os << rofcore::indent(2) << "<ifindex: " << neigh.ifindex << " >"
       << std::endl;
    os << rofcore::indent(2) << "<lladdr: " << neigh.lladdr.str() << " >"
       << std::endl;
    os << rofcore::indent(2) << "<family: " << neigh.family << " >"
       << std::endl;
    os << rofcore::indent(2) << "<type: " << neigh.type << " >" << std::endl;
    os << rofcore::indent(2) << "<vlan: " << neigh.vlan << " >" << std::endl;
    return os;
  }

private:
  int state;
  unsigned int flags;
  int ifindex;
  rofl::cmacaddr lladdr;
  int family;
  int type;
  int vlan;
};

class crtneigh_ll_find : public std::unary_function<crtneigh, bool> {
  crtneigh rtneigh;

public:
  crtneigh_ll_find(const crtneigh &rtneigh) : rtneigh(rtneigh) {}
  bool operator()(const crtneigh &rtn) { return (rtneigh == rtn); }
  bool operator()(const std::pair<unsigned int, crtneigh> &p) {
    return (rtneigh == p.second);
  }
};

class crtneigh_in4 : public crtneigh {
public:
  crtneigh_in4() {}

  ~crtneigh_in4() override{};

  crtneigh_in4(const crtneigh_in4 &neigh) { *this = neigh; }

  crtneigh_in4 &operator=(const crtneigh_in4 &neigh) {
    if (this == &neigh)
      return *this;
    crtneigh::operator=(neigh);
    dst = neigh.dst;
    return *this;
  }

  crtneigh_in4(struct rtnl_neigh *neigh) : crtneigh(neigh) {

    nl_object_get(
        (struct nl_object *)neigh); // increment reference counter by one

    char s_buf[128];
    memset(s_buf, 0, sizeof(s_buf));

    std::string s_dst;
    nl_addr2str(rtnl_neigh_get_dst(neigh), s_buf, sizeof(s_buf));
    if (std::string(s_buf) != std::string("none"))
      s_dst.assign(
          nl_addr2str(rtnl_neigh_get_dst(neigh), s_buf, sizeof(s_buf)));
    else
      s_dst.assign("0.0.0.0/0");

    dst = rofl::caddress_in4(s_dst.c_str());

    nl_object_put(
        (struct nl_object *)neigh); // decrement reference counter by one
  }

  bool operator==(const crtneigh_in4 &rtneigh) {
    return ((crtneigh::operator==(rtneigh)) && (dst == rtneigh.dst));
  }

public:
  const rofl::caddress_in4 get_dst() const { return dst; }

public:
  friend std::ostream &operator<<(std::ostream &os, const crtneigh_in4 &neigh) {
    os << rofcore::indent(0) << "<crtneigh_in4: >" << std::endl;
    rofcore::indent i(2);
    os << dynamic_cast<const crtneigh &>(neigh);
    os << rofcore::indent(2) << "<dst: " << neigh.dst.str() << " >"
       << std::endl;
    return os;
  }

  std::string str() const {
    /*
    192.168.133.1 dev wlp3s0 lladdr 00:90:0b:30:3a:e3 STALE
     */
    std::stringstream ss;
    ss << "neigh/inet " << dst.str() << " via lladdr " << get_lladdr().str()
       << " ";
    if (get_flags() & NUD_INCOMPLETE)
      ss << "INCOMPLETE";
    if (get_flags() & NUD_REACHABLE)
      ss << "REACHABLE";
    if (get_flags() & NUD_STALE)
      ss << "STALE";
    if (get_flags() & NUD_DELAY)
      ss << "DELAY";
    if (get_flags() & NUD_PROBE)
      ss << "PROBE";
    if (get_flags() & NUD_FAILED)
      ss << "FAILED";
    return ss.str();
  }

  class crtneigh_in4_find_by_dst {
    rofl::caddress_in4 dst;

  public:
    crtneigh_in4_find_by_dst(const rofl::caddress_in4 &dst) : dst(dst) {}
    bool operator()(const std::pair<uint16_t, crtneigh_in4> &p) {
      return (p.second.dst == dst);
    }
  };

private:
  rofl::caddress_in4 dst;
};

class crtneigh_in4_find : public std::unary_function<crtneigh_in4, bool> {
  crtneigh_in4 rtneigh;

public:
  crtneigh_in4_find(const crtneigh_in4 &rtneigh) : rtneigh(rtneigh) {}
  bool operator()(const crtneigh_in4 &rtn) { return (rtneigh == rtn); }
  bool operator()(const std::pair<unsigned int, crtneigh_in4> &p) {
    return (rtneigh == p.second);
  }
};

class crtneigh_in6 : public crtneigh {
public:
  crtneigh_in6() {}

  ~crtneigh_in6() override{};

  crtneigh_in6(const crtneigh_in6 &neigh) { *this = neigh; }

  crtneigh_in6 &operator=(const crtneigh_in6 &neigh) {
    if (this == &neigh)
      return *this;
    crtneigh::operator=(neigh);
    dst = neigh.dst;
    return *this;
  }

  crtneigh_in6(struct rtnl_neigh *neigh) : crtneigh(neigh) {

    nl_object_get(
        (struct nl_object *)neigh); // increment reference counter by one

    char s_buf[128];
    memset(s_buf, 0, sizeof(s_buf));

    std::string s_dst;
    nl_addr2str(rtnl_neigh_get_dst(neigh), s_buf, sizeof(s_buf));
    if (std::string(s_buf) != std::string("none"))
      s_dst.assign(
          nl_addr2str(rtnl_neigh_get_dst(neigh), s_buf, sizeof(s_buf)));
    else
      s_dst.assign("::");

    dst = rofl::caddress_in6(s_dst.c_str());

    nl_object_put(
        (struct nl_object *)neigh); // decrement reference counter by one
  }

  bool operator==(const crtneigh_in6 &rtneigh) {
    return ((crtneigh::operator==(rtneigh)) && (dst == rtneigh.dst));
  }

public:
  const rofl::caddress_in6 get_dst() const { return dst; }

public:
  friend std::ostream &operator<<(std::ostream &os, const crtneigh_in6 &neigh) {
    os << rofcore::indent(0) << "<crtneigh_in6: >" << std::endl;
    rofcore::indent i(2);
    os << dynamic_cast<const crtneigh &>(neigh);
    os << rofcore::indent(2) << "<dst: " << neigh.dst.str() << " >"
       << std::endl;
    return os;
  }

  std::string str() const {
    /*
    192.168.133.1 dev wlp3s0 lladdr 00:90:0b:30:3a:e3 STALE
     */
    std::stringstream ss;
    ss << "neigh/inet6 " << dst.str() << " via lladdr " << get_lladdr().str()
       << " ";
    if (get_flags() & NUD_INCOMPLETE)
      ss << "INCOMPLETE";
    if (get_flags() & NUD_REACHABLE)
      ss << "REACHABLE";
    if (get_flags() & NUD_STALE)
      ss << "STALE";
    if (get_flags() & NUD_DELAY)
      ss << "DELAY";
    if (get_flags() & NUD_PROBE)
      ss << "PROBE";
    if (get_flags() & NUD_FAILED)
      ss << "FAILED";
    return ss.str();
  }

  class crtneigh_in6_find_by_dst {
    rofl::caddress_in6 dst;

  public:
    crtneigh_in6_find_by_dst(const rofl::caddress_in6 &dst) : dst(dst) {}
    bool operator()(const std::pair<uint16_t, crtneigh_in6> &p) {
      return (p.second.dst == dst);
    }
  };

private:
  rofl::caddress_in6 dst;
};

class crtneigh_in6_find : public std::unary_function<crtneigh_in6, bool> {
  crtneigh_in6 rtneigh;

public:
  crtneigh_in6_find(const crtneigh_in6 &rtneigh) : rtneigh(rtneigh) {}
  bool operator()(const crtneigh_in6 &rtn) { return (rtneigh == rtn); }
  bool operator()(const std::pair<unsigned int, crtneigh_in6> &p) {
    return (rtneigh == p.second);
  }
};

} // end of namespace
