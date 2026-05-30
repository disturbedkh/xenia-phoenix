/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/base/threading.h"

#include <cstring>

namespace xe {
namespace threading {

thread_local char current_thread_name_[16] = {};

void set_current_thread_name_storage(std::string_view name) {
  const size_t n = std::min(name.size(), sizeof(current_thread_name_) - 1);
  if (n) {
    std::memcpy(current_thread_name_, name.data(), n);
  }
  current_thread_name_[n] = '\0';
}

std::string_view current_thread_name() {
  return current_thread_name_[0] ? std::string_view(current_thread_name_)
                                 : std::string_view{};
}

uint32_t logical_processor_count() {
  static uint32_t value = 0;
  if (!value) {
    value = std::thread::hardware_concurrency();
  }
  return value;
}

thread_local uint32_t current_thread_id_ = UINT_MAX;

uint32_t current_thread_id() {
  return current_thread_id_ == UINT_MAX ? current_thread_system_id()
                                        : current_thread_id_;
}

void set_current_thread_id(uint32_t id) { current_thread_id_ = id; }

}  // namespace threading
}  // namespace xe
