/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Tier 0 (MVP4): optional JSONL log of unimplemented / stub kernel paths.
 ******************************************************************************
 */

#ifndef XENIA_KERNEL_UTIL_STUB_TRACE_H_
#define XENIA_KERNEL_UTIL_STUB_TRACE_H_

#include <string_view>

namespace xe {
namespace kernel {

// If cvar kernel_stub_hit_log is non-empty, appends one JSON object per line:
// {"module":"xboxkrnl","export":"...","detail":"..."}
// Thread-safe; low overhead when the cvar path is empty.
void LogKernelStubHit(std::string_view module, std::string_view export_name,
                      std::string_view detail = {});

}  // namespace kernel
}  // namespace xe

#endif  // XENIA_KERNEL_UTIL_STUB_TRACE_H_
