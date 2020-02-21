/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <glog/logging.h>

#include "cnetlink.h"
#include "nbi_impl.h"
#include "tap_manager.h"

#include "netlink/ctapdev.h"
#include "utils/utils.h"

namespace basebox {

nbi_impl::nbi_impl(std::shared_ptr<cnetlink> nl,
                   std::shared_ptr<tap_manager> tap_man)
    : nl(nl), tap_man(tap_man) {
  nl->set_tapmanager(tap_man);
}

nbi_impl::~nbi_impl() { nl->stop(); }

void nbi_impl::resend_state() noexcept { nl->resend_state(); }

void nbi_impl::register_switch(switch_interface *swi) noexcept {
  this->swi = swi;
  nl->register_switch(swi);
}

void nbi_impl::switch_state_notification(enum switch_state state) noexcept {
  switch (state) {
  case SWITCH_STATE_UP:
    nl->start();
    break;
  case SWITCH_STATE_DOWN:
  case SWITCH_STATE_FAILED:
  case SWITCH_STATE_UNKNOWN:
    nl->stop();
    break;
  default:
    LOG(FATAL) << __FUNCTION__ << ": invalid state";
    break;
  }
}

void nbi_impl::port_notification(
    std::deque<port_notification_data> &notifications) noexcept {

  for (auto &&ntfy : notifications) {
    switch (ntfy.ev) {
    case PORT_EVENT_MODIFY:
      switch (get_port_type(ntfy.port_id)) {
      case nbi::port_type_physical:
        LOG(INFO) << __FUNCTION__ << ": port state changed, continuing";
        break;
      default:
        LOG(ERROR) << __FUNCTION__ << ": unknown port";
        break;
      }
    case PORT_EVENT_ADD:
      switch (get_port_type(ntfy.port_id)) {
      case nbi::port_type_physical:
        tap_man->create_tapdev(ntfy.port_id, ntfy.name, *this);
        tap_man->change_port_status(ntfy.name, ntfy.status);
        break;
      case nbi::port_type_vxlan:
        // XXX TODO notify this?
        LOG(INFO) << __FUNCTION__ << ": port_type_vxlan added";
        break;
      default:
        LOG(ERROR) << __FUNCTION__ << ": unknown port";
        break;
      }
      break;
    case PORT_EVENT_DEL:
      switch (get_port_type(ntfy.port_id)) {
      case nbi::port_type_physical:
        tap_man->destroy_tapdev(ntfy.port_id, ntfy.name);
        break;
      case nbi::port_type_vxlan:
        // XXX TODO notify this?
        LOG(INFO) << __FUNCTION__ << ": port_type_vxlan removed";
        break;
      default:
        LOG(ERROR) << __FUNCTION__ << ": unknown port";
        break;
      }
      break;
    default:
      break;
    }
  }
}

int nbi_impl::enqueue_to_switch(uint32_t port_id, basebox::packet *packet) {
  swi->enqueue(port_id, packet);
  return 0;
}

int nbi_impl::enqueue(uint32_t port_id, basebox::packet *pkt) noexcept {
  int rv = 0;
  assert(pkt);
  try {
    // detour via netlink to learn the source mac
    int fd = tap_man->get_fd(port_id);
    nl->learn_l2(port_id, fd, pkt);
  } catch (std::exception &e) {
    LOG(ERROR) << __FUNCTION__
               << ": failed to enqueue packet for port_id=" << port_id << ": "
               << e.what();
    std::free(pkt);
    rv = -1;
  }
  return rv;
}

int nbi_impl::fdb_timeout(uint32_t port_id, uint16_t vid,
                          const rofl::caddress_ll &mac) noexcept {
  nl->fdb_timeout(port_id, vid, mac);
  return 0;
}

} // namespace basebox
