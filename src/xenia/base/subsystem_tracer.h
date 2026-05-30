/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                         *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_BASE_SUBSYSTEM_TRACER_H_
#define XENIA_BASE_SUBSYSTEM_TRACER_H_

#include <cstdint>

namespace xe {

class SubsystemTracer {
 public:
  SubsystemTracer(const char* subsystem, const char* phase);
  ~SubsystemTracer();

  SubsystemTracer(const SubsystemTracer&) = delete;
  SubsystemTracer& operator=(const SubsystemTracer&) = delete;

 private:
  const char* subsystem_;
  const char* phase_;
  uint64_t enter_us_;
  bool enabled_;
};

}  // namespace xe

#define XE_SUBSYSTEM_TRACE_CONCAT_INNER(a, b) a##b
#define XE_SUBSYSTEM_TRACE_CONCAT(a, b) XE_SUBSYSTEM_TRACE_CONCAT_INNER(a, b)
#define XE_SUBSYSTEM_TRACE(sub, phase) \
  ::xe::SubsystemTracer XE_SUBSYSTEM_TRACE_CONCAT(_xst_, __LINE__)(sub, phase)

#endif  // XENIA_BASE_SUBSYSTEM_TRACER_H_
