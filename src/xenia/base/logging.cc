/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2022 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/base/logging.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <cstring>

#include "third_party/disruptorplus/include/disruptorplus/multi_threaded_claim_strategy.hpp"
#include "third_party/disruptorplus/include/disruptorplus/ring_buffer.hpp"
#include "third_party/disruptorplus/include/disruptorplus/sequence_barrier.hpp"
#include "third_party/disruptorplus/include/disruptorplus/spin_wait_strategy.hpp"
#include "xenia/base/assert.h"
#include "xenia/base/atomic.h"
#include "xenia/base/console.h"
#include "xenia/base/cvar.h"
#include "xenia/base/debugging.h"
#include "xenia/base/diagnostics.h"
#include "xenia/base/filesystem.h"
#include "xenia/base/literals.h"
#include "xenia/base/math.h"
#include "xenia/base/memory.h"
#include "xenia/base/platform.h"
#include "xenia/base/ring_buffer.h"
#include "xenia/base/string.h"
#include "xenia/base/system.h"
#include "xenia/base/threading.h"

// TODO(benvanik): generic API? logging_win.cc?
#if XE_PLATFORM_ANDROID
#include <android/log.h>
#elif XE_PLATFORM_WIN32
// For MessageBox and crash sidecars:
#include "xenia/base/platform_win.h"
#include <Psapi.h>
#endif  // XE_PLATFORM

#include "third_party/fmt/include/fmt/format.h"

#if XE_PLATFORM_ANDROID
DEFINE_bool(log_to_logcat, true, "Write log output to Android Logcat.",
            "Logging");
#else
DEFINE_path(log_file, "", "Logs are written to the given file", "Logging");
DEFINE_bool(log_to_stdout, true, "Write log output to stdout", "Logging");
DEFINE_bool(log_to_debugprint, false, "Dump the log to DebugPrint.", "Logging");
#endif  // XE_PLATFORM_ANDROID
DEFINE_bool(flush_log, true, "Flush log file after each log line batch.",
            "Logging");

DEFINE_uint32(log_mask, 0,
              "Disables specific categorizes for more granular debug logging. "
              "Kernel = 1, Apu = 2, Cpu = 4, Gpu = 8. "
              "(Alias: log_disable_mask — same semantics.)",
              "Logging");

DEFINE_uint32(log_disable_mask, 0,
              "Alias for log_mask — bit mask of LogSrc channels to disable.",
              "Logging");

DEFINE_int32(
    log_level, 2,
    "Maximum level to be logged. (0=error, 1=warning, 2=info, 3=debug)",
    "Logging");

namespace dp = disruptorplus;
using namespace xe::literals;
using namespace std::chrono_literals;

namespace xe {

class Logger;

Logger* logger_ = nullptr;

std::string app_name_;
std::filesystem::path active_log_file_path_;
std::filesystem::path log_dir_;
std::string session_timestamp_;
bool file_log_sink_attached_ = false;

class BufferedLogSink final : public LogSink {
 public:
  void Write(const char* buf, size_t size) override {
    buffer_.append(buf, size);
  }
  void Flush() override {}

  const std::string& buffer() const { return buffer_; }
  void Clear() { buffer_.clear(); }

 private:
  std::string buffer_;
};

BufferedLogSink* pending_file_buffer_sink_ = nullptr;

struct LogLine {
  size_t buffer_length;
  uint32_t thread_id;
  uint32_t ts_ms_since_start;
  char thread_name[16];
  bool terminate;
  char prefix_char;
};

thread_local char thread_log_buffer_[64_KiB];

std::chrono::steady_clock::time_point log_steady_start_;

static constexpr size_t kRecentLogLineCapacity = 256;
static constexpr size_t kRecentLogLineMaxBytes = 512;
static std::array<std::array<char, kRecentLogLineMaxBytes>,
                  kRecentLogLineCapacity>
    recent_log_lines_{};
static std::atomic<size_t> recent_log_line_write_index_{0};
static std::atomic<bool> flush_requested_{false};

static uint32_t LogMsSinceStart() {
  const auto now = std::chrono::steady_clock::now();
  return static_cast<uint32_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(now -
                                                            log_steady_start_)
          .count());
}

