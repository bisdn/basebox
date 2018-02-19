#include "netlink-utils.hpp"

uint64_t nlall2uint64(const nl_addr *a) {
  uint64_t b = 0;
  char *b_, *a_;

  assert(nl_addr_get_len(a) == 6);

  a_ = static_cast<char *>(nl_addr_get_binary_addr(a));
  b_ = reinterpret_cast<char *>(&b);
  b_[5] = a_[0];
  b_[4] = a_[1];
  b_[3] = a_[2];
  b_[2] = a_[3];
  b_[1] = a_[4];
  b_[0] = a_[5];
  return b;
}
