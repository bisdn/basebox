#include <memory>

extern "C" {
struct rtnl_link;
}

namespace basebox {

class cnetlink;
class switch_interface;

class nl_vlan {
public:
  nl_vlan(cnetlink *nl);
  ~nl_vlan() {}

  void register_switch_interface(switch_interface *swi) { this->swi = swi; }

  int add_vlan(rtnl_link *link, uint16_t vid, bool tagged) const;
  int remove_vlan(rtnl_link *link, uint16_t vid, bool tagged) const;

  uint16_t get_vid(rtnl_link *link);

  bool is_vid_valid(uint16_t vid) const {
    if (vid < vid_low || vid > vid_high)
      return false;
    return true;
  }

private:
  static const uint16_t vid_low = 1;
  static const uint16_t vid_high = 0xfff;
  static const uint16_t default_vid = vid_low;

  switch_interface *swi;
  cnetlink *nl;
};

} // namespace basebox
