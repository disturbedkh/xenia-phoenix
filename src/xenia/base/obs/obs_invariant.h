/**
 ******************************************************************************
 * Observability invariants — named failure-mode breadcrumbs.
 ******************************************************************************
 */
#pragma once

#include <functional>
#include <string_view>

#include "third_party/fmt/include/fmt/format.h"
#include "xenia/base/obs/obs.h"
#include "xenia/base/obs/obs_channel.h"

#define OBS_INVARIANT(code, channel_id, cond, ...)                          \
  do {                                                                       \
    if (cond) {                                                              \
      xe::obs::InvariantLazy(                                                \
          code, channel_id, true,                                            \
          [&]() { return fmt::format(__VA_ARGS__); });                       \
    }                                                                        \
  } while (0)

namespace xe {
namespace obs {

void ResetInvariantFrameBudget();

void Invariant(std::string_view code, ChannelId channel, bool violated,
               std::string_view detail = {});

void InvariantLazy(std::string_view code, ChannelId channel, bool violated,
                   std::function<std::string()> detail_fn);

void ConfigureInvariantPreset(Preset preset);

void InvariantCountOnly(std::string_view code);

uint64_t InvariantCounter(std::string_view code);

}  // namespace obs
}  // namespace xe
