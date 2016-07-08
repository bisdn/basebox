/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef CTAPDEV_H_
#define CTAPDEV_H_ 1

#include <deque>
#include <exception>

#include <rofl/common/cthread.hpp>
#include <rofl/common/cpacket.h>

namespace rofcore {

class tap_callback;

class ctapdev : public rofl::cthread_env {

  int fd;                                 // tap device file descriptor
  std::deque<rofl::cpacket *> pout_queue; // queue of outgoing packets
  mutable rofl::crwlock pout_queue_rwlock;
  std::string devname;
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
  ctapdev(tap_callback &cb, std::string const &devname, pthread_t tid = 0);

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
   * @brief	open tapX device
   */
  void tap_open();

  /**
   * @brief	close tapX device
   *
   */
  void tap_close();

protected:
  void tx();

  /**
   * @brief	handle read events on file descriptor
   */
  virtual void handle_read_event(rofl::cthread &thread, int fd);

  /**
   * @brief	handle write events on file descriptor
   */
  virtual void handle_write_event(rofl::cthread &thread, int fd);

  virtual void handle_wakeup(rofl::cthread &thread) { tx(); }
};

} // end of namespace rofcore

#endif /* CTAPDEV_H_ */
