#include "netlink/cnetlink.h"
#include "netlink/ctapdev.h"

#include <netlink/route/link.h>
#include "gtest/gtest.h"

namespace basebox {

TEST(cnetlink,  is_bridge_interface_false) {
  basebox::cnetlink test;
  std::unique_ptr<rtnl_link, decltype(&rtnl_link_put)> link(rtnl_link_alloc(),
                                                              &rtnl_link_put);

  rtnl_link_set_ifindex(link.get(), 1);
  rtnl_link_set_family(link.get(), AF_UNSPEC);

  auto res = test.is_bridge_interface(link.get());
  EXPECT_FALSE(res);
}

// Test case not covered
TEST(cnetlink, is_bridge_interface_bridge) {
  basebox::cnetlink test;
  std::unique_ptr<rtnl_link, decltype(&rtnl_link_put)> link(rtnl_link_alloc(),
                                                              &rtnl_link_put);

  rtnl_link_set_ifindex(link.get(), 2);
  rtnl_link_set_family(link.get(), AF_BRIDGE);

  auto res = test.is_bridge_interface(link.get());
  EXPECT_TRUE(res);
}

TEST(cnetlink, is_bridge_interface_bridge_slave) {
  basebox::cnetlink test;

  // bridge declaration
  std::unique_ptr<rtnl_link, decltype(&rtnl_link_put)> bridge(rtnl_link_bridge_alloc(),
                                                              &rtnl_link_put);

  rtnl_link_set_ifindex(bridge.get(), 123);
  rtnl_link_set_name(bridge.get(), "bridge0");
  rtnl_link_set_family(bridge.get(), AF_BRIDGE);

  // bridge slave declaration
  std::unique_ptr<rtnl_link, decltype(&rtnl_link_put)> slave(rtnl_link_alloc(),
                                                              &rtnl_link_put);

  rtnl_link_set_name(slave.get(), "port1");
  rtnl_link_set_type(slave.get(), "vlan");
  rtnl_link_set_master(slave.get(), 123);

  auto res = test.is_bridge_interface(slave.get());
  EXPECT_TRUE(res);
}
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
