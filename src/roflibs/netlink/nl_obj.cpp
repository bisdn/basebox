#include "nl_obj.hpp"

namespace rofcore {

nl_obj::nl_obj(struct nl_object *obj) : obj(obj) { nl_object_get(obj); }

nl_obj::nl_obj(const nl_obj &other) : obj(other.obj) { nl_object_get(obj); }

nl_obj::nl_obj(nl_obj &&other) noexcept : obj(other.obj) {
  other.obj = nullptr;
}

nl_obj &nl_obj::operator=(nl_obj &&other) noexcept {
  nl_object_put(obj);
  obj = other.obj;
  other.obj = nullptr;
  return *this;
}

nl_obj::~nl_obj() { nl_object_put(obj); }

} // namespace rofcore
