/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <glog/logging.h>
#include "netlink/tap_manager.hpp"
#include "utils.hpp"

namespace basebox {

static inline void
release_packets(std::deque<std::pair<int, basebox::packet *>> &q) {
  for (auto i : q) {
    std::free(i.second);
  }
}

tap_io::~tap_io() { thread.stop(); }

void tap_io::register_tap(int fd, uint32_t port_id, switch_callback &cb) {
  {
    std::lock_guard<std::mutex> guard(events_mutex);
    events.emplace_back(std::make_tuple(TAP_IO_ADD, fd, port_id, &cb));
  }

  thread.wakeup();
}

void tap_io::unregister_tap(int fd, uint32_t port_id) {
  {
    std::lock_guard<std::mutex> guard(events_mutex);
    events.emplace_back(std::make_tuple(TAP_IO_REM, fd, port_id, nullptr));
  }

  thread.wakeup();
}

void tap_io::enqueue(int fd, basebox::packet *pkt) {
  if (fd == -1) {
    std::free(pkt);
    return;
  }

  {
    // store pkt in outgoing queue
    std::lock_guard<std::mutex> guard(pout_queue_mutex);
    pout_queue.emplace_back(std::make_pair(fd, pkt));
  }
  thread.wakeup();
}

void tap_io::handle_read_event(rofl::cthread &thread, int fd) {
  basebox::packet *pkt =
      (basebox::packet *)std::malloc(sizeof(basebox::packet));

  if (pkt == nullptr)
    return;

  pkt->len = read(fd, pkt->data, basebox::packet_data_len);

  if (pkt->len > 0) {
    VLOG(1) << __FUNCTION__ << ": read " << pkt->len << " bytes from fd=" << fd
            << " into pkt=" << pkt << " tid=" << pthread_self();
    std::pair<uint32_t, switch_callback *> &cb = sw_cbs.at(fd);
    cb.second->enqueue_to_switch(cb.first, pkt);
  } else {
    // error occured (or non-blocking)
    switch (errno) {
    case EAGAIN:
      LOG(ERROR) << __FUNCTION__
                 << ": EAGAIN XXX not implemented packet is dropped";
      std::free(pkt);
    default:
      LOG(ERROR) << __FUNCTION__ << ": unknown error occured";
      std::free(pkt);
    }
  }
}

void tap_io::handle_write_event(rofl::cthread &thread, int fd) {
  thread.drop_write_fd(fd);
  tx();
}

void tap_io::tx() {
  std::pair<int, basebox::packet *> pkt;
  std::deque<std::pair<int, basebox::packet *>> out_queue;

  {
    std::lock_guard<std::mutex> guard(pout_queue_mutex);
    std::swap(out_queue, pout_queue);
  }

  while (not out_queue.empty()) {

    pkt = out_queue.front();
    int rc = 0;
    if ((rc = write(pkt.first, pkt.second->data, pkt.second->len)) < 0) {
      switch (errno) {
      case EAGAIN:
        VLOG(1) << __FUNCTION__ << ": EAGAIN";
        {
          std::lock_guard<std::mutex> guard(pout_queue_mutex);
          std::move(out_queue.rbegin(), out_queue.rend(),
                    std::front_inserter(pout_queue));
        }
        thread.add_write_fd(pkt.first, true, false);
        return;
      case EIO:
        // tap not enabled drop packet
        VLOG(1) << __FUNCTION__ << ": EIO";
        release_packets(out_queue);
        return;
      default:
        // will drop packets
        release_packets(out_queue);
        LOG(ERROR) << __FUNCTION__ << ": unknown error occurred rc=" << rc
                   << " errno=" << errno << " '" << strerror(errno);
        return;
      }
    }
    std::free(pkt.second);
    out_queue.pop_front();
  }
}

void tap_io::handle_events() {
  std::lock_guard<std::mutex> guard(events_mutex);

  // register fds
  for (auto ev : events) {
    int fd = std::get<1>(ev);
    switch (std::get<0>(ev)) {

    case TAP_IO_ADD:
      sw_cbs.emplace(
          std::make_pair(fd, std::make_pair(std::get<2>(ev), std::get<3>(ev))));
      thread.add_read_fd(fd, true, false);
      break;
    case TAP_IO_REM:
      thread.drop_fd(fd, false);
      sw_cbs.erase(fd);
      break;
    default:
      break;
    }
  }
  events.clear();
}

tap_manager::~tap_manager() { destroy_tapdevs(); }

int tap_manager::create_tapdev(uint32_t port_id, const std::string &port_name,
                               switch_callback &cb) {
  int r = 0;
  auto it = devs.find(port_id);
  if (it == devs.end()) {
    ctapdev *dev;
    try {
      // XXX create mapping of port_ids?
      dev = new ctapdev(port_name);
      devs.insert(std::make_pair(port_id, dev));
      dev->tap_open();
      int fd = dev->get_fd();

      LOG(INFO) << __FUNCTION__ << ": portname=" << port_name << " fd=" << fd
                << " ptr=" << dev;

      io.register_tap(fd, port_id, cb);

    } catch (std::exception &e) {
      LOG(ERROR) << __FUNCTION__ << ": failed to create tapdev " << port_name;
      r = -EINVAL;
    }
  } else {
    LOG(INFO) << __FUNCTION__ << ": " << port_name
              << " with port_id=" << port_id << " already existing";
  }
  return r;
}

int tap_manager::destroy_tapdev(uint32_t port_id,
                                const std::string &port_name) {
  auto it = devs.find(port_id);
  if (it == devs.end()) {
    LOG(WARNING) << __FUNCTION__ << ": called for invalid port_id=" << port_id
                 << " port_name=" << port_name;
    return 0;
  }

  auto dev = it->second;
  int fd = dev->get_fd();
  devs.erase(it);
  delete dev;

  // XXX check if previous to delete
  io.unregister_tap(fd, port_id);

  return 0;
}

void tap_manager::destroy_tapdevs() {
  std::map<uint32_t, ctapdev *> ddevs;
  ddevs.swap(devs);
  for (auto &dev : ddevs) {
    delete dev.second;
  }
}

int tap_manager::enqueue(uint32_t port_id, basebox::packet *pkt) {
  try {
    int fd = devs.at(port_id)->get_fd();
    io.enqueue(fd, pkt);
  } catch (std::exception &e) {
    LOG(ERROR) << __FUNCTION__ << ": failed to enqueue packet " << pkt
               << " to port_id=" << port_id;
    std::free(pkt);
  }
  return 0;
}

} // namespace basebox
