#include <cassert>
#include <map>
#include <memory>
#include <vector>

#include <glog/logging.h>
#include <netlink/route/link.h>

#include "netlink-utils.hpp"

#define lt_names "unknown", "unsupported", "bridge", "tun"

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

} // namespace basebox
