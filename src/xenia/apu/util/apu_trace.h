/**
 ******************************************************************************
 * APU runtime telemetry (Phase 1.4).
 ******************************************************************************
 */

#pragma once

#include <cstdint>
#include <string_view>

namespace xe {
namespace apu {

// Updated by AudioSystem::SubmitFrame from the current executable title.
void SetTelemetryTitleId(uint32_t title_id);
uint32_t GetTelemetryTitleId();

void LogXmaDivergence(uint32_t context_id, std::string_view site,
                      std::string_view detail, uint32_t title_id = 0,
                      uint32_t lr = 0);

void LogPcmHashWindow(uint32_t title_id, uint64_t window_start_ms,
                      std::string_view sha256_hex, uint32_t sample_rate_hz,
                      uint32_t channels);

// Called from audio drivers on each submitted frame (may rate-limit internally).
void OnSubmitFramePcm(const float* frame, uint32_t channel_count,
                      uint32_t samples_per_channel, uint32_t sample_rate_hz,
                      uint32_t title_id);

}  // namespace apu
}  // namespace xe
