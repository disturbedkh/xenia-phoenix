/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Canary. All rights reserved.                          *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_BASE_ATOMIC_SHARED_PTR_H_
#define XENIA_BASE_ATOMIC_SHARED_PTR_H_

#include <atomic>
#include <memory>

#include "xenia/base/platform.h"

// std::atomic<std::shared_ptr<T>> (C++20) is not implemented by the Android
// NDK r26c libc++; it falls back to the primary template which requires a
// trivially copyable type and fails to compile. Provide a mutex-guarded
// equivalent on Android with the subset of the std::atomic interface this
// codebase uses (load/store with a memory_order argument).
#if XE_PLATFORM_ANDROID
#include <mutex>

namespace xe {

template <typename T>
class AtomicSharedPtr {
 public:
  AtomicSharedPtr() = default;
  explicit AtomicSharedPtr(std::shared_ptr<T> value)
      : value_(std::move(value)) {}

  std::shared_ptr<T> load(std::memory_order = std::memory_order_seq_cst) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return value_;
  }

  void store(std::shared_ptr<T> value,
             std::memory_order = std::memory_order_seq_cst) {
    std::lock_guard<std::mutex> lock(mutex_);
    value_ = std::move(value);
  }

 private:
  mutable std::mutex mutex_;
  std::shared_ptr<T> value_;
};

}  // namespace xe
#else
namespace xe {

template <typename T>
using AtomicSharedPtr = std::atomic<std::shared_ptr<T> >;

}  // namespace xe
#endif

#endif  // XENIA_BASE_ATOMIC_SHARED_PTR_H_
