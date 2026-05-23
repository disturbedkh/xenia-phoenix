/**
 ******************************************************************************
 * Buffered JSONL file sink for observability events.
 ******************************************************************************
 */
#pragma once

#include <cstddef>
#include <filesystem>
#include <mutex>
#include <string>

namespace xe {
namespace obs {

class JsonlSink {
 public:
  void Configure(const std::filesystem::path& path, bool enabled);
  void WriteLine(const std::string& line);
  void Flush();
  void Close();

  bool enabled() const { return enabled_; }
  const std::filesystem::path& path() const { return path_; }

 private:
  std::mutex mutex_;
  std::filesystem::path path_;
  FILE* file_ = nullptr;
  bool enabled_ = false;
  size_t bytes_since_flush_ = 0;
  static constexpr size_t kFlushThreshold = 64 * 1024;
};

std::string JsonEscape(std::string_view s);

}  // namespace obs
}  // namespace xe
