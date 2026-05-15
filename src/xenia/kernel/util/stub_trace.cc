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
#include "xenia/kernel/kernel_flags.h"

namespace xe {
namespace kernel {

namespace {
std::mutex g_stub_trace_mutex;
}  // namespace

void LogKernelStubHit(std::string_view module, std::string_view export_name,
                      std::string_view detail) {
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

  line.append("}\n");

  std::lock_guard<std::mutex> lock(g_stub_trace_mutex);
  FILE* f = filesystem::OpenFile(path, "ab");
  if (!f) {
    return;
  }
  fwrite(line.data(), 1, line.size(), f);
  fclose(f);
}

}  // namespace kernel
}  // namespace xe