static void FormatLogLinePrefix(const LogLine& line, char* out,
                                size_t out_size) {
  const uint32_t ms = line.ts_ms_since_start;
  const uint32_t sec_total = ms / 1000;
  const uint32_t min = (sec_total / 60) % 100;
  const uint32_t sec = sec_total % 60;
  const uint32_t frac_ms = ms % 1000;

  char name_col[16] = "              ";
  if (line.thread_name[0]) {
    const size_t name_len = strnlen(line.thread_name, sizeof(line.thread_name));
    const size_t copy_len = std::min(name_len, size_t(14));
    std::memcpy(name_col, line.thread_name, copy_len);
  }

  fmt::format_to_n(out, out_size, "{}> {:08X} [+{:02}:{:02}.{:03}] [{}] ",
                   line.prefix_char, line.thread_id, min, sec, frac_ms,
                   name_col);
}

static void RecordRecentFormattedLine(const char* prefix, size_t prefix_len,
                                      const char* body, size_t body_len) {
  const size_t slot =
      recent_log_line_write_index_.fetch_add(1, std::memory_order_relaxed) %
      kRecentLogLineCapacity;
  auto& line_buf = recent_log_lines_[slot];
  size_t written = 0;
  const size_t max_write = kRecentLogLineMaxBytes - 1;
  if (prefix_len) {
    const size_t n = std::min(prefix_len, max_write);
    std::memcpy(line_buf.data(), prefix, n);
    written = n;
  }
  if (body_len && written < max_write) {
    const size_t n = std::min(body_len, max_write - written);
    std::memcpy(line_buf.data() + written, body, n);
    written += n;
  }
  if (written < max_write) {
    line_buf[written++] = '\n';
  }
  line_buf[written] = '\0';
}

static void AppendRecentLogTail(std::string& out, size_t max_lines) {
  if (!max_lines) {
    return;
  }
  fmt::format_to(std::back_inserter(out),
                 "\n--- recent log tail ({} lines) ---\n", max_lines);

  const size_t write_idx =
      recent_log_line_write_index_.load(std::memory_order_acquire);
  const size_t available = std::min(write_idx, kRecentLogLineCapacity);
  const size_t start = write_idx >= available ? write_idx - available : 0;

  size_t emitted = 0;
  for (size_t i = start; i < write_idx && emitted < max_lines; ++i) {
    const size_t slot = i % kRecentLogLineCapacity;
    const char* line = recent_log_lines_[slot].data();
    if (line[0]) {
      out.append(line);
      if (out.empty() || out.back() != '\n') {
        out.push_back('\n');
      }
      ++emitted;
    }
  }
}

FileLogSink::~FileLogSink() {
  if (file_) {
    fflush(file_);
    if (owns_file_) {
      fclose(file_);
    }
  }
}

void FileLogSink::Write(const char* buf, size_t size) {
  if (file_) {
    fwrite(buf, 1, size, file_);
  }
}

void FileLogSink::Flush() {
  if (file_) {
    fflush(file_);
  }
}

void DebugPrintLogSink::Write(const char* buf, size_t size) {
  debugging::DebugPrint("{}", std::string_view(buf, size));
}

#if XE_PLATFORM_ANDROID
class AndroidLogSink final : public LogSink {
 public:
  explicit AndroidLogSink(const std::string_view tag) : tag_(tag) {}

  void Write(const char* buf, size_t size) override;
  void Flush() override;

 private:
  // May be called with an empty line_buffer_ to write an empty log message (if
  // the original message contains a blank line as a separator, for instance).
  void WriteLineBuffer();

  const std::string tag_;
  android_LogPriority current_priority_ = ANDROID_LOG_INFO;
  bool is_line_continuation_ = false;
  size_t line_buffer_used_ = 0;
  // LOGGER_ENTRY_MAX_PAYLOAD is defined as 4076 on older Android versions and
  // as 4068 on newer ones. An attempt to write more than that amount to the
  // kernel logger will result in a truncated log entry. 4000 is the commonly
  // used limit considered safe.
  // However, "Log message text may be truncated to less than an
  // implementation-specific limit (1023 bytes)" - android/log.h.
  char line_buffer_[1023 + 1];
};

