#include <map>
#include <memory>

#include <glog/logging.h>
#include <netlink/route/link.h>

#include "netlink-utils.hpp"

#define lt_names "unknown", "unsupported", "bridge", "tun", "vxlan"

namespace basebox {

std::map<std::string, enum link_type> kind2lt;
std::vector<std::string> lt2names = {lt_names};

static void init_kind2lt_map() {
  enum link_type lt = LT_UNKNOWN;
  for (const auto &n : lt2names) {
    assert(lt != LT_MAX);
    kind2lt.emplace(n, lt);
    lt = static_cast<enum link_type>(lt + 1);
  }
}

enum link_type kind_to_link_type(const char *type) noexcept {
  if (type == nullptr)
    return LT_UNKNOWN;

  assert(lt2names.size() == LT_MAX);

  if (kind2lt.size() == 0) {
    init_kind2lt_map();
  }

  auto it = kind2lt.find(std::string(type)); // XXX string_view

  if (it != kind2lt.end())
    return it->second;

  VLOG(1) << __FUNCTION__ << ": type=" << type << " not supported";
  return LT_UNSUPPORTED;
}

uint64_t nlall2uint64(const nl_addr *a) noexcept {
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

void get_bridge_ports(int br_ifindex, struct nl_cache *link_cache,
                      std::deque<rtnl_link *> *list) noexcept {
  assert(link_cache);
  assert(list);

  std::unique_ptr<rtnl_link, void (*)(rtnl_link *)> filter(rtnl_link_alloc(),
                                                           &rtnl_link_put);
  assert(filter && "out of memory");

  rtnl_link_set_family(filter.get(), AF_BRIDGE);
  rtnl_link_set_master(filter.get(), br_ifindex);

  nl_cache_foreach_filter(link_cache, OBJ_CAST(filter.get()),
                          [](struct nl_object *obj, void *arg) {
                            assert(arg);
                            std::deque<rtnl_link *> *list =
                                static_cast<std::deque<rtnl_link *> *>(arg);

                            VLOG(3) << __FUNCTION__ << ": found bridge port "
                                    << obj;
                            list->push_back(LINK_CAST(obj));
                          },
                          list);
}

} // namespace basebox
