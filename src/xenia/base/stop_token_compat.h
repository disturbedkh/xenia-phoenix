/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Canary. All rights reserved.                          *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_BASE_STOP_TOKEN_COMPAT_H_
#define XENIA_BASE_STOP_TOKEN_COMPAT_H_

#include "xenia/base/platform.h"

#if XE_PLATFORM_ANDROID
#include <atomic>
#include <memory>

class XeStopToken {
 public:
  XeStopToken() = default;
  explicit XeStopToken(std::shared_ptr<std::atomic<bool>> flag)
      : flag_(std::move(flag)) {}

  bool stop_requested() const {
    return flag_ && flag_->load(std::memory_order_relaxed);
  }

 private:
  std::shared_ptr<std::atomic<bool>> flag_;
};

class XeStopSource {
 public:
  XeStopSource() : flag_(std::make_shared<std::atomic<bool>>(false)) {}

  XeStopToken get_token() const { return XeStopToken(flag_); }
  void request_stop() {
    if (flag_) {
      flag_->store(true, std::memory_order_relaxed);
    }
  }
  bool stop_requested() const { return get_token().stop_requested(); }

 private:
  std::shared_ptr<std::atomic<bool>> flag_;
};

#else
#include <stop_token>

using XeStopToken = std::stop_token;
using XeStopSource = std::stop_source;
#endif

#endif  // XENIA_BASE_STOP_TOKEN_COMPAT_H_
