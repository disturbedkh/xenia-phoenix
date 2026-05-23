/**
 ******************************************************************************
 * Observability invariants.
 ******************************************************************************
 */

#include "xenia/base/obs/obs_invariant.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <unordered_map>

#include "xenia/base/debugging.h"
#include "xenia/base/logging.h"
#include "xenia/base/obs/obs.h"
#include "xenia/base/obs/obs_internal.h"

#if XE_DEBUG
DEFINE_bool(break_on_invariant, false,
            "Break into debugger when an invariant is violated.", "Logging");
#endif

namespace xe {
namespace obs {
namespace {

std::mutex g_inv_mutex;
std::unordered_map<std::string, std::atomic<uint64_t>> g_inv_counters;
Preset g_inv_preset = Preset::kPlay;

thread_local uint32_t g_inv_budget_per_frame = 8;
thread_local uint32_t g_inv_budget_per_second = 32;
thread_local uint64_t g_inv_budget_second = 0;

EventSeverity DefaultSeverity(Preset preset) {
  if (preset == Preset::kForensic) {
    return EventSeverity::kWarn;
  }
  if (preset == Preset::kDevelop) {
    return EventSeverity::kWarn;
  }
  return EventSeverity::kInfo;
}

bool ConsumeInvariantBudget(ChannelId channel) {
  switch (ChannelBudgetGroup(channel)) {
    case ChannelGroup::kPerFrame:
      if (!g_inv_budget_per_frame) {
        return false;
      }
      --g_inv_budget_per_frame;
      return true;
    case ChannelGroup::kPerSecond:
      if (!g_inv_budget_per_second) {
        return false;
      }
      --g_inv_budget_per_second;
      return true;
    default:
      return true;
  }
}

void InvariantImpl(std::string_view code, ChannelId channel, bool violated,
                   std::string_view detail) {
  if (!violated) {
    return;
  }
  InvariantCountOnly(code);

  const Preset preset = g_inv_preset;
  if (preset == Preset::kPlay || preset == Preset::kSupport) {
    return;
  }

  const bool emit_event = preset == Preset::kForensic ||
                          preset == Preset::kDevelop ||
                          preset == Preset::kHomebrew;
  if (!emit_event) {
    return;
  }

  Event ev;
  ev.kind = EventKind::kInvariant;
  ev.severity = DefaultSeverity(preset);
  const auto& meta = ChannelMetaFor(channel);
  ev.domain = meta.domain;
  ev.channel = meta.name;
  ev.code = code;
  ev.detail = detail;
  EmitEvent(ev);

  if (preset == Preset::kDevelop && ConsumeInvariantBudget(channel)) {
    XE_LOG_CHAN(channel, Warning, "invariant {}: {}", code, detail);
  }

#if XE_DEBUG
  if (cvars::break_on_invariant) {
    debugging::Break();
  }
#endif
}

}  // namespace

void ConfigureInvariantPreset(Preset preset) { g_inv_preset = preset; }

void ResetInvariantFrameBudget() {
  g_inv_budget_per_frame = 8;
  const uint64_t sec = static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::steady_clock::now().time_since_epoch())
          .count());
  if (sec != g_inv_budget_second) {
    g_inv_budget_second = sec;
    g_inv_budget_per_second = 32;
  }
}

void InvariantCountOnly(std::string_view code) {
  std::lock_guard<std::mutex> lock(g_inv_mutex);
  g_inv_counters[std::string(code)].fetch_add(1, std::memory_order_relaxed);
}

uint64_t InvariantCounter(std::string_view code) {
  std::lock_guard<std::mutex> lock(g_inv_mutex);
  auto it = g_inv_counters.find(std::string(code));
  if (it == g_inv_counters.end()) {
    return 0;
  }
  return it->second.load(std::memory_order_relaxed);
}

void Invariant(std::string_view code, ChannelId channel, bool violated,
               std::string_view detail) {
  InvariantImpl(code, channel, violated, detail);
}

void InvariantLazy(std::string_view code, ChannelId channel, bool violated,
                   std::function<std::string()> detail_fn) {
  if (!violated) {
    return;
  }
  const Preset preset = g_inv_preset;
  if (preset == Preset::kPlay || preset == Preset::kSupport) {
    InvariantCountOnly(code);
    return;
  }
  std::string detail = detail_fn ? detail_fn() : std::string();
  InvariantImpl(code, channel, true, detail);
}

}  // namespace obs
}  // namespace xe
