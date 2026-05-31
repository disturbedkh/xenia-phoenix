/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2022 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_KERNEL_XAM_XAM_NET_H_
#define XENIA_KERNEL_XAM_XAM_NET_H_

#include <future>
#include <map>
#include <memory>
#include <mutex>

#include "xenia/base/platform.h"

namespace xe {
namespace kernel {
namespace xam {

// Very hacky
bool EXPLICIT_XBOXLIVE_KEY = false;

std::vector<std::future<int32_t>> upnp_actions_;

#if !XE_PLATFORM_ANDROID
std::map<uint32_t, std::stop_source> qos_lookup_threads;
std::mutex qos_lookup_mutex;

std::map<uint32_t, std::stop_source> dns_lookup_threads;
std::mutex dns_lookup_mutex;
#else
using CancelFlag = std::shared_ptr<std::atomic<bool>>;
std::map<uint32_t, CancelFlag> qos_lookup_threads;
std::mutex qos_lookup_mutex;

std::map<uint32_t, CancelFlag> dns_lookup_threads;
std::mutex dns_lookup_mutex;
#endif

static void CleanupUPnPActions();

}  // namespace xam
}  // namespace kernel
}  // namespace xe

#endif  // XENIA_KERNEL_XAM_XAM_NET_H_
