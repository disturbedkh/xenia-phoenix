/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Tier 0 (MVP4): optional JSONL log of unimplemented / stub kernel paths.
 ******************************************************************************
 */

#ifndef XENIA_KERNEL_UTIL_STUB_TRACE_H_
#define XENIA_KERNEL_UTIL_STUB_TRACE_H_

#include <cstdint>
#include <string_view>

#include "xenia/cpu/ppc/ppc_context.h"

namespace xe {
namespace kernel {

// If cvar kernel_stub_hit_log is non-empty, appends one JSON object per line:
// {"module":"...","export":"...","detail":"...","title_id":"0x........","lr":"0x........"}
// Thread-safe; low overhead when the cvar path is empty.
void LogKernelStubHit(std::string_view module, std::string_view export_name,
                      std::string_view detail = {}, uint32_t title_id = 0,
                      uint32_t lr = 0);

// When called from a guest export, records title_id and lr from PPCContext when
// set.
void LogKernelStubHitGuest(cpu::ppc::PPCContext* ctx, std::string_view module,
                           std::string_view export_name,
                           std::string_view detail = {});

}  // namespace kernel
}  // namespace xe

#endif  // XENIA_KERNEL_UTIL_STUB_TRACE_H_