void AndroidLogSink::Write(const char* buf, size_t size) {
  // A null character, if appears, is fine - it will just truncate a single
  // line, but since every message ends with a newline, and WriteLineBuffer is
  // done after every line, it won't have lasting effects.
  // Using memchr and memcpy as they are vectorized in Bionic.
  while (size) {
    const void* newline = std::memchr(buf, '\n', size);
    size_t line_remaining =
        newline ? size_t(static_cast<const char*>(newline) - buf) : size;
    while (line_remaining) {
      assert_true(line_buffer_used_ < xe::countof(line_buffer_));
      size_t append_count =
          std::min(line_remaining,
                   xe::countof(line_buffer_) - size_t(1) - line_buffer_used_);
      std::memcpy(line_buffer_ + line_buffer_used_, buf, append_count);
      buf += append_count;
      size -= append_count;
      line_remaining -= append_count;
      line_buffer_used_ += append_count;
      if (line_buffer_used_ >= xe::countof(line_buffer_) - size_t(1)) {
        WriteLineBuffer();
        // The next WriteLineBuffer belongs to the same line.
        is_line_continuation_ = true;
      }
    }
    if (newline) {
      // If the end of the buffer was reached right before a newline character,
      // so line_buffer_used_ is 0, and is_line_continuation_ has been set, the
      // line break has already been done by Android itself - don't write an
      // empty message. However, if the message intentionally contains blank
      // lines, write them.
      if (line_buffer_used_ || !is_line_continuation_) {
        WriteLineBuffer();
      }
      is_line_continuation_ = false;
      ++buf;
      --size;
    }
  }
}

void AndroidLogSink::Flush() {
  if (line_buffer_used_) {
    WriteLineBuffer();
  }
}

void AndroidLogSink::WriteLineBuffer() {
  // If this is a new line, check if it's a new log message, and if it is,
  // update the priority based on the prefix.
  if (!is_line_continuation_ && line_buffer_used_ >= 3 &&
      line_buffer_[1] == '>' && line_buffer_[2] == ' ') {
    switch (line_buffer_[0]) {
      case 'd':
        current_priority_ = ANDROID_LOG_DEBUG;
        break;
      case 'w':
        current_priority_ = ANDROID_LOG_WARN;
        break;
      case '!':
        current_priority_ = ANDROID_LOG_ERROR;
        break;
      case 'x':
        current_priority_ = ANDROID_LOG_FATAL;
        break;
      default:
        current_priority_ = ANDROID_LOG_INFO;
        break;
    }
  }
  // Android skips blank lines, but if writing a blank line was requested
  // explicitly for formatting, write a non-newline character.
  if (!line_buffer_used_) {
    line_buffer_[line_buffer_used_++] = ' ';
  }
  // Terminate the text.
  assert_true(line_buffer_used_ < xe::countof(line_buffer_));
  line_buffer_[line_buffer_used_] = '\0';
  // Write.
  __android_log_write(current_priority_, tag_.c_str(), line_buffer_);
  line_buffer_used_ = 0;
}
#endif  // XE_PLATFORM_ANDROID

class Logger {
 public:
  explicit Logger(const std::string_view app_name)
      : wait_strategy_(),
        claim_strategy_(kBlockCount, wait_strategy_),
        consumed_(wait_strategy_) {
    claim_strategy_.add_claim_barrier(consumed_);

    write_thread_ =
        xe::threading::Thread::Create({}, [this]() { WriteThread(); });
    assert_not_null(write_thread_);
    write_thread_->set_name("Logging Writer");
  }

  ~Logger() {
    AppendLine(0, '\0', nullptr, 0, true);  // append a terminator
    xe::threading::Wait(write_thread_.get(), true);
  }

  void AddLogSink(std::unique_ptr<LogSink>&& sink) {
    sinks_.push_back(std::move(sink));
  }

  void RemoveLogSink(const LogSink* sink) {
    sinks_.erase(std::remove_if(sinks_.begin(), sinks_.end(),
                                [sink](const std::unique_ptr<LogSink>& entry) {
                                  return entry.get() == sink;
                                }),
                 sinks_.end());
  }

