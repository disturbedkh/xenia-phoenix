/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                         *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/base/diagnostics.h"

#include "xenia/base/cvar.h"

DEFINE_bool(dev_mode, false,
            "Enable Phoenix Dev Mode diagnostics profile (verbose logging, "
            "subsystem sentinels).",
            "Diagnostics");

DEFINE_bool(diag_subsystem_sentinels, false,
            "Log subsystem ctor/dtor/Setup/Shutdown enter and exit.",
            "Diagnostics");

DEFINE_bool(diag_force_flush_on_error, false,
            "Flush the log file immediately after each Error/Warning line.",
            "Diagnostics");

DEFINE_bool(diag_capture_log_tail_on_crash, true,
            "Append recent log lines to crash sidecar reports.", "Diagnostics");

DEFINE_uint32(diag_crash_log_tail_lines, 200,
              "Number of recent log lines to include in crash sidecars.",
              "Diagnostics");

namespace xe {

namespace {

DiagnosticsState g_diagnostics;
std::chrono::steady_clock::time_point g_steady_start;
std::string g_session_id;

}  // namespace

DiagnosticsState& diagnostics() { return g_diagnostics; }

uint64_t MonotonicUs() {
  const auto now = std::chrono::steady_clock::now();
  return static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::microseconds>(now -
                                                            g_steady_start)
          .count());
}

const std::string& SessionId() { return g_session_id; }

void InitializeDiagnostics() {
  g_steady_start = std::chrono::steady_clock::now();

  const auto wall = std::chrono::system_clock::now();
  const auto epoch_us = std::chrono::duration_cast<std::chrono::microseconds>(
                            wall.time_since_epoch())
                            .count();
  g_session_id = std::to_string(epoch_us);

  g_diagnostics.dev_mode.store(cvars::dev_mode, std::memory_order_relaxed);
  g_diagnostics.subsystem_sentinels.store(cvars::diag_subsystem_sentinels,
                                          std::memory_order_relaxed);
  g_diagnostics.force_flush_on_error.store(cvars::diag_force_flush_on_error,
                                           std::memory_order_relaxed);
  g_diagnostics.capture_log_tail_on_crash.store(
      cvars::diag_capture_log_tail_on_crash, std::memory_order_relaxed);
  g_diagnostics.crash_log_tail_lines.store(cvars::diag_crash_log_tail_lines,
                                           std::memory_order_relaxed);
}

}  // namespace xe
