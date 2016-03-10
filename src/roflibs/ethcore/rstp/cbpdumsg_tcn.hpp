/*
 * cbpdumsg_tcn.hpp
 *
 *  Created on: 03.01.2015
 *      Author: andreas
 */

#ifndef CBPDUMSG_TCN_HPP_
#define CBPDUMSG_TCN_HPP_

#include "cbpdumsg.hpp"

namespace roflibs {
namespace eth {
namespace rstp {

/**
 * @class cbpdumsg_configuration
 * @brief Configuration bridge PDU
 */
class cbpdumsg_tcn : public cbpdumsg {
public:
  /**
   * @brief cbpdumsg_tcn destructor
   */
  virtual ~cbpdumsg_tcn(){};

  /**
   * @brief cbpdumsg_tcn default constructor
   */
  cbpdumsg_tcn(){};

public:
  /**
   * @brief Assignment operator
   */
  cbpdumsg_tcn &operator=(const cbpdumsg_tcn &bpdu) {
    if (this == &bpdu)
      return *this;
    cbpdumsg::operator=(bpdu);
    return *this;
  };

public:
  /**
   * @brief Returns buffer length required for packing this instance.
   */
  virtual size_t length() const {
    return sizeof(bpdu_tcn_body_t) + cbpdumsg::length();
  };

  /**
   * @brief Packs this instance' internal state into the given buffer.
   */
  virtual void pack(uint8_t *buf, size_t buflen) {
    if (buflen < cbpdumsg_tcn::length()) {
      throw exception::eBpduInval("cbpdumsg_tcn::pack() buflen too short");
    }
    cbpdumsg::pack(buf, buflen);
    set_bpdu_type(BPDU_TYPE_TOPOLOGY_CHANGE_NOTIFICATION);
  };

  /**
   * @brief Unpacks a given buffer and stores its content in this instance.
   */
  virtual void unpack(uint8_t *buf, size_t buflen) {
    if (buflen < cbpdumsg_tcn::length()) {
      throw exception::eBpduInval("cbpdumsg_cnf::unpack() buflen too short");
    }
    cbpdumsg::unpack(buf, buflen);
    if (BPDU_TYPE_TOPOLOGY_CHANGE_NOTIFICATION != get_bpdu_type()) {
      throw exception::eBpduInval("cbpdumsg_cnf::unpack() invalid BPDU type");
    }
  };

public:
  friend std::ostream &operator<<(std::ostream &os, const cbpdumsg_tcn &msg) {

    return os;
  };

private:
};

}; // end of namespace rstp
}; // end of namespace ethernet
}; // end of namespace roflibs

#endif /* CBPDUMSG_TCN_HPP_ */