  void FlushAllSinks() {
    for (const auto& sink : sinks_) {
      sink->Flush();
    }
  }

 private:
  static constexpr size_t kBufferSize = 8_MiB;
  uint8_t buffer_[kBufferSize] = {};

  static constexpr size_t kBlockSize = 256;
  static constexpr size_t kBlockCount = kBufferSize / kBlockSize;
  static constexpr size_t kBlockIndexMask = kBlockCount - 1;

  static const size_t kClaimStrategyFootprint =
      sizeof(std::atomic<dp::sequence_t>[kBlockCount]);

  static size_t BlockOffset(dp::sequence_t sequence) {
    return (sequence & kBlockIndexMask) * kBlockSize;
  }

  static size_t BlockCount(size_t byte_size) {
    return (byte_size + (kBlockSize - 1)) / kBlockSize;
  }

  dp::spin_wait_strategy wait_strategy_;
  dp::multi_threaded_claim_strategy<dp::spin_wait_strategy> claim_strategy_;
  dp::sequence_barrier<dp::spin_wait_strategy> consumed_;

  std::vector<std::unique_ptr<LogSink>> sinks_;

  std::unique_ptr<xe::threading::Thread> write_thread_;

  void Write(const char* buf, size_t size) {
    for (const auto& sink : sinks_) {
      sink->Write(buf, size);
    }
  }

  void WriteThread() {
    RingBuffer rb(buffer_, kBufferSize);

    size_t idle_loops = 0;

    dp::sequence_t next_sequence = 0;
    dp::sequence_t last_sequence = -1;

    size_t desired_count = 1;
    while (true) {
      // We want one block to find out how many blocks we need or we know how
      // many blocks needed for at least one log line.
      auto next_range = dp::sequence_range(next_sequence, desired_count);

      claim_strategy_.wait_until_published(next_range.last(), last_sequence);

      size_t read_count = 0;
      auto available_range = next_range;
      auto available_count = available_range.size();

      rb.set_write_offset(BlockOffset(available_range.end()));

      bool terminate = false;
      for (size_t i = available_range.first(); i != available_range.end();) {
        rb.set_read_offset(BlockOffset(i));

        LogLine line;
        rb.Read(&line, sizeof(line));

        auto needed_count = BlockCount(sizeof(LogLine) + line.buffer_length);
        if (read_count + needed_count > available_count) {
          // More blocks are needed for a complete line.
          desired_count = needed_count;
          break;
        } else {
          // Enough blocks to read this log line, advance by that many.
          read_count += needed_count;
          i += needed_count;

          char prefix_buf[96] = {};
          size_t prefix_len = 0;
          if (line.prefix_char) {
            FormatLogLinePrefix(line, prefix_buf, sizeof(prefix_buf));
            prefix_len = strlen(prefix_buf);
            Write(prefix_buf, prefix_len);
          }

          if (line.buffer_length) {
            // Get access to the line data - which may be split in the ring
            // buffer - and write it out in parts.
            auto line_range = rb.BeginRead(line.buffer_length);
            Write(reinterpret_cast<const char*>(line_range.first),
                  line_range.first_length);
            if (line_range.second_length) {
              Write(reinterpret_cast<const char*>(line_range.second),
                    line_range.second_length);
            }

            // Always ensure there is a newline.
            char last_char =
                line_range.second
                    ? line_range.second[line_range.second_length - 1]
                    : line_range.first[line_range.first_length - 1];
            if (last_char != '\n') {
              constexpr char suffix[1] = {'\n'};
              Write(suffix, 1);
            }

            RecordRecentFormattedLine(
                prefix_buf, prefix_len,
                reinterpret_cast<const char*>(line_range.first),
                line_range.first_length +
                    (line_range.second_length ? line_range.second_length : 0));

            rb.EndRead(std::move(line_range));
          } else {
            // Always ensure there is a newline.
            constexpr char suffix[1] = {'\n'};
            Write(suffix, 1);
          }

          if (line.terminate) {
            terminate = true;
            break;
          }
        }
      }

      if (terminate) {
        break;
      }

      if (read_count) {
        // Advance by the number of blocks we read.
        auto read_range = dp::sequence_range(next_sequence, read_count);
        next_sequence = read_range.end();
        last_sequence = read_range.last();
        consumed_.publish(last_sequence);

        desired_count = 1;

        if (cvars::flush_log ||
            flush_requested_.exchange(false, std::memory_order_acq_rel)) {
          FlushAllSinks();
        }

        idle_loops = 0;
      } else {
        if (flush_requested_.load(std::memory_order_acquire)) {
          FlushAllSinks();
          flush_requested_.store(false, std::memory_order_release);
          idle_loops = 0;
        } else if (idle_loops >= 1000) {
          // Introduce a waiting period.
          xe::threading::Sleep(std::chrono::milliseconds(50));
        } else {
          idle_loops++;
        }
      }
    }
  }

