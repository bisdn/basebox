#pragma once

#include <netlink/object.h>

namespace rofcore {

class nl_obj {
public:
  nl_obj(struct nl_object *obj);
  nl_obj(const nl_obj &other);
  nl_obj(nl_obj &&other) noexcept;
  nl_obj &operator=(nl_obj &&other) noexcept;

  const struct nl_object *get_obj() const { return obj; }
  ~nl_obj();

private:
  struct nl_object *obj;
};

} // namespace rofcore
