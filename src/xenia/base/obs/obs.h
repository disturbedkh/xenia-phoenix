/**
 ******************************************************************************
 * Observability v2 hub — presets, channels, structured events, invariants.
 ******************************************************************************
 */
#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace xe {
namespace obs {

enum class Preset : uint8_t {
  kPlay = 0,
  kSupport,
  kHomebrew,
  kDevelop,
  kForensic,
  kUnknown,
};

enum class EventKind : uint8_t {
  kLog = 0,
  kStub,
  kInvariant,
  kProbe,
};

enum class EventSeverity : uint8_t {
  kError = 0,
  kWarn,
  kInfo,
  kDebug,
};

struct Event {
  EventKind kind = EventKind::kProbe;
  EventSeverity severity = EventSeverity::kInfo;
  std::string_view domain;
  std::string_view channel;
  std::string_view code;
  std::string_view detail;
  uint32_t frame = 0;
  uint32_t guest_lr = 0;
};

struct AggregatorEntry {
  std::string code;
  std::string channel;
  uint64_t count = 0;
  uint64_t last_ts_ms = 0;
  std::string last_detail;
};

void Init();
void Shutdown();

Preset ParsePreset(std::string_view name);
std::string_view PresetName(Preset preset);
Preset CurrentPreset();

bool ApplyPreset(std::string_view name);
void ApplyPresetFromCvar();
void ApplyChannelOverrides();

void SetTitleId(uint32_t title_id);
uint32_t TitleId();
std::string_view SessionId();

void OnFrameBegin(uint32_t frame_id = 0);
void RegisterGuestDebugRing(uint32_t guest_ptr);
void TryDrainGuestDebugRing();
void LogObservabilityBlock();

bool EventsEnabled();
bool JsonlEnabled();
std::filesystem::path EventsLogPath();
std::filesystem::path GuestLogPath();

void EmitEvent(const Event& event);
void EmitGuestPrint(std::string_view text, uint32_t guest_lr = 0);
void BridgeProbeEvent(std::string_view probe_kind, std::string_view detail);

void AppendConfigDump(std::string& out);
void WriteObsSummaryIfConfigured();
void WriteForensicBundleOnShutdown();

std::vector<AggregatorEntry> GetAggregatorSnapshot();
uint64_t CurrentEventSeq();
uint32_t CurrentFrame();

}  // namespace obs
}  // namespace xe