 public:
  void AppendLine(uint32_t thread_id, const char prefix_char,
                  const char* buffer_data, size_t buffer_length,
                  bool terminate = false) {
    size_t count = BlockCount(sizeof(LogLine) + buffer_length);

    auto range = claim_strategy_.claim(count);
    assert_true(range.size() == count);

    RingBuffer rb(buffer_, kBufferSize);
    rb.set_write_offset(BlockOffset(range.first()));
    rb.set_read_offset(BlockOffset(range.end()));

    LogLine line = {};
    line.buffer_length = buffer_length;
    line.thread_id = thread_id;
    line.ts_ms_since_start = LogMsSinceStart();
    const auto thread_name = xe::threading::current_thread_name();
    if (!thread_name.empty()) {
      const size_t name_len =
          std::min(thread_name.size(), sizeof(line.thread_name) - 1);
      std::memcpy(line.thread_name, thread_name.data(), name_len);
      line.thread_name[name_len] = '\0';
    }
    line.prefix_char = prefix_char;
    line.terminate = terminate;

    rb.Write(&line, sizeof(LogLine));
    if (buffer_length) {
      rb.Write(buffer_data, buffer_length);
    }

    claim_strategy_.publish(range);
  }
};

void InitializeLogging(const std::string_view app_name) {
  log_steady_start_ = std::chrono::steady_clock::now();
  InitializeDiagnostics();

  app_name_ = std::string(app_name);
  auto mem = memory::AlignedAlloc<Logger>(0x10);
  logger_ = new (mem) Logger(app_name);

#if XE_PLATFORM_ANDROID
  // TODO(Triang3l): Enable file logging, but not by default as logs may be
  // huge.
  if (cvars::log_to_logcat) {
    logger_->AddLogSink(std::make_unique<AndroidLogSink>(app_name));
  }
#else
  // Buffer early log lines until AttachFileLogSink knows storage_root/log/.
  auto buffer_sink = std::make_unique<BufferedLogSink>();
  pending_file_buffer_sink_ = buffer_sink.get();
  logger_->AddLogSink(std::move(buffer_sink));

  if (cvars::log_to_stdout) {
    logger_->AddLogSink(std::make_unique<FileLogSink>(stdout, false));
  }

  if (cvars::log_to_debugprint) {
    logger_->AddLogSink(std::make_unique<DebugPrintLogSink>());
  }

  // Explicit log_file cvar: attach immediately (legacy behaviour).
  if (!cvars::log_file.empty()) {
    xe::filesystem::CreateParentFolder(cvars::log_file);
    FILE* log_file = xe::filesystem::OpenFile(cvars::log_file, "wt");
    if (log_file) {
      active_log_file_path_ = cvars::log_file;
      log_dir_ = cvars::log_file.parent_path();
      if (pending_file_buffer_sink_) {
        std::string early = pending_file_buffer_sink_->buffer();
        if (!early.empty()) {
          fwrite(early.data(), 1, early.size(), log_file);
        }
        logger_->RemoveLogSink(pending_file_buffer_sink_);
        pending_file_buffer_sink_ = nullptr;
      }
      logger_->AddLogSink(std::make_unique<FileLogSink>(log_file, true));
      file_log_sink_attached_ = true;
    }
  }
#endif  // XE_PLATFORM_ANDROID
}

