/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 */

#include "xenia/kernel/util/stub_trace.h"

#include <cstdio>
#include <mutex>
#include <string>

#include "xenia/base/filesystem.h"
#include "xenia/base/string_util.h"
#include "xenia/cpu/ppc/ppc_context.h"
#include "xenia/base/obs/obs.h"
#include "xenia/debug/phoenix_probe.h"
#include "xenia/kernel/kernel_flags.h"
#include "xenia/kernel/kernel_state.h"

namespace xe {
namespace kernel {

namespace {
std::mutex g_stub_trace_mutex;
}  // namespace

void LogKernelStubHit(std::string_view module, std::string_view export_name,
                      std::string_view detail, uint32_t title_id,
                      uint32_t lr) {
  const auto& path = cvars::kernel_stub_hit_log;
  if (path.empty()) {
    return;
  }

  std::string line;
  line.reserve(256);
  line.append("{\"module\":\"");
  line.append(module);
  line.append("\",\"export\":\"");
  line.append(export_name);
  line.append("\"");

  if (!detail.empty()) {
    line.append(",\"detail\":\"");
    for (char c : detail) {
      if (c == '\\' || c == '"') {
        line.push_back('\\');
      }
      line.push_back(c);
    }
    line.append("\"");
  }

  if (title_id) {
    line.append(",\"title_id\":\"");
    line.append(xe::string_util::to_hex_string(title_id));
    line.append("\"");
  }
  if (lr) {
    line.append(",\"lr\":\"");
    line.append(xe::string_util::to_hex_string(lr));
    line.append("\"");
  }

  line.append("}\n");

  std::lock_guard<std::mutex> lock(g_stub_trace_mutex);
  FILE* f = filesystem::OpenFile(path, "ab");
  if (!f) {
    return;
  }
  fwrite(line.data(), 1, line.size(), f);
  fclose(f);

  debug::PhoenixProbeNotifyStubHit(module, export_name);

  obs::Event ev;
  ev.kind = obs::EventKind::kStub;
  ev.severity = obs::EventSeverity::kInfo;
  ev.domain = "Kernel";
  ev.channel = "Kernel.Stub";
  ev.code = "StubHit";
  ev.detail = export_name;
  ev.guest_lr = lr;
  obs::EmitEvent(ev);
}

void LogKernelStubHitGuest(cpu::ppc::PPCContext* ctx, std::string_view module,
                           std::string_view export_name,
                           std::string_view detail) {
  uint32_t title_id = 0;
  uint32_t lr = 0;
  if (ctx) {
    lr = static_cast<uint32_t>(ctx->lr);
    if (ctx->kernel_state) {
      title_id = ctx->kernel_state->title_id();
    }
  }
  LogKernelStubHit(module, export_name, detail, title_id, lr);
}

}  // namespace kernel
}  // namespace xe
