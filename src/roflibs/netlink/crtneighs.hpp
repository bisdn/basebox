/*
 * crtneighs.h
 *
 *  Created on: 22.07.2014
 *      Author: andreas
 */

#ifndef CRTNEIGHS_H_
#define CRTNEIGHS_H_

#include <iostream>
#include <map>
#include <list>
#include <algorithm>

#include <roflibs/netlink/crtneigh.hpp>
#include <roflibs/netlink/clogging.hpp>

namespace rofcore {

class crtneighs_ll {
public:
  /**
   *
   */
  crtneighs_ll() {}

  /**
   *
   */
  virtual ~crtneighs_ll() {}

  /**
   *
   */
  crtneighs_ll(const crtneighs_ll &rtneighs) { *this = rtneighs; }

  /**
   *
   */
  crtneighs_ll &operator=(const crtneighs_ll &rtneighs) {
    if (this == &rtneighs)
      return *this;

    clear();
    for (std::map<unsigned int, crtneigh>::const_iterator it =
             rtneighs.rtneighs.begin();
         it != rtneighs.rtneighs.end(); ++it) {
      add_neigh(it->first) = it->second;
    }

    return *this;
  }

public:
  /**
   *
   */
  bool empty() const { return rtneighs.empty(); }

  /**
   *
   */
  void clear() { rtneighs.clear(); }

  /**
   *
   */
  unsigned int add_neigh(const crtneigh &rtneigh) {
    std::map<unsigned int, crtneigh>::iterator it;
    if ((it = find_if(rtneighs.begin(), rtneighs.end(),
                      crtneigh_ll_find(rtneigh))) != rtneighs.end()) {
      rtneighs.erase(it->first);
    }
    unsigned int nbindex = 0;
    while (rtneighs.find(nbindex) != rtneighs.end()) {
      nbindex++;
    }
    rtneighs[nbindex] = rtneigh;
    return nbindex;
  }

  /**
   *
   */
  unsigned int set_neigh(const crtneigh &rtneigh) {
    std::map<unsigned int, crtneigh>::iterator it;
    if ((it = find_if(rtneighs.begin(), rtneighs.end(),
                      crtneigh_ll_find(rtneigh))) == rtneighs.end()) {
      return add_neigh(rtneigh);
    }
    rtneighs[it->first] = rtneigh;
    return it->first;
  }

  /**
   *
   */
  unsigned int get_neigh(const crtneigh &rtneigh) const {
    std::map<unsigned int, crtneigh>::const_iterator it;
    if ((it = find_if(rtneighs.begin(), rtneighs.end(),
                      crtneigh_ll_find(rtneigh))) == rtneighs.end()) {
      throw crtneigh::eRtNeighNotFound(
          "crtneighs_ll::get_neigh() / error: rtneigh not found");
    }
    return it->first;
  }

  /**
   *
   */
  crtneigh &add_neigh(unsigned int nbindex) {
    if (rtneighs.find(nbindex) != rtneighs.end()) {
      rtneighs.erase(nbindex);
    }
    return rtneighs[nbindex];
  }

  /**
   *
   */
  crtneigh &set_neigh(unsigned int nbindex) {
    if (rtneighs.find(nbindex) == rtneighs.end()) {
      rtneighs[nbindex];
    }
    return rtneighs[nbindex];
  }

  /**
   *
   */
  const crtneigh &get_neigh(unsigned int nbindex) const {
    if (rtneighs.find(nbindex) == rtneighs.end()) {
      throw crtneigh::eRtNeighNotFound(
          "crtneighs_ll::get_neigh() / error: nbindex not found");
    }
    return rtneighs.at(nbindex);
  }

  /**
   *
   */
  void drop_neigh(unsigned int nbindex) {
    if (rtneighs.find(nbindex) == rtneighs.end()) {
      return;
    }
    rtneighs.erase(nbindex);
  };

  /**
   *
   */
  bool has_neigh(unsigned int nbindex) const {
    return (not(rtneighs.find(nbindex) == rtneighs.end()));
  }

  std::list<unsigned int> keys() const {
    std::list<unsigned int> keys;

    for (const auto &i : rtneighs) {
      keys.push_back(i.first);
    }

    return keys;
  }

public:
  friend std::ostream &operator<<(std::ostream &os,
                                  const crtneighs_ll &rtneighs) {
    os << rofcore::indent(0)
       << "<crtneighs_ll #rtneighs: " << rtneighs.rtneighs.size() << " >"
       << std::endl;
    rofcore::indent i(2);
    for (std::map<unsigned int, crtneigh>::const_iterator it =
             rtneighs.rtneighs.begin();
         it != rtneighs.rtneighs.end(); ++it) {
      os << it->second;
    }
    return os;
  }

