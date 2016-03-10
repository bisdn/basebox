/*
 * cpriovect.hpp
 *
 *  Created on: 17.11.2014
 *      Author: andreas
 */

#ifndef CPRIOVECT_HPP_
#define CPRIOVECT_HPP_

#include "cbridgeid.hpp"
#include "crpcost.hpp"
#include "cportid.hpp"

#include <roflibs/netlink/clogging.hpp>

namespace roflibs {
namespace eth {
namespace rstp {

class cpriovect {
public:
  /**
   *
   */
  cpriovect(){};

  /**
   *
   */
  ~cpriovect(){};

  /**
   *
   */
  cpriovect(const cbridgeid &RootBridgeID, const crpcost &RootPathCost,
            const cbridgeid &DesignatedBridgeID,
            const cportid &DesignatedPortID, const cportid &BridgePortID)
      : RootBridgeID(RootBridgeID), RootPathCost(RootPathCost),
        DesignatedBridgeID(DesignatedBridgeID),
        DesignatedPortID(DesignatedPortID), BridgePortID(BridgePortID){};

  /**
   *
   */
  cpriovect(const cpriovect &vect) { *this = vect; };

  /**
   *
   */
  cpriovect &operator=(const cpriovect &vect) {
    if (this == &vect)
      return *this;
    RootBridgeID = vect.RootBridgeID;
    RootPathCost = vect.RootPathCost;
    DesignatedBridgeID = vect.DesignatedBridgeID;
    DesignatedPortID = vect.DesignatedPortID;
    BridgePortID = vect.BridgePortID;
    return *this;
  };

  /**
   *
   */
  bool operator<(const cpriovect &vect) {
    // IEEE802.1D-2004 section 17.6
    return (((vect.RootBridgeID < RootBridgeID)) ||
            ((vect.RootBridgeID == RootBridgeID) &&
             (vect.RootPathCost < RootPathCost)) ||
            ((vect.RootBridgeID == RootBridgeID) &&
             (vect.RootPathCost == RootPathCost) &&
             (vect.DesignatedBridgeID < DesignatedBridgeID)) ||
            ((vect.RootBridgeID == RootBridgeID) &&
             (vect.RootPathCost == RootPathCost) &&
             (vect.DesignatedBridgeID == DesignatedBridgeID) &&
             (vect.DesignatedPortID < DesignatedPortID)) ||
            ((vect.DesignatedBridgeID.get_bridge_address() ==
              DesignatedBridgeID.get_bridge_address()) &&
             (vect.get_DesignatedPortID() == DesignatedPortID)));
  };

public:
  // RootBridgeID
  const cbridgeid &get_RootBridgeID() const { return RootBridgeID; };

  void set_RootBridgeID(const cbridgeid &RootBridgeID) {
    this->RootBridgeID = RootBridgeID;
  };

  cbridgeid set_RootBridgeID() { return RootBridgeID; };

  // RootPathCost
  const crpcost &get_RootPathCost() const { return RootPathCost; };

  void set_RootPathCost(const crpcost &RootPathCost) {
    this->RootPathCost = RootPathCost;
  };

  crpcost &set_RootPathCost() { return RootPathCost; };

  // DesignatedBridgeID
  const cbridgeid &get_DesignatedBridgeID() const {
    return DesignatedBridgeID;
  };

  void set_DesignatedBridgeID(const cbridgeid &DesignatedBridgeID) {
    this->DesignatedBridgeID = DesignatedBridgeID;
  };

  cbridgeid &set_DesignatedBridgeID() { return DesignatedBridgeID; };

  // DesignatedPortID
  const cportid &get_DesignatedPortID() const { return DesignatedPortID; };

  void set_DesignatedPortID(const cportid &DesignatedPortID) {
    this->DesignatedPortID = DesignatedPortID;
  };

  cportid &set_DesignatedPortID() { return DesignatedPortID; };

  // BridgePortID
  const cportid &get_BridgePortID() const { return BridgePortID; };

  void set_BridgePortID(const cportid &BridgePortID) {
    this->BridgePortID = BridgePortID;
  };

  cportid &set_BridgePortID() { return BridgePortID; };

public:
  friend std::ostream &operator<<(std::ostream &os, const cpriovect &vect) {
    os << rofcore::indent(0) << "<cpriovect ";
    os << vect.get_RootBridgeID().str() << ":";
    os << vect.get_RootPathCost().str() << ":";
    os << vect.get_DesignatedBridgeID().str() << ":";
    os << vect.get_DesignatedPortID().str() << ":";
    os << vect.get_BridgePortID().str() << " >";
    return os;
  };

protected:
  // port priority vector
  cbridgeid RootBridgeID;
  crpcost RootPathCost;
  cbridgeid DesignatedBridgeID;
  cportid DesignatedPortID;
  cportid BridgePortID;
};

}; // end of namespace rstp
}; // end of namespace eth
}; // end of namespace roflibs

#endif /* CPRIOVECTOR_HPP_ */
