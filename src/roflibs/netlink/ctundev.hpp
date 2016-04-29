/*
 * ctundev.h
 *
 *  Created on: 25.08.2014
 *      Author: andreas
 */

#ifndef CTUNDEV_H_
#define CTUNDEV_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>

#include <assert.h>
#include <netlink/object.h>
#include <netlink/route/addr.h>

#include <list>
#include <iostream>

#include <roflibs/netlink/cnetdev.hpp>
#include <roflibs/netlink/clogging.hpp>
#include <roflibs/netlink/cnetlink.hpp>

namespace rofcore {

class eTunDevBase : public eNetDevBase {
public:
  eTunDevBase(const std::string &__arg) : eNetDevBase(__arg){};
};
class eTunDevSysCallFailed : public eNetDevSysCallFailed {
public:
  eTunDevSysCallFailed(const std::string &__arg)
      : eNetDevSysCallFailed(__arg){};
};
class eTunDevOpenFailed : public eTunDevSysCallFailed {
public:
  eTunDevOpenFailed(const std::string &__arg) : eTunDevSysCallFailed(__arg){};
};
class eTunDevIoctlFailed : public eTunDevSysCallFailed {
public:
  eTunDevIoctlFailed(const std::string &__arg) : eTunDevSysCallFailed(__arg){};
};
class eTunDevNotFound : public eTunDevBase {
public:
  eTunDevNotFound(const std::string &__arg) : eTunDevBase(__arg){};
};

class ctundev : public cnetdev, rofl::cthread_env {
public:
  /**
   *
   */
  ctundev(cnetdev_owner *netdev_owner, std::string const &devname);

  /**
   *
   */
  virtual ~ctundev();

  /**
   * @brief	Enqueues a single rofl::cpacket instance on cnetdev.
   *
   * rofl::cpacket instance must have been allocated on heap and must
   * be removed
   */
  virtual void enqueue(rofl::cpacket *pkt);

  virtual void enqueue(std::vector<rofl::cpacket *> pkts);

protected:
  /**
   * @brief	open tapX device
   */
  void tun_open(std::string const &devname);

  /**
   * @brief	close tapX device
   *
   */
  void tun_close();

  virtual void handle_wakeup(rofl::cthread &thread) {}

  /**
   * @brief	handle read events on file descriptor
   */
  virtual void handle_read_event(rofl::cthread &thread, int fd);

  /**
   * @brief	handle write events on file descriptor
   */
  virtual void handle_write_event(rofl::cthread &thread, int fd);

  /**
   * @brief	reschedule opening of port in case of failure
   */
  virtual void handle_timeout(rofl::cthread &thread, uint32_t timer_id,
                              const std::list<unsigned int> &ttypes);

public:
  friend std::ostream &operator<<(std::ostream &os, const ctundev &tundev) {
    os << rofcore::indent(0) << "<ctundev "
       << "devname: " << tundev.devname << " >" << std::endl;
    return os;
  };

  class ctundev_find_by_devname {
    std::string devname;

  public:
    ctundev_find_by_devname(const std::string &devname) : devname(devname){};
    bool operator()(const std::pair<uint32_t, ctundev *> &p) const {
      return (p.second->devname == devname);
    };
  };

private:
  int fd;                                // tap device file descriptor
  std::list<rofl::cpacket *> pout_queue; // queue of outgoing packets
  std::string devname;
  rofl::cthread thread;

  enum ctundev_timer_t {
    CTUNDEV_TIMER_OPEN_PORT = 1,
  };
};

}; // end of namespace rofcore

#endif /* CTUNDEV_H_ */
