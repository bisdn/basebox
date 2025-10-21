// SPDX-FileCopyrightText: © 2016 BISDN GmbH
// SPDX-License-Identifier: MPL-2.0-no-copyleft-exception

#include "nl_obj.h"
#include <glog/logging.h>

namespace basebox {

nl_obj::nl_obj(int action, struct nl_object *old_obj, struct nl_object *new_obj)
    : action(action), old_obj(old_obj), new_obj(new_obj) {
  increment_refcount();
  VLOG(2) << "created nl_obj=" << this << " (old_obj=" << old_obj
          << " new_obj=" << new_obj << ")";
}

nl_obj::nl_obj(const nl_obj &other)
    : action(other.action), old_obj(other.old_obj), new_obj(other.new_obj) {
  increment_refcount();
  VLOG(2) << "copied nl_obj=" << this << " other=" << &other
          << " (old_obj=" << old_obj << " new_obj=" << new_obj << ")";
}

nl_obj &nl_obj::operator=(nl_obj &other) noexcept {
  action = other.action;
  old_obj = other.old_obj;
  new_obj = other.new_obj;
  increment_refcount();
  return *this;
}

nl_obj &nl_obj::operator=(nl_obj &&other) noexcept {
  action = other.action;
  old_obj = other.old_obj;
  new_obj = other.new_obj;
  other.action = NL_ACT_UNSPEC;
  other.old_obj = other.new_obj = nullptr;
  return *this;
}

nl_obj::~nl_obj() {
  switch (action) {
  case NL_ACT_NEW:
    nl_object_put(new_obj);
    break;
  case NL_ACT_DEL:
    nl_object_put(old_obj);
    break;
  case NL_ACT_CHANGE:
    nl_object_put(old_obj);
    nl_object_put(new_obj);
    break;
  default:
    LOG(FATAL) << "invalid action";
    break;
  }
  VLOG(2) << "destroyed nl_obj=" << this << " (old_obj=" << old_obj
          << " new_obj=" << new_obj << ")";
}

void nl_obj::increment_refcount() {
  switch (action) {
  case NL_ACT_NEW:
    nl_object_get(new_obj);
    break;
  case NL_ACT_DEL:
    nl_object_get(old_obj);
    break;
  case NL_ACT_CHANGE:
    nl_object_get(old_obj);
    nl_object_get(new_obj);
    break;
  default:
    LOG(FATAL) << "invalid action";
    break;
  }
  VLOG(2) << "incremented refcounts nl_obj=" << this << " (old_obj=" << old_obj
          << " new_obj=" << new_obj << ")";
}
} // namespace basebox
