/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cerrno>
#include <glog/logging.h>
#include <sys/resource.h>

#include "tap_io.hpp"

namespace basebox {

static inline void release_packets(std::deque<std::pair<int, packet *>> &q) {
  for (auto i : q) {
    std::free(i.second);
  }
}

tap_io::tap_io() : thread(this) {
  struct rlimit limit;
  int rv = getrlimit(RLIMIT_NOFILE, &limit);

  if (rv == -1) {
    LOG(FATAL) << __FUNCTION__
               << ": could not retrive RLIMIT_NOFILE errno=" << errno;
  }

  VLOG(2) << __FUNCTION__ << ": RLIMIT_NOFILE soft=" << limit.rlim_cur
          << " hard=" << limit.rlim_max;

  sw_cbs.resize(limit.rlim_max);

  thread.start("tap_io");
};

tap_io::~tap_io() { thread.stop(); }

void tap_io::register_tap(tap_io_details td) {
  {
    std::lock_guard<std::mutex> guard(events_mutex);
    events.emplace_back(std::make_pair(TAP_IO_ADD, td));
  }

  thread.wakeup();
}

void tap_io::unregister_tap(int fd, uint32_t port_id) {
  {
    std::lock_guard<std::mutex> guard(events_mutex);
    tap_io_details td;
    td.fd = fd;
    events.emplace_back(std::make_pair(TAP_IO_REM, td));
  }

  thread.wakeup();
}

void tap_io::enqueue(int fd, packet *pkt) {
  if (fd < 0) {
    std::free(pkt);
    return;
  }

  if (static_cast<size_t>(fd) < sw_cbs.capacity() && sw_cbs[fd].fd == fd) {
    // store pkt in outgoing queue
    std::lock_guard<std::mutex> guard(pout_queue_mutex);
    pout_queue.emplace_back(std::make_pair(fd, pkt));
  } else {
    std::free(pkt);
    return;
  }

  thread.wakeup();
}

void tap_io::update_mtu(int fd, unsigned mtu) {
  if (fd < 0 || static_cast<size_t>(fd) > sw_cbs.size()) {
    LOG(ERROR) << __FUNCTION__ << ": invalid fd=" << fd;
    return;
  }

  VLOG(4) << __FUNCTION__ << ": of fd=" << fd << ", mtu=" << mtu;

  sw_cbs[fd].mtu = mtu;
}

void tap_io::handle_read_event(rofl::cthread &thread, int fd) {

  tap_io_details *td;

  try {
    td = &sw_cbs.at(fd);
  } catch (std::exception &e) {
    LOG(ERROR) << __FUNCTION__ << ": failed to read from fd=" << fd;
    return;
  }

  size_t len = sizeof(std::size_t) + 22 + td->mtu;

  VLOG(4) << __FUNCTION__ << ": read on fd=" << fd << ", max_len=" << len;

  packet *pkt = (packet *)std::malloc(len);

  if (pkt == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": no mem left";
    return;
  }

  pkt->len = read(fd, pkt->data, len);

  if (pkt->len > 0) {
    VLOG(3) << __FUNCTION__ << ": read " << pkt->len << " bytes from fd=" << fd
            << " into pkt=" << pkt << " tid=" << pthread_self();
    assert(td->cb);
    td->cb->enqueue_to_switch(td->port_id, pkt);
  } else {
    // error occured (or non-blocking)
    switch (errno) {
    case EAGAIN:
      LOG(ERROR) << __FUNCTION__
                 << ": EAGAIN XXX not implemented packet is dropped";
      std::free(pkt);
      break;
    default:
      LOG(ERROR) << __FUNCTION__ << ": unknown error occured";
      std::free(pkt);
      break;
    }
  }
}

void tap_io::handle_write_event(rofl::cthread &thread, int fd) {
  thread.drop_write_fd(fd);
  tx();
}

void tap_io::tx() {
  std::pair<int, packet *> pkt;
  std::deque<std::pair<int, packet *>> out_queue;

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
    int fd = ev.second.fd;
    switch (ev.first) {

    case TAP_IO_ADD:
      sw_cbs[fd] = ev.second;
      VLOG(3) << __FUNCTION__ << ": register fd=" << fd
              << ", mtu=" << ev.second.mtu << ", port_id=" << ev.second.port_id;
      thread.add_read_fd(fd, true, false);
      break;
    case TAP_IO_REM:
      thread.drop_fd(fd, false);
      sw_cbs[fd] = tap_io_details();
      break;
    default:
      break;
    }
  }
  events.clear();
}

} // namespace basebox