void AttachFileLogSink(const std::filesystem::path& log_dir,
                       const std::string_view app_name) {
#if XE_PLATFORM_ANDROID
  (void)log_dir;
  (void)app_name;
  return;
#else
  if (file_log_sink_attached_ || !logger_) {
    return;
  }

  app_name_ = std::string(app_name);
  log_dir_ = log_dir;

  std::filesystem::path resolved_log_dir = log_dir;
  auto ec = xe::filesystem::CreateFolder(resolved_log_dir);
  if (ec) {
    XELOGW(
        "AttachFileLogSink: failed to create {} ({}), falling back to exe dir",
        resolved_log_dir, ec.message());
    resolved_log_dir = xe::filesystem::GetExecutableFolder() / "log";
    xe::filesystem::CreateFolder(resolved_log_dir);
  }
  log_dir_ = resolved_log_dir;

  session_timestamp_ =
      fmt::format("{:%Y%m%d-%H%M%S}", std::chrono::system_clock::now());

  std::filesystem::path log_file_path =
      log_dir_ / fmt::format("{}_{}.log", app_name_, session_timestamp_);

  FILE* log_file = xe::filesystem::OpenFile(log_file_path, "wt");
  if (!log_file) {
    XELOGW("AttachFileLogSink: failed to open {}", log_file_path);
    return;
  }

  active_log_file_path_ = log_file_path;

  if (pending_file_buffer_sink_) {
    const std::string& early = pending_file_buffer_sink_->buffer();
    if (!early.empty()) {
      fwrite(early.data(), 1, early.size(), log_file);
    }
    logger_->RemoveLogSink(pending_file_buffer_sink_);
    pending_file_buffer_sink_ = nullptr;
  }

  logger_->AddLogSink(std::make_unique<FileLogSink>(log_file, true));
  file_log_sink_attached_ = true;

  // Pointer file for the current session log.
  std::filesystem::path latest_pointer = log_dir_ / "xenia_latest.log";
  if (FILE* pointer_file = xe::filesystem::OpenFile(latest_pointer, "wt")) {
    fmt::print(pointer_file, "{}\n", active_log_file_path_.string());
    fclose(pointer_file);
  }

  XELOGI("Log file: {}", active_log_file_path_);
#endif  // XE_PLATFORM_ANDROID
}

std::filesystem::path GetActiveLogFilePath() { return active_log_file_path_; }

std::filesystem::path GetLogDirectory() {
  if (!log_dir_.empty()) {
    return log_dir_;
  }
  if (!active_log_file_path_.empty()) {
    return active_log_file_path_.parent_path();
  }
  return {};
}

#if XE_PLATFORM_WIN32
static void AppendWin32ExceptionContext(std::string& out,
                                        const _EXCEPTION_POINTERS* info) {
  if (!info || !info->ExceptionRecord) {
    return;
  }

  const auto* record = info->ExceptionRecord;
  const auto* context = info->ContextRecord;
  fmt::format_to(std::back_inserter(out),
                 "\n--- exception context ---\nExceptionCode: 0x{:08X}\n",
                 record->ExceptionCode);
  if (record->ExceptionAddress) {
    fmt::format_to(std::back_inserter(out), "ExceptionAddress: {:p}\n",
                   record->ExceptionAddress);
  }

  if (context) {
#if XE_ARCH_AMD64
    fmt::format_to(std::back_inserter(out),
                   "RIP: 0x{:016X}\nRSP: 0x{:016X}\nRBP: 0x{:016X}\n",
                   context->Rip, context->Rsp, context->Rbp);
#elif XE_ARCH_ARM64
    fmt::format_to(std::back_inserter(out),
                   "PC: 0x{:016X}\nSP: 0x{:016X}\nFP: 0x{:016X}\n", context->Pc,
                   context->Sp, context->Fp);
#endif
  }

  HMODULE modules[256];
  DWORD needed = 0;
  if (EnumProcessModules(GetCurrentProcess(), modules,
                         static_cast<DWORD>(sizeof(modules)), &needed)) {
    out.append("\n--- loaded modules ---\n");
    const unsigned module_count =
        std::min<unsigned>(needed / sizeof(HMODULE), xe::countof(modules));
    for (unsigned i = 0; i < module_count; ++i) {
      char module_name[MAX_PATH + 1] = {};
      if (GetModuleFileNameA(modules[i], module_name, sizeof(module_name))) {
        out.append(module_name);
        out.push_back('\n');
      }
    }
  }
}
#endif  // XE_PLATFORM_WIN32

