#include <glog/logging.h>

#include "cnetlink.hpp"
#include "nbi_impl.hpp"
#include "tap_manager.hpp"

#include "roflibs/netlink/ctapdev.hpp"

namespace rofcore {

nbi_impl::nbi_impl() : tap_man(new tap_manager()) {
  // start netlink
  cnetlink *nl = &cnetlink::get_instance();
  (void)nl;
}

void nbi_impl::resend_state() noexcept {
  cnetlink::get_instance().resend_state();
}

void nbi_impl::register_switch(switch_interface *swi) noexcept {
  this->swi = swi;
  cnetlink::get_instance().register_switch(swi);
}

void nbi_impl::port_notification(
    std::deque<port_notification_data> &notifications) noexcept {
  std::deque<port_notification_data> add;
  std::deque<port_notification_data> del;

  for (auto &&ntfy : notifications) {
    switch (ntfy.ev) {
    case PORT_EVENT_ADD:
      add.push_back(ntfy);
      break;
    case PORT_EVENT_DEL:
      del.push_back(ntfy);
      break;
    default:
      break;
    }
  }

  // XXX del not implemented
  tap_man->register_tapdevs(add, *this);
  cnetlink::get_instance().start();
  tap_man->start();
}

void nbi_impl::port_status_changed(uint32_t port_no,
                                   enum nbi::port_status ps) noexcept {
  // TODO keep info of state?
  cnetlink::get_instance().port_status_changed(port_no, ps);
}

int nbi_impl::enqueue_to_switch(uint32_t port_id, rofl::cpacket *packet) {
  swi->enqueue(port_id, packet);
  return 0;
}

int nbi_impl::enqueue(uint32_t port_id, rofl::cpacket *pkt) noexcept {
  try {
    tap_man->get_dev(tap_man->get_tapdev_id(port_id)).enqueue(pkt);
  } catch (std::exception &e) {
    LOG(ERROR) << __FUNCTION__ << ": failed to enqueue packet: " << e.what();
  }
  return 0;
}

} // namespace rofcore
