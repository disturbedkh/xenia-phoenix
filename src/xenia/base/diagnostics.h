/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                         *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_BASE_DIAGNOSTICS_H_
#define XENIA_BASE_DIAGNOSTICS_H_

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>

#include "xenia/base/cvar.h"

namespace xe {

struct DiagnosticsState {
  std::atomic<bool> dev_mode{false};
  std::atomic<bool> subsystem_sentinels{false};
  std::atomic<bool> force_flush_on_error{false};
  std::atomic<bool> capture_log_tail_on_crash{true};
  std::atomic<uint32_t> crash_log_tail_lines{200};
};

DiagnosticsState& diagnostics();

// Monotonic microseconds since diagnostics initialization (shared clock).
uint64_t MonotonicUs();

// Stable id for the current emulator session (assigned at startup).
const std::string& SessionId();

void InitializeDiagnostics();

}  // namespace xe

DECLARE_bool(dev_mode);
DECLARE_bool(diag_subsystem_sentinels);
DECLARE_bool(diag_force_flush_on_error);
DECLARE_bool(diag_capture_log_tail_on_crash);
DECLARE_uint32(diag_crash_log_tail_lines);

#endif  // XENIA_BASE_DIAGNOSTICS_H_
