#pragma once

#include <memory>

#include "sai.hpp"
#include "tap_manager.hpp"

namespace basebox {

class cnetlink;
class tap_manager;

class nbi_impl : public nbi, public switch_callback {
  std::shared_ptr<tap_manager> tap_man;
  switch_interface *swi;
  std::unique_ptr<cnetlink> nl;

public:
  nbi_impl(std::shared_ptr<tap_manager> tap_man);
  virtual ~nbi_impl();

  // nbi
  void register_switch(switch_interface *) noexcept override;
  void resend_state() noexcept override;
  void
  port_notification(std::deque<port_notification_data> &) noexcept override;
  void port_status_changed(uint32_t port, enum port_status) noexcept override;
  int enqueue(uint32_t port_id, basebox::packet *pkt) noexcept override;

  // tap_callback
  int enqueue_to_switch(uint32_t port_id, struct basebox::packet *) override;
};

} // namespace basebox