  std::string str() const {
    std::stringstream ss;
    for (std::map<unsigned int, crtneigh>::const_iterator it = rtneighs.begin();
         it != rtneighs.end(); ++it) {
      ss << it->second << std::endl;
    }
    return ss.str();
  }

private:
  std::map<unsigned int, crtneigh> rtneighs;
};

class crtneighs_in4 {
public:
  /**
   *
   */
  crtneighs_in4(){};

  /**
   *
   */
  virtual ~crtneighs_in4(){};

  /**
   *
   */
  crtneighs_in4(const crtneighs_in4 &rtneighs) { *this = rtneighs; };

  /**
   *
   */
  crtneighs_in4 &operator=(const crtneighs_in4 &rtneighs) {
    if (this == &rtneighs)
      return *this;

    clear();
    for (std::map<unsigned int, crtneigh_in4>::const_iterator it =
             rtneighs.rtneighs.begin();
         it != rtneighs.rtneighs.end(); ++it) {
      add_neigh(it->first) = it->second;
    }

    return *this;
  };

public:
  /**
   *
   */
  bool empty() const { return rtneighs.empty(); };

  /**
   *
   */
  void clear() { rtneighs.clear(); };

  /**
   *
   */
  unsigned int add_neigh(const crtneigh_in4 &rtneigh) {
    std::map<unsigned int, crtneigh_in4>::iterator it;
    if ((it = find_if(rtneighs.begin(), rtneighs.end(),
                      crtneigh_in4_find(rtneigh))) != rtneighs.end()) {
      rtneighs.erase(it->first);
    }
    unsigned int nbindex = 0;
    while (rtneighs.find(nbindex) != rtneighs.end()) {
      nbindex++;
    }
    rtneighs[nbindex] = rtneigh;
    return nbindex;
  };

  /**
   *
   */
  unsigned int set_neigh(const crtneigh_in4 &rtneigh) {
    std::map<unsigned int, crtneigh_in4>::iterator it;
    if ((it = find_if(rtneighs.begin(), rtneighs.end(),
                      crtneigh_in4_find(rtneigh))) == rtneighs.end()) {
      return add_neigh(rtneigh);
    }
    rtneighs[it->first] = rtneigh;
    return it->first;
  };

  /**
   *
   */
  unsigned int get_neigh(const crtneigh_in4 &rtneigh) const {
    std::map<unsigned int, crtneigh_in4>::const_iterator it;
    if ((it = find_if(rtneighs.begin(), rtneighs.end(),
                      crtneigh_in4_find(rtneigh))) == rtneighs.end()) {
      throw crtneigh::eRtNeighNotFound(
          "crtneighs_in4::get_neigh() / error: rtneigh not found");
    }
    return it->first;
  };

  /**
   *
   */
  crtneigh_in4 &add_neigh(unsigned int nbindex) {
    if (rtneighs.find(nbindex) != rtneighs.end()) {
      rtneighs.erase(nbindex);
    }
    return rtneighs[nbindex];
  };

  /**
   *
   */
  crtneigh_in4 &set_neigh(unsigned int nbindex) {
    if (rtneighs.find(nbindex) == rtneighs.end()) {
      rtneighs[nbindex];
    }
    return rtneighs[nbindex];
  };

  /**
   *
   */
  const crtneigh_in4 &get_neigh(unsigned int nbindex) const {
    if (rtneighs.find(nbindex) == rtneighs.end()) {
      throw crtneigh::eRtNeighNotFound(
          "crtneighs_in4::get_neigh() / error: nbindex not found");
    }
    return rtneighs.at(nbindex);
  };

  /**
   *
   */
  void drop_neigh(unsigned int nbindex) {
    if (rtneighs.find(nbindex) == rtneighs.end()) {
      return;
    }
    rtneighs.erase(nbindex);
  };

  /**
   *
   */
  bool has_neigh(unsigned int nbindex) const {
    return (not(rtneighs.find(nbindex) == rtneighs.end()));
  };

public:
  friend std::ostream &operator<<(std::ostream &os,
                                  const crtneighs_in4 &rtneighs) {
    os << rofcore::indent(0)
       << "<crtneighs_in4 #rtneighs: " << rtneighs.rtneighs.size() << " >"
       << std::endl;
    rofcore::indent i(2);
    for (std::map<unsigned int, crtneigh_in4>::const_iterator it =
             rtneighs.rtneighs.begin();
         it != rtneighs.rtneighs.end(); ++it) {
      os << it->second;
    }
    return os;
  };

  std::string str() const {
    std::stringstream ss;
    for (std::map<unsigned int, crtneigh_in4>::const_iterator it =
             rtneighs.begin();
         it != rtneighs.end(); ++it) {
      ss << it->second.str() << std::endl;
    }
    return ss.str();
  };

private:
  std::map<unsigned int, crtneigh_in4> rtneighs;
};

class crtneighs_in6 {
public:
  /**
   *
   */
  crtneighs_in6(){};

