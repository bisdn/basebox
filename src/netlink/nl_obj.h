// SPDX-FileCopyrightText: © 2016 BISDN GmbH
//
// SPDX-License-Identifier: MPL-2.0-no-copyleft-exception

#pragma once

#include <netlink/cache.h>
#include <netlink/object.h>

namespace basebox {

class nl_obj {
public:
  nl_obj(int action, struct nl_object *old_obj, struct nl_object *new_obj);
  nl_obj(const nl_obj &other);
  nl_obj(nl_obj &&other) = default;
  nl_obj &operator=(nl_obj &other) noexcept;
  nl_obj &operator=(nl_obj &&other) noexcept;
  ~nl_obj();

  int get_action() const { return action; }
  int get_msg_type() const { return nl_object_get_msgtype(get_obj()); }
  struct nl_object *get_old_obj() const {
    return old_obj;
  }
  struct nl_object *get_new_obj() const {
    return new_obj;
  }

private:
  struct nl_object *get_obj() const {
    struct nl_object *obj;
    switch (action) {
    case NL_ACT_DEL:
      obj = old_obj;
      break;
    case NL_ACT_CHANGE:
    case NL_ACT_NEW:
      obj = new_obj;
      break;
    default:
      obj = nullptr;
      break;
    }
    return obj;
  }

  void increment_refcount();

  int action;
  struct nl_object *old_obj;
  struct nl_object *new_obj;
};

} // namespace basebox
