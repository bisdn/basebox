#include <memory>

namespace basebox {

class cnetlink;
class switch_interface;
class tap_manager;

class nl_vlan {
public:
  nl_vlan(std::shared_ptr<tap_manager> tap_man, cnetlink *nl);
  ~nl_vlan() {}

  void register_switch_interface(switch_interface *swi) { this->swi = swi; }

  int add_vlan(int ifindex);
  int remove_vlan(int ifindex);

private:
  switch_interface *swi;
  std::shared_ptr<tap_manager> tap_man;
};

} // namespace basebox
