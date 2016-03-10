/*
 * crtroutes.h
 *
 *  Created on: 22.07.2014
 *      Author: andreas
 */

#ifndef CRTROUTES_H_
#define CRTROUTES_H_

#include <iostream>
#include <map>

#include <roflibs/netlink/crtroute.hpp>
#include <roflibs/netlink/clogging.hpp>

namespace rofcore {

class crtroutes_in4 {
public:
  /**
   *
   */
  crtroutes_in4(){};

  /**
   *
   */
  virtual ~crtroutes_in4(){};

  /**
   *
   */
  crtroutes_in4(const crtroutes_in4 &rtroutes) { *this = rtroutes; };

  /**
   *
   */
  crtroutes_in4 &operator=(const crtroutes_in4 &rtroutes) {
    if (this == &rtroutes)
      return *this;

    clear();
    for (std::map<unsigned int, crtroute_in4>::const_iterator it =
             rtroutes.rtroutes.begin();
         it != rtroutes.rtroutes.end(); ++it) {
      add_route(it->first) = it->second;
    }

    return *this;
  };

public:
  /**
   *
   */
  void clear() { rtroutes.clear(); };

  /**
   *
   */
  unsigned int add_route(const crtroute_in4 &rtroute) {
    std::map<unsigned int, crtroute_in4>::iterator it;
    if ((it = find_if(rtroutes.begin(), rtroutes.end(),
                      crtroute_in4_find(rtroute))) != rtroutes.end()) {
      rtroutes.erase(it->first);
    }
    unsigned int rtindex = 0;
    while (rtroutes.find(rtindex) != rtroutes.end()) {
      rtindex++;
    }
    rtroutes[rtindex] = rtroute;
    return rtindex;
  };

  /**
   *
   */
  unsigned int set_route(const crtroute_in4 &rtroute) {
    std::map<unsigned int, crtroute_in4>::iterator it;
    if ((it = find_if(rtroutes.begin(), rtroutes.end(),
                      crtroute_in4_find(rtroute))) == rtroutes.end()) {
      return add_route(rtroute);
    }
    rtroutes[it->first] = rtroute;
    return it->first;
  };

  /**
   *
   */
  unsigned int get_route(const crtroute_in4 &rtroute) const {
    std::map<unsigned int, crtroute_in4>::const_iterator it;
    if ((it = find_if(rtroutes.begin(), rtroutes.end(),
                      crtroute_in4_find(rtroute))) == rtroutes.end()) {
      throw crtroute::eRtRouteNotFound(
          "crtroutes_in4::get_route() / error: rtroute not found");
    }
    return it->first;
  };

  /**
   *
   */
  crtroute_in4 &add_route(unsigned int rtindex) {
    if (rtroutes.find(rtindex) != rtroutes.end()) {
      rtroutes.erase(rtindex);
    }
    return rtroutes[rtindex];
  };

  /**
   *
   */
  crtroute_in4 &set_route(unsigned int rtindex) {
    if (rtroutes.find(rtindex) == rtroutes.end()) {
      rtroutes[rtindex];
    }
    return rtroutes[rtindex];
  };

  /**
   *
   */
  const crtroute_in4 &get_route(unsigned int rtindex) const {
    if (rtroutes.find(rtindex) == rtroutes.end()) {
      throw crtroute::eRtRouteNotFound(
          "crtroutes_in4::get_route() / error: rtindex not found");
    }
    return rtroutes.at(rtindex);
  };

  /**
   *
   */
  void drop_route(unsigned int rtindex) {
    if (rtroutes.find(rtindex) == rtroutes.end()) {
      return;
    }
    rtroutes.erase(rtindex);
  };

  /**
   *
   */
  bool has_route(unsigned int rtindex) const {
    return (not(rtroutes.find(rtindex) == rtroutes.end()));
  };

public:
  friend std::ostream &operator<<(std::ostream &os,
                                  const crtroutes_in4 &rtroutes) {
    os << rofcore::indent(0)
       << "<crtroutes_in4 #rtroutes: " << rtroutes.rtroutes.size() << " >"
       << std::endl;
    rofcore::indent i(2);
    for (std::map<unsigned int, crtroute_in4>::const_iterator it =
             rtroutes.rtroutes.begin();
         it != rtroutes.rtroutes.end(); ++it) {
      os << it->second;
    }
    return os;
  };

