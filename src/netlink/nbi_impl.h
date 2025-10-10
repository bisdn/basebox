// SPDX-FileCopyrightText: © 2016 BISDN GmbH
//
// SPDX-License-Identifier: MPL-2.0-no-copyleft-exception

#pragma once

#include <memory>

#include "sai.h"
#include "port_manager.h"

namespace basebox {

class cnetlink;
class port_manager;

class nbi_impl : public nbi, public switch_callback {
  switch_interface *swi;
  std::shared_ptr<cnetlink> nl;
  std::shared_ptr<port_manager> port_man;

public:
  nbi_impl(std::shared_ptr<cnetlink> nl,
           std::shared_ptr<port_manager> port_man);
  virtual ~nbi_impl();

  // nbi
  void register_switch(switch_interface *) noexcept override;
  void switch_state_notification(enum switch_state) noexcept override;
  void resend_state() noexcept override;
  void
  port_notification(std::deque<port_notification_data> &) noexcept override;
  int enqueue(uint32_t port_id, basebox::packet *pkt) noexcept override;
  int fdb_timeout(uint32_t port_id, uint16_t vid,
                  const rofl::caddress_ll &mac) noexcept override;

  // tap_callback
  int enqueue_to_switch(uint32_t port_id, struct basebox::packet *) override;

  std::shared_ptr<port_manager> get_tapmanager() { return port_man; }
};

} // namespace basebox
