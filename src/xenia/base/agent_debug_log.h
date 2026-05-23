/**
 * Temporary NDJSON debug logging for agent debug sessions.
 */
#pragma once

#include <chrono>
#include <fstream>
#include <string>

#include "third_party/fmt/include/fmt/format.h"

namespace xe::agent_debug {

inline const char* LogFilePath() {
  return R"(G:\Xenia-Xenia Canary\debug-6f7947.log)";
}

template <typename... Args>
inline void Log(const char* location, const char* message,
                const char* hypothesis_id, const char* run_id,
                fmt::format_string<Args...> data_fmt, Args&&... data_args) {
  // #region agent log
  const auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count();
  const std::string data =
      fmt::format(data_fmt, std::forward<Args>(data_args)...);
  std::ofstream out(LogFilePath(), std::ios::app);
  if (!out) {
    return;
  }
  out << fmt::format(
             R"({{"sessionId":"6f7947","timestamp":{},"location":"{}","message":"{}","hypothesisId":"{}","runId":"{}","data":{}}})",
             ts, location, message, hypothesis_id, run_id, data)
      << "\n";
  // #endregion
}

}  // namespace xe::agent_debug