  std::string str() const {
    std::stringstream ss;
    for (std::map<unsigned int, crtroute_in4>::const_iterator it =
             rtroutes.begin();
         it != rtroutes.end(); ++it) {
      ss << it->second.str() << std::endl;
    }
    return ss.str();
  };

private:
  std::map<unsigned int, crtroute_in4> rtroutes;
};

class crtroutes_in6 {
public:
  /**
   *
   */
  crtroutes_in6(){};

  /**
   *
   */
  virtual ~crtroutes_in6(){};

  /**
   *
   */
  crtroutes_in6(const crtroutes_in6 &rtroutes) { *this = rtroutes; };

  /**
   *
   */
  crtroutes_in6 &operator=(const crtroutes_in6 &rtroutes) {
    if (this == &rtroutes)
      return *this;

    clear();
    for (std::map<unsigned int, crtroute_in6>::const_iterator it =
             rtroutes.rtroutes.begin();
         it != rtroutes.rtroutes.end(); ++it) {
      add_route(it->first) = it->second;
    }

    return *this;
  };

public:
  /**
   *
   */
  void clear() { rtroutes.clear(); };

  /**
   *
   */
  unsigned int add_route(const crtroute_in6 &rtroute) {
    std::map<unsigned int, crtroute_in6>::iterator it;
    if ((it = find_if(rtroutes.begin(), rtroutes.end(),
                      crtroute_in6_find(rtroute))) != rtroutes.end()) {
      rtroutes.erase(it->first);
    }
    unsigned int rtindex = 0;
    while (rtroutes.find(rtindex) != rtroutes.end()) {
      rtindex++;
    }
    rtroutes[rtindex] = rtroute;
    return rtindex;
  };

  /**
   *
   */
  unsigned int set_route(const crtroute_in6 &rtroute) {
    std::map<unsigned int, crtroute_in6>::iterator it;
    if ((it = find_if(rtroutes.begin(), rtroutes.end(),
                      crtroute_in6_find(rtroute))) == rtroutes.end()) {
      return add_route(rtroute);
    }
    rtroutes[it->first] = rtroute;
    return it->first;
  };

  /**
   *
   */
  unsigned int get_route(const crtroute_in6 &rtroute) const {
    std::map<unsigned int, crtroute_in6>::const_iterator it;
    if ((it = find_if(rtroutes.begin(), rtroutes.end(),
                      crtroute_in6_find(rtroute))) == rtroutes.end()) {
      throw crtroute::eRtRouteNotFound(
          "crtroutes_in6::get_route() / error: rtroute not found");
    }
    return it->first;
  };

  /**
   *
   */
  crtroute_in6 &add_route(unsigned int rtindex) {
    if (rtroutes.find(rtindex) != rtroutes.end()) {
      rtroutes.erase(rtindex);
    }
    return rtroutes[rtindex];
  };

  /**
   *
   */
  crtroute_in6 &set_route(unsigned int rtindex) {
    if (rtroutes.find(rtindex) == rtroutes.end()) {
      rtroutes[rtindex];
    }
    return rtroutes[rtindex];
  };

  /**
   *
   */
  const crtroute_in6 &get_route(unsigned int rtindex) const {
    if (rtroutes.find(rtindex) == rtroutes.end()) {
      throw crtroute::eRtRouteNotFound(
          "crtroutes_in6::get_route() / error: rtindex not found");
    }
    return rtroutes.at(rtindex);
  };

  /**
   *
   */
  void drop_route(unsigned int rtindex) {
    if (rtroutes.find(rtindex) == rtroutes.end()) {
      return;
    }
    rtroutes.erase(rtindex);
  };

  /**
   *
   */
  bool has_route(unsigned int rtindex) const {
    return (not(rtroutes.find(rtindex) == rtroutes.end()));
  };

public:
  friend std::ostream &operator<<(std::ostream &os,
                                  const crtroutes_in6 &rtroutes) {
    os << rofcore::indent(0)
       << "<crtroutes_in6 #rtroutes: " << rtroutes.rtroutes.size() << " >"
       << std::endl;
    rofcore::indent i(2);
    for (std::map<unsigned int, crtroute_in6>::const_iterator it =
             rtroutes.rtroutes.begin();
         it != rtroutes.rtroutes.end(); ++it) {
      os << it->second;
    }
    return os;
  };

  std::string str() const {
    std::stringstream ss;
    for (std::map<unsigned int, crtroute_in6>::const_iterator it =
             rtroutes.begin();
         it != rtroutes.end(); ++it) {
      ss << it->second.str() << std::endl;
    }
    return ss.str();
  };

private:
  std::map<unsigned int, crtroute_in6> rtroutes;
};

}; // end of namespace

#endif /* CRTROUTES_H_ */
