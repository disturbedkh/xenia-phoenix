/**
 ******************************************************************************
 * Observability channels.
 ******************************************************************************
 */

#include "xenia/base/obs/obs_channel.h"

#include <mutex>
#include <unordered_map>

#include "xenia/base/cvar.h"

DECLARE_string(log_channel_overrides);
#include "xenia/base/logging.h"
#include "xenia/base/string_util.h"

namespace xe {
namespace obs {
namespace {

constexpr ChannelMeta kChannels[] = {
    {ChannelId::kGuestPrint, "Guest.Print", "Guest", LogLevel::Info,
     LogSrc::Cpu, 'P'},
    {ChannelId::kKernelStub, "Kernel.Stub", "Kernel", LogLevel::Info,
     LogSrc::Kernel, 'K'},
    {ChannelId::kKernelExport, "Kernel.Export", "Kernel", LogLevel::Debug,
     LogSrc::Kernel, 'K'},
    {ChannelId::kGpuEdram, "Gpu.Edram", "Gpu", LogLevel::Debug, LogSrc::Gpu,
     'G'},
    {ChannelId::kGpuPipeline, "Gpu.Pipeline", "Gpu", LogLevel::Debug,
     LogSrc::Gpu, 'G'},
    {ChannelId::kApuXma, "Apu.Xma", "Apu", LogLevel::Info, LogSrc::Apu, 'A'},
    {ChannelId::kHidDevice, "Hid.Device", "Hid", LogLevel::Info,
     LogSrc::Uncategorized, 'H'},
    {ChannelId::kBaseLifecycle, "Base.Lifecycle", "Base", LogLevel::Info,
     LogSrc::Uncategorized, 'i'},
};

std::mutex g_channel_mutex;
LogLevel g_channel_levels[static_cast<size_t>(ChannelId::kCount)] = {};
bool g_channel_levels_set[static_cast<size_t>(ChannelId::kCount)] = {};

LogLevel ParseLevel(std::string_view s) {
  if (s == "error") {
    return LogLevel::Error;
  }
  if (s == "warn" || s == "warning") {
    return LogLevel::Warning;
  }
  if (s == "info") {
    return LogLevel::Info;
  }
  if (s == "debug") {
    return LogLevel::Debug;
  }
  if (s == "trace") {
    return LogLevel::Trace;
  }
  return LogLevel::Disabled;
}

}  // namespace

const ChannelMeta& ChannelMetaFor(ChannelId id) {
  const size_t idx = static_cast<size_t>(id);
  if (idx < static_cast<size_t>(ChannelId::kCount)) {
    return kChannels[idx];
  }
  return kChannels[0];
}

const char* ChannelName(ChannelId id) { return ChannelMetaFor(id).name; }

ChannelGroup ChannelBudgetGroup(ChannelId id) {
  switch (id) {
    case ChannelId::kGpuEdram:
    case ChannelId::kGpuPipeline:
      return ChannelGroup::kPerFrame;
    case ChannelId::kApuXma:
    case ChannelId::kKernelStub:
    case ChannelId::kKernelExport:
      return ChannelGroup::kPerSecond;
    default:
      return ChannelGroup::kPerEvent;
  }
}

LogLevel ChannelLevel(ChannelId id) {
  const size_t idx = static_cast<size_t>(id);
  if (idx < static_cast<size_t>(ChannelId::kCount) &&
      g_channel_levels_set[idx]) {
    return g_channel_levels[idx];
  }
  return ChannelMetaFor(id).default_level;
}

void SetChannelLevel(ChannelId id, LogLevel level) {
  const size_t idx = static_cast<size_t>(id);
  if (idx >= static_cast<size_t>(ChannelId::kCount)) {
    return;
  }
  std::lock_guard<std::mutex> lock(g_channel_mutex);
  g_channel_levels[idx] = level;
  g_channel_levels_set[idx] = true;
}

bool ShouldLogChannel(ChannelId id, LogLevel level) {
  const auto& meta = ChannelMetaFor(id);
  const LogLevel max_level = ChannelLevel(id);
  if (static_cast<int>(level) > static_cast<int>(max_level)) {
    return false;
  }
  return logging::ShouldLog(level, meta.log_src_mask);
}

void ApplyChannelOverridesFromCvars() {
  const std::string& spec = cvars::log_channel_overrides;
  if (spec.empty()) {
    return;
  }
  size_t pos = 0;
  while (pos < spec.size()) {
    size_t comma = spec.find(',', pos);
    const std::string_view pair(spec.data() + pos, comma == std::string::npos
                                                       ? spec.size() - pos
                                                       : comma - pos);
    const size_t eq = pair.find('=');
    if (eq != std::string_view::npos) {
      const std::string_view ch_name(pair.data(), eq);
      const std::string_view level_name(pair.data() + eq + 1,
                                        pair.size() - eq - 1);
      const LogLevel level = ParseLevel(level_name);
      if (level != LogLevel::Disabled) {
        for (const auto& ch : kChannels) {
          if (ch_name == ch.name) {
            SetChannelLevel(ch.id, level);
            break;
          }
        }
      }
    }
    if (comma == std::string::npos) {
      break;
    }
    pos = comma + 1;
  }
}

}  // namespace obs
}  // namespace xe
