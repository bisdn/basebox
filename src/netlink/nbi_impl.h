/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <memory>

#include "sai.h"
#include "tap_manager.h"

namespace basebox {

class cnetlink;
class tap_manager;

class nbi_impl : public nbi, public switch_callback {
  switch_interface *swi;
  std::shared_ptr<cnetlink> nl;
  std::shared_ptr<tap_manager> tap_man;

public:
  nbi_impl(std::shared_ptr<cnetlink> nl, std::shared_ptr<tap_manager> tap_man);
  virtual ~nbi_impl();

  // nbi
  void register_switch(switch_interface *) noexcept override;
  void resend_state() noexcept override;
  void
  port_notification(std::deque<port_notification_data> &) noexcept override;
  int enqueue(uint32_t port_id, basebox::packet *pkt) noexcept override;
  int fdb_timeout(uint32_t port_id, uint16_t vid,
                  const rofl::caddress_ll &mac) noexcept override;

  // tap_callback
  int enqueue_to_switch(uint32_t port_id, struct basebox::packet *) override;

  std::shared_ptr<tap_manager> get_tapmanager() { return tap_man; }
};

} // namespace basebox
