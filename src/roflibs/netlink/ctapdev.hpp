/*
 * ctapdev.h
 *
 *  Created on: 24.06.2013
 *      Author: andreas
 */

#ifndef CTAPDEV_H_
#define CTAPDEV_H_ 1

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>

#include <list>

#include <rofl/common/cthread.hpp>
#include <rofl/common/cdptid.h>
#include <rofl/common/cpacket.h>

#include "roflibs/netlink/clogging.hpp"

namespace rofcore {

class tap_callback;

class eTapDevBase : public std::runtime_error {
public:
  eTapDevBase(const std::string &__arg) : std::runtime_error(__arg){}
};
class eTapDevSysCallFailed : public eTapDevBase {
public:
  eTapDevSysCallFailed(const std::string &__arg) : eTapDevBase(__arg){}
};
class eTapDevOpenFailed : public eTapDevSysCallFailed {
public:
  eTapDevOpenFailed(const std::string &__arg) : eTapDevSysCallFailed(__arg){}
};
class eTapDevIoctlFailed : public eTapDevSysCallFailed {
public:
  eTapDevIoctlFailed(const std::string &__arg) : eTapDevSysCallFailed(__arg){}
};
class eTapDevNotFound : public eTapDevBase {
public:
  eTapDevNotFound(const std::string &__arg) : eTapDevBase(__arg){}
};

class ctapdev : public rofl::cthread_env {

  int fd;                                // tap device file descriptor
  std::list<rofl::cpacket *> pout_queue; // queue of outgoing packets
  mutable rofl::crwlock pout_queue_rwlock;
  std::string devname;
  uint16_t pvid;
  rofl::cthread thread;
  tap_callback &cb;

  enum ctapdev_timer_t {
    CTAPDEV_TIMER_OPEN_PORT = 1,
  };

public:
  /**
   *
   * @param netdev_owner
   * @param devname
   */
  ctapdev(tap_callback &cb, std::string const &devname,
          pthread_t tid = 0);

  /**
   *
   */
  virtual ~ctapdev();

  const std::string &get_devname() const { return devname; }

  /**
   * @brief	Enqueues a single rofl::cpacket instance on cnetdev.
   *
   * rofl::cpacket instance must have been allocated on heap and must
   * be removed
   */
  virtual void enqueue(rofl::cpacket *pkt);

  virtual void enqueue(std::vector<rofl::cpacket *> pkts);

  /**
   *
   */
  uint16_t get_pvid() const { return pvid; }

protected:
  void tx();

  /**
   * @brief	open tapX device
   */
  void tap_open();

  /**
   * @brief	close tapX device
   *
   */
  void tap_close();

  /**
   * @brief	handle read events on file descriptor
   */
  virtual void handle_read_event(rofl::cthread &thread, int fd);

  /**
   * @brief	handle write events on file descriptor
   */
  virtual void handle_write_event(rofl::cthread &thread, int fd);

  virtual void handle_wakeup(rofl::cthread &thread) { tx(); }

  /**
   * @brief	reschedule opening of port in case of failure
   */
  virtual void handle_timeout(rofl::cthread &thread, uint32_t timer_id,
                              const std::list<unsigned int> &ttypes);

};

} // end of namespace rofcore

#endif /* CTAPDEV_H_ */
