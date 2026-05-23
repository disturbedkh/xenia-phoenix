/**
 ******************************************************************************
 * Observability channels — per-subsystem log routing.
 ******************************************************************************
 */
#pragma once

#include <cstdint>

#include "xenia/base/logging.h"

namespace xe {
namespace obs {

enum class ChannelGroup : uint8_t {
  kPerFrame = 0,
  kPerSecond,
  kPerEvent,
};

enum class ChannelId : uint16_t {
  kGuestPrint = 0,
  kKernelStub,
  kKernelExport,
  kGpuEdram,
  kGpuPipeline,
  kApuXma,
  kHidDevice,
  kBaseLifecycle,
  kCount,
};

struct ChannelMeta {
  ChannelId id;
  const char* name;
  const char* domain;
  LogLevel default_level;
  uint32_t log_src_mask;
  char prefix_char;
};

const ChannelMeta& ChannelMetaFor(ChannelId id);
ChannelGroup ChannelBudgetGroup(ChannelId id);
const char* ChannelName(ChannelId id);

LogLevel ChannelLevel(ChannelId id);
void SetChannelLevel(ChannelId id, LogLevel level);
bool ShouldLogChannel(ChannelId id, LogLevel level);

}  // namespace obs
}  // namespace xe

#define XELOGGPU_PIPELINE(fmt, ...) \
  XE_LOG_CHAN(xe::obs::ChannelId::kGpuPipeline, Debug, fmt, ##__VA_ARGS__)
#define XELOGGPU_EDRAM(fmt, ...) \
  XE_LOG_CHAN(xe::obs::ChannelId::kGpuEdram, Debug, fmt, ##__VA_ARGS__)
#define XELOGKERNEL_STUB(fmt, ...) \
  XE_LOG_CHAN(xe::obs::ChannelId::kKernelStub, Info, fmt, ##__VA_ARGS__)
#define XELOGKERNEL_EXPORT(fmt, ...) \
  XE_LOG_CHAN(xe::obs::ChannelId::kKernelExport, Debug, fmt, ##__VA_ARGS__)
#define XELOGAPU_XMA(fmt, ...) \
  XE_LOG_CHAN(xe::obs::ChannelId::kApuXma, Info, fmt, ##__VA_ARGS__)
#define XELOGHID(fmt, ...) \
  XE_LOG_CHAN(xe::obs::ChannelId::kHidDevice, Info, fmt, ##__VA_ARGS__)
#define XELOG_GUEST(fmt, ...) \
  XE_LOG_CHAN(xe::obs::ChannelId::kGuestPrint, Info, fmt, ##__VA_ARGS__)
#define XELOG_LIFECYCLE(fmt, ...) \
  XE_LOG_CHAN(xe::obs::ChannelId::kBaseLifecycle, Info, fmt, ##__VA_ARGS__)

#define XE_LOG_CHAN(channel_id, level, fmt, ...)                      \
  do {                                                                \
    if (xe::obs::ShouldLogChannel(channel_id, xe::LogLevel::level)) { \
      const auto& _xe_obs_ch = xe::obs::ChannelMetaFor(channel_id);   \
      xe::logging::AppendLogLineFormat(                               \
          _xe_obs_ch.log_src_mask, xe::LogLevel::level,               \
          _xe_obs_ch.prefix_char, fmt, ##__VA_ARGS__);                \
    }                                                                 \
  } while (0)