void logging::WriteCrashSidecar(const char* category,
                                const std::string_view body,
                                const void* exception_pointers) {
  if (log_dir_.empty() || session_timestamp_.empty()) {
    return;
  }

  std::filesystem::path sidecar_path =
      log_dir_ /
      fmt::format("{}_{}_{}.txt", app_name_, session_timestamp_, category);

  std::string sidecar_body(body);
#if XE_PLATFORM_WIN32
  AppendWin32ExceptionContext(
      sidecar_body,
      static_cast<const _EXCEPTION_POINTERS*>(exception_pointers));
#endif

  if (diagnostics().capture_log_tail_on_crash.load(std::memory_order_relaxed)) {
    AppendRecentLogTail(sidecar_body, diagnostics().crash_log_tail_lines.load(
                                          std::memory_order_relaxed));
  }

  if (FILE* sidecar_file = xe::filesystem::OpenFile(sidecar_path, "wt")) {
    fwrite(sidecar_body.data(), 1, sidecar_body.size(), sidecar_file);
    fclose(sidecar_file);
  }
}

void ShutdownLogging() {
  Logger* logger = logger_;
  logger_ = nullptr;

  logger->~Logger();
  memory::AlignedFree(logger);
}

void FlushLog() {
  if (!logger_) {
    return;
  }

  xe::threading::Sleep(10ms);
  logger_->FlushAllSinks();
}

static int g_saved_loglevel = static_cast<int>(LogLevel::Disabled);
void logging::ToggleLogLevel() {
  auto swap = g_saved_loglevel;

  g_saved_loglevel = cvars::log_level;
  cvars::log_level = swap;
}

uint32_t EffectiveLogDisableMask() {
  return cvars::log_disable_mask ? cvars::log_disable_mask : cvars::log_mask;
}

bool logging::ShouldLog(LogLevel log_level, uint32_t log_mask) {
  return static_cast<int32_t>(log_level) <= cvars::log_level &&
         (log_mask & EffectiveLogDisableMask()) == 0;
}

uint32_t logging::internal::GetLogLevel() { return cvars::log_level; }

std::pair<char*, size_t> logging::internal::GetThreadBuffer() {
  return {thread_log_buffer_, sizeof(thread_log_buffer_)};
}
XE_NOALIAS
void logging::internal::AppendLogLine(LogLevel log_level,
                                      const char prefix_char, size_t written) {
  if (!logger_ || !ShouldLog(log_level) || !written) {
    return;
  }
  logger_->AppendLine(xe::threading::current_thread_id(), prefix_char,
                      thread_log_buffer_, written);

  if (static_cast<int32_t>(log_level) <=
          static_cast<int32_t>(LogLevel::Warning) &&
      diagnostics().force_flush_on_error.load(std::memory_order_relaxed)) {
    flush_requested_.store(true, std::memory_order_release);
  }
}

void logging::AppendLogLine(LogLevel log_level, const char prefix_char,
                            const std::string_view str, uint32_t log_mask) {
  if (!ShouldLog(log_level, log_mask) || !str.size()) {
    return;
  }
  logger_->AppendLine(xe::threading::current_thread_id(), prefix_char,
                      str.data(), str.size());
}

void FatalError(const std::string_view str) {
  logging::AppendLogLine(LogLevel::Error, 'x', str);
  logging::WriteCrashSidecar("fatal", str);
  FlushLog();

  if (!xe::has_console_attached()) {
    ShowSimpleMessageBox(SimpleMessageBoxType::Error, str);
  }

  ShutdownLogging();

#if XE_PLATFORM_ANDROID
  // Throw an error that can be reported to the developers via the store.
  std::abort();
#else
  // skip static destructors so they can't race with worker threads still
  // running and corrupt the heap, at_quick_exit handlers will take care
  // of necessary cleanup (e.g. /dev/shm/xenia* files on linux )
  std::quick_exit(EXIT_FAILURE);
#endif  // XE_PLATFORM_ANDROID
}

}  // namespace xe
