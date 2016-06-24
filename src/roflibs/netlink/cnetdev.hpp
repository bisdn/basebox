/*
 * cnetdevice.h
 *
 *  Created on: 24.06.2013
 *      Author: andreas
 */

#ifndef CNETDEVICE_H_
#define CNETDEVICE_H_ 1

#include <exception>
#include <map>
#include <string>

#include <sys/ioctl.h>
#include <string.h>
#include <stdio.h>
#include <endian.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_arp.h>

#include <rofl/common/cpacket.h>
#include <rofl/common/caddress.h>

#include <roflibs/netlink/cpacketpool.hpp>

namespace rofcore {

class ctapdev;

class eNetDevBase : public std::runtime_error {
public:
  eNetDevBase(const std::string &__arg) : std::runtime_error(__arg){};
};
class eNetDevInval : public eNetDevBase {
public:
  eNetDevInval(const std::string &__arg) : eNetDevBase(__arg){};
};
class eNetDevInvalOwner : public eNetDevInval {
public:
  eNetDevInvalOwner(const std::string &__arg) : eNetDevInval(__arg){};
};
class eNetDevInvalNetDev : public eNetDevInval {
public:
  eNetDevInvalNetDev(const std::string &__arg) : eNetDevInval(__arg){};
};
class eNetDevExists : public eNetDevInval {
public:
  eNetDevExists(const std::string &__arg) : eNetDevInval(__arg){};
};
class eNetDevSysCallFailed : public eNetDevBase {
  std::string s_error;

public:
  eNetDevSysCallFailed(const std::string &__arg) : eNetDevBase(__arg) {
    std::stringstream ss;
    ss << __arg << " syscall failed (";
    ss << "errno:" << errno << " ";
    ss << strerror(errno) << ")";
    s_error = ss.str();
  };
  virtual ~eNetDevSysCallFailed() throw(){};
  virtual const char *what() const throw() { return s_error.c_str(); };
};
class eNetDevSocket : public eNetDevSysCallFailed {
public:
  eNetDevSocket(const std::string &__arg) : eNetDevSysCallFailed(__arg){};
};
class eNetDevIoctl : public eNetDevSysCallFailed {
public:
  eNetDevIoctl(const std::string &__arg) : eNetDevSysCallFailed(__arg){};
};
class eNetDevAgain : public eNetDevSysCallFailed {
public:
  eNetDevAgain(const std::string &__arg) : eNetDevSysCallFailed(__arg){};
};
class eNetDevCritical : public eNetDevBase {
public:
  eNetDevCritical(const std::string &__arg) : eNetDevBase(__arg){};
};

class cnetdev_owner {
public:
  virtual ~cnetdev_owner(){};

  /**
   * @brief	Enqeues a packet received on a cnetdevice to this
   * cnetdevice_owner
   *
   * This method is used to enqueue new cpacket instances received on a
   * cnetdevice
   * on a cnetdevice_owner instance. Default behaviour of all ::enqueue()
   * methods
   * is deletion of all packets from heap. All ::enqueue() methods should be
   * subscribed
   * by a class deriving from cnetdevice_owner.
   *
   * @param pkt Pointer to packet allocated on heap. This pkt must be deleted by
   * this method.
   */
  virtual void enqueue(ctapdev *netdev, rofl::cpacket *pkt);

  /**
   * @brief	Enqeues a vector of packets received on a cnetdevice to this
   * cnetdevice_owner
   *
   * This method is used to enqueue new cpacket instances received on a
   * cnetdevice
   * on a cnetdevice_owner instance. Default behaviour of all ::enqueue()
   * methods
   * is deletion of all packets from heap. All ::enqueue() methods should be
   * subscribed
   * by a class deriving from cnetdevice_owner.
   *
   * @param pkts Vector of cpacket instances allocated on the heap. All pkts
   * must be deleted by this method.
   */
  virtual void enqueue(ctapdev *netdev, std::vector<rofl::cpacket *> pkts);
};
};

#endif /* CNETDEVICE_H_ */
