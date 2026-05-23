/**
 ******************************************************************************
 * Buffered JSONL file sink.
 ******************************************************************************
 */

#include "xenia/base/obs/obs_jsonl_sink.h"

#include <cstdio>

#include "xenia/base/filesystem.h"

namespace xe {
namespace obs {

std::string JsonEscape(std::string_view s) {
  std::string out;
  out.reserve(s.size() + 16);
  for (unsigned char c : s) {
    switch (c) {
      case '\\':
        out.append("\\\\");
        break;
      case '"':
        out.append("\\\"");
        break;
      case '\b':
        out.append("\\b");
        break;
      case '\f':
        out.append("\\f");
        break;
      case '\n':
        out.append("\\n");
        break;
      case '\r':
        out.append("\\r");
        break;
      case '\t':
        out.append("\\t");
        break;
      default:
        if (c < 0x20) {
          char buf[8];
          std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned>(c));
          out.append(buf);
        } else {
          out.push_back(static_cast<char>(c));
        }
        break;
    }
  }
  return out;
}

void JsonlSink::Configure(const std::filesystem::path& path, bool enabled) {
  std::lock_guard<std::mutex> lock(mutex_);
  Close();
  path_ = path;
  enabled_ = enabled && !path.empty();
  if (enabled_) {
    std::filesystem::create_directories(path_.parent_path());
    file_ = filesystem::OpenFile(path_, "ab");
  }
}

void JsonlSink::WriteLine(const std::string& line) {
  if (!enabled_ || line.empty()) {
    return;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  if (!file_) {
    file_ = filesystem::OpenFile(path_, "ab");
    if (!file_) {
      return;
    }
  }
  fwrite(line.data(), 1, line.size(), file_);
  bytes_since_flush_ += line.size();
  if (bytes_since_flush_ >= kFlushThreshold) {
    fflush(file_);
    bytes_since_flush_ = 0;
  }
}

void JsonlSink::Flush() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (file_) {
    fflush(file_);
    bytes_since_flush_ = 0;
  }
}

void JsonlSink::Close() {
  if (file_) {
    fflush(file_);
    fclose(file_);
    file_ = nullptr;
  }
  bytes_since_flush_ = 0;
}

}  // namespace obs
}  // namespace xe
