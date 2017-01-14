#include <glog/logging.h>

#include "cnetlink.hpp"
#include "nbi_impl.hpp"
#include "tap_manager.hpp"

#include "roflibs/netlink/ctapdev.hpp"
#include "roflibs/netlink/cpacketpool.hpp"

namespace rofcore {

nbi_impl::nbi_impl() : tap_man(new tap_manager()) {
  // start netlink
  cnetlink *nl = &cnetlink::get_instance();
  (void)nl;
}

nbi_impl::~nbi_impl() { cnetlink::get_instance().stop(); }

void nbi_impl::resend_state() noexcept {
  cnetlink::get_instance().resend_state();
}

void nbi_impl::register_switch(switch_interface *swi) noexcept {
  this->swi = swi;
  cnetlink::get_instance().register_switch(swi);
}

void nbi_impl::port_notification(
    std::deque<port_notification_data> &notifications) noexcept {

  for (auto &&ntfy : notifications) {
    switch (ntfy.ev) {
    case PORT_EVENT_ADD:
      cnetlink::get_instance().register_link(ntfy.port_id, ntfy.name);
      tap_man->create_tapdev(ntfy.port_id, ntfy.name, *this);
      break;
    case PORT_EVENT_DEL:
      cnetlink::get_instance().unregister_link(ntfy.port_id, ntfy.name);
      tap_man->destroy_tapdev(ntfy.port_id, ntfy.name);
      break;
    default:
      break;
    }
  }
}

void nbi_impl::port_status_changed(uint32_t port_no,
                                   enum nbi::port_status ps) noexcept {
  cnetlink::get_instance().port_status_changed(port_no, ps);
}

int nbi_impl::enqueue_to_switch(uint32_t port_id, rofl::cpacket *packet) {
  swi->enqueue(port_id, packet);
  return 0;
}

int nbi_impl::enqueue(uint32_t port_id, rofl::cpacket *pkt) noexcept {
  int rv = 0;
  assert(pkt);
  try {
    tap_man->enqueue(port_id, pkt);
  } catch (std::exception &e) {
    LOG(ERROR) << __FUNCTION__
               << ": failed to enqueue packet for port_id=" << port_id << ": "
               << e.what();
    rofcore::cpacketpool::get_instance().release_pkt(pkt);
    rv = -1;
  }
  return rv;
}

} // namespace rofcore