  /**
   *
   */
  virtual ~crtneighs_in6(){};

  /**
   *
   */
  crtneighs_in6(const crtneighs_in6 &rtneighs) { *this = rtneighs; };

  /**
   *
   */
  crtneighs_in6 &operator=(const crtneighs_in6 &rtneighs) {
    if (this == &rtneighs)
      return *this;

    clear();
    for (std::map<unsigned int, crtneigh_in6>::const_iterator it =
             rtneighs.rtneighs.begin();
         it != rtneighs.rtneighs.end(); ++it) {
      add_neigh(it->first) = it->second;
    }

    return *this;
  };

public:
  /**
   *
   */
  bool empty() const { return rtneighs.empty(); };

  /**
   *
   */
  void clear() { rtneighs.clear(); };

  /**
   *
   */
  unsigned int add_neigh(const crtneigh_in6 &rtneigh) {
    std::map<unsigned int, crtneigh_in6>::iterator it;
    if ((it = find_if(rtneighs.begin(), rtneighs.end(),
                      crtneigh_in6_find(rtneigh))) != rtneighs.end()) {
      rtneighs.erase(it->first);
    }
    unsigned int nbindex = 0;
    while (rtneighs.find(nbindex) != rtneighs.end()) {
      nbindex++;
    }
    rtneighs[nbindex] = rtneigh;
    return nbindex;
  };

  /**
   *
   */
  unsigned int set_neigh(const crtneigh_in6 &rtneigh) {
    std::map<unsigned int, crtneigh_in6>::iterator it;
    if ((it = find_if(rtneighs.begin(), rtneighs.end(),
                      crtneigh_in6_find(rtneigh))) == rtneighs.end()) {
      return add_neigh(rtneigh);
    }
    rtneighs[it->first] = rtneigh;
    return it->first;
  };

  /**
   *
   */
  unsigned int get_neigh(const crtneigh_in6 &rtneigh) const {
    std::map<unsigned int, crtneigh_in6>::const_iterator it;
    if ((it = find_if(rtneighs.begin(), rtneighs.end(),
                      crtneigh_in6_find(rtneigh))) == rtneighs.end()) {
      throw crtneigh::eRtNeighNotFound(
          "crtneighs_in6::get_neigh() / error: rtneigh not found");
    }
    return it->first;
  };

  /**
   *
   */
  crtneigh_in6 &add_neigh(unsigned int nbindex) {
    if (rtneighs.find(nbindex) != rtneighs.end()) {
      rtneighs.erase(nbindex);
    }
    return rtneighs[nbindex];
  };

  /**
   *
   */
  crtneigh_in6 &set_neigh(unsigned int nbindex) {
    if (rtneighs.find(nbindex) == rtneighs.end()) {
      rtneighs[nbindex];
    }
    return rtneighs[nbindex];
  };

  /**
   *
   */
  const crtneigh_in6 &get_neigh(unsigned int nbindex) const {
    if (rtneighs.find(nbindex) == rtneighs.end()) {
      throw crtneigh::eRtNeighNotFound(
          "crtneighs_in6::get_neigh() / error: nbindex not found");
    }
    return rtneighs.at(nbindex);
  };

  /**
   *
   */
  void drop_neigh(unsigned int nbindex) {
    if (rtneighs.find(nbindex) == rtneighs.end()) {
      return;
    }
    rtneighs.erase(nbindex);
  };

  /**
   *
   */
  bool has_neigh(unsigned int nbindex) const {
    return (not(rtneighs.find(nbindex) == rtneighs.end()));
  };

public:
  friend std::ostream &operator<<(std::ostream &os,
                                  const crtneighs_in6 &rtneighs) {
    os << rofcore::indent(0)
       << "<crtneighs_in6 #rtneighs: " << rtneighs.rtneighs.size() << " >"
       << std::endl;
    rofcore::indent i(2);
    for (std::map<unsigned int, crtneigh_in6>::const_iterator it =
             rtneighs.rtneighs.begin();
         it != rtneighs.rtneighs.end(); ++it) {
      os << it->second;
    }
    return os;
  };

  std::string str() const {
    std::stringstream ss;
    for (std::map<unsigned int, crtneigh_in6>::const_iterator it =
             rtneighs.begin();
         it != rtneighs.end(); ++it) {
      ss << it->second.str() << std::endl;
    }
    return ss.str();
  };

private:
  std::map<unsigned int, crtneigh_in6> rtneighs;
};

}; // end of namespace

#endif /* CRTNEIGHS_H_ */
