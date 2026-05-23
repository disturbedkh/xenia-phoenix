#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "xenia/base/obs/obs.h"
#include "xenia/base/obs/obs_jsonl_sink.h"

namespace xe {
namespace obs {

// Central runtime state (obs.cc).
void SetRuntimeState(std::string_view session_id, uint32_t title_id,
                     Preset preset);
std::string_view SessionIdStorage();
uint32_t TitleIdStorage();
Preset CurrentPresetStorage();
uint32_t CurrentFrameStorage();
uint64_t NextEventSeq();

void ConfigureEventSinks(bool jsonl, const std::filesystem::path& events_path,
                         const std::filesystem::path& guest_path);

JsonlSink& EventsSink();
JsonlSink& GuestSink();

void FlushAggregatorSummaries();
std::vector<AggregatorEntry> GetAggregatorSnapshot();

void ApplyChannelOverridesFromCvars();

bool SetCvarInt32(const char* name, int32_t value);
bool SetCvarUint32(const char* name, uint32_t value);
bool SetCvarBool(const char* name, bool value);
bool SetCvarPath(const char* name, const std::filesystem::path& value);
std::string CvarRuntimeString(const char* name);

}  // namespace obs
}  // namespace xe
