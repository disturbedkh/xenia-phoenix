/**
 ******************************************************************************
 * Observability presets.
 ******************************************************************************
 */

#include "xenia/base/obs/obs.h"

#include "xenia/base/cvar.h"
#include "xenia/base/logging.h"
#include "xenia/base/obs/obs_channel.h"
#include "xenia/base/obs/obs_internal.h"
#include "xenia/base/obs/obs_invariant.h"
#include "xenia/base/string_util.h"

DEFINE_string(log_preset, "play",
              "Observability preset: play|support|homebrew|develop|forensic",
              "Logging");

namespace xe {
namespace obs {
namespace {

std::filesystem::path StubsPathForTitle(uint32_t title_id) {
  if (!title_id) {
    return {};
  }
  return std::filesystem::path("telemetry") /
         (string_util::to_hex_string(title_id) + "_stubs.jsonl");
}

std::filesystem::path EventsPathForTitle(uint32_t title_id) {
  if (!title_id) {
    return {};
  }
  return std::filesystem::path("telemetry") /
         (string_util::to_hex_string(title_id) + "_events.jsonl");
}

std::filesystem::path GuestLogForTitle(uint32_t title_id) {
  if (!title_id) {
    return std::filesystem::path("guest.log");
  }
  return std::filesystem::path("telemetry") /
         (string_util::to_hex_string(title_id) + "_guest.log");
}

void ApplyChannelDefaults(Preset preset) {
  SetChannelLevel(ChannelId::kGuestPrint, LogLevel::Info);
  SetChannelLevel(ChannelId::kGpuEdram, LogLevel::Info);
  SetChannelLevel(ChannelId::kKernelStub, LogLevel::Info);
  switch (preset) {
    case Preset::kDevelop:
    case Preset::kForensic:
      SetChannelLevel(ChannelId::kGuestPrint, LogLevel::Debug);
      SetChannelLevel(ChannelId::kGpuEdram, LogLevel::Debug);
      SetChannelLevel(ChannelId::kKernelExport, LogLevel::Debug);
      break;
    case Preset::kHomebrew:
      SetChannelLevel(ChannelId::kGuestPrint, LogLevel::Info);
      break;
    default:
      break;
  }
}

}  // namespace

Preset ParsePreset(std::string_view name) {
  if (name == "play") {
    return Preset::kPlay;
  }
  if (name == "support") {
    return Preset::kSupport;
  }
  if (name == "homebrew") {
    return Preset::kHomebrew;
  }
  if (name == "develop") {
    return Preset::kDevelop;
  }
  if (name == "forensic") {
    return Preset::kForensic;
  }
  return Preset::kUnknown;
}

std::string_view PresetName(Preset preset) {
  switch (preset) {
    case Preset::kSupport:
      return "support";
    case Preset::kHomebrew:
      return "homebrew";
    case Preset::kDevelop:
      return "develop";
    case Preset::kForensic:
      return "forensic";
    case Preset::kPlay:
    default:
      return "play";
  }
}

bool ApplyPreset(std::string_view name) {
  const Preset preset = ParsePreset(name);
  if (preset == Preset::kUnknown) {
    return false;
  }
  SetRuntimeState(SessionIdStorage(), TitleIdStorage(), preset);
  ConfigureInvariantPreset(preset);

  int32_t log_level = 2;
  bool stub_jsonl = false;
  bool guest_debug = false;
  bool jsonl = false;
  bool probe_port = false;
  std::filesystem::path events_path;
  std::filesystem::path guest_path;
  std::filesystem::path stub_path;

  const uint32_t tid = TitleIdStorage();
  switch (preset) {
    case Preset::kPlay:
      log_level = 1;
      break;
    case Preset::kSupport:
      log_level = 2;
      stub_jsonl = true;
      jsonl = true;
      events_path = EventsPathForTitle(tid);
      stub_path = StubsPathForTitle(tid);
      break;
    case Preset::kHomebrew:
      log_level = 2;
      stub_jsonl = true;
      guest_debug = true;
      jsonl = true;
      events_path = EventsPathForTitle(tid);
      guest_path = GuestLogForTitle(tid);
      stub_path = StubsPathForTitle(tid);
      break;
    case Preset::kDevelop:
      log_level = 3;
      stub_jsonl = true;
      guest_debug = true;
      jsonl = true;
      probe_port = true;
      events_path = EventsPathForTitle(tid);
      guest_path = GuestLogForTitle(tid);
      stub_path = StubsPathForTitle(tid);
      break;
    case Preset::kForensic:
      log_level = 3;
      stub_jsonl = true;
      guest_debug = true;
      jsonl = true;
      probe_port = true;
      events_path = EventsPathForTitle(tid);
      guest_path = GuestLogForTitle(tid);
      stub_path = StubsPathForTitle(tid);
      break;
    default:
      break;
  }

  SetCvarInt32("log_level", log_level);
  SetCvarUint32("log_mask", 0);
  SetCvarUint32("log_disable_mask", 0);
  if (stub_jsonl && !stub_path.empty()) {
    SetCvarPath("kernel_stub_hit_log", stub_path);
  } else {
    SetCvarPath("kernel_stub_hit_log", {});
  }
  if (jsonl && !events_path.empty()) {
    SetCvarPath("obs_events_log", events_path);
  } else {
    SetCvarPath("obs_events_log", {});
  }
  SetCvarBool("debugprint_trap_log", guest_debug);
  if (probe_port) {
    SetCvarUint32("phoenix_debug_port", 8765);
  }
  ConfigureEventSinks(jsonl, events_path, guest_path);
  ApplyChannelDefaults(preset);
  ApplyChannelOverridesFromCvars();
  return true;
}

void ApplyPresetFromCvar() { ApplyPreset(cvars::log_preset); }

void ApplyChannelOverrides() { ApplyChannelOverridesFromCvars(); }

}  // namespace obs
}  // namespace xe
