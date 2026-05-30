/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                         *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/base/subsystem_tracer.h"

#include "xenia/base/diagnostics.h"
#include "xenia/base/logging.h"

namespace xe {

SubsystemTracer::SubsystemTracer(const char* subsystem, const char* phase)
    : subsystem_(subsystem), phase_(phase), enter_us_(0), enabled_(false) {
  if (!diagnostics().subsystem_sentinels.load(std::memory_order_relaxed)) {
    return;
  }
  enabled_ = true;
  enter_us_ = MonotonicUs();
  XELOGW("[subsystem] {}::{} enter", subsystem_, phase_);
}

SubsystemTracer::~SubsystemTracer() {
  if (!enabled_) {
    return;
  }
  const uint64_t elapsed_us = MonotonicUs() - enter_us_;
  XELOGW("[subsystem] {}::{} exit (+{:.3f} ms)", subsystem_, phase_,
         static_cast<double>(elapsed_us) / 1000.0);
}

}  // namespace xe
