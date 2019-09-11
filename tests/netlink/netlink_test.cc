#include "netlink/cnetlink.h"

#include <netlink/route/link.h>
#include "gtest/gtest.h"

namespace basebox {

TEST(cnetlink,  DISABLED_is_bridge_interface_false) {
  basebox::cnetlink test;
  std::unique_ptr<rtnl_link, decltype(&rtnl_link_put)> link(rtnl_link_alloc(),
                                                              &rtnl_link_put);

  rtnl_link_set_ifindex(link.get(), 1);
  rtnl_link_set_family(link.get(), AF_UNSPEC);

  auto res = test.is_bridge_interface(link.get());
  EXPECT_FALSE(res);
}

// Test case not covered
TEST(cnetlink, DISABLED_is_bridge_interface_bridge) {
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

  rtnl_link_set_ifindex(bridge.get(), 1);
  rtnl_link_set_family(bridge.get(), AF_BRIDGE);

  // bridge slave declaration
  std::unique_ptr<rtnl_link, decltype(&rtnl_link_put)> slave(rtnl_link_alloc(),
                                                              &rtnl_link_put);
  rtnl_link_set_ifindex(slave.get(), 2);
  rtnl_link_set_family(slave.get(), AF_UNSPEC);

  auto res = test.is_bridge_interface(slave.get());
  EXPECT_TRUE(res);
}

}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
