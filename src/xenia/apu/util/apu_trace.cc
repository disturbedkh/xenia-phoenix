/**
 ******************************************************************************
 * APU runtime telemetry (Phase 1.4).
 ******************************************************************************
 */

#include "xenia/apu/util/apu_trace.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <vector>

#include "third_party/crypto/sha256.h"
#include "xenia/apu/apu_flags.h"
#include "xenia/base/filesystem.h"
#include "xenia/base/obs/obs.h"
#include "xenia/base/obs/obs_invariant.h"
#include "xenia/base/string_util.h"
#include "xenia/debug/phoenix_probe.h"

namespace xe {
namespace apu {
namespace {

std::atomic<uint32_t> g_telemetry_title_id{0};

std::mutex g_apu_trace_mutex;

void AppendJsonl(const std::string& path, const std::string& line) {
  if (path.empty()) {
    return;
  }
  std::lock_guard<std::mutex> lock(g_apu_trace_mutex);
  FILE* f = filesystem::OpenFile(path, "ab");
  if (!f) {
    return;
  }
  fwrite(line.data(), 1, line.size(), f);
  fclose(f);
}

}  // namespace

void SetTelemetryTitleId(uint32_t title_id) {
  g_telemetry_title_id.store(title_id, std::memory_order_relaxed);
  debug::PhoenixProbeSetTitleId(title_id);
}

uint32_t GetTelemetryTitleId() {
  return g_telemetry_title_id.load(std::memory_order_relaxed);
}

void LogXmaDivergence(uint32_t context_id, std::string_view site,
                      std::string_view detail, uint32_t title_id, uint32_t lr) {
  const auto& path = cvars::apu_xma_divergence_log;
  if (path.empty()) {
    return;
  }

  std::string line;
  line.reserve(256);
  line.append("{\"context_id\":");
  line.append(std::to_string(context_id));
  line.append(",\"site\":\"");
  for (char c : site) {
    if (c == '\\' || c == '"') {
      line.push_back('\\');
    }
    line.push_back(c);
  }
  line.append("\"");
  if (!detail.empty()) {
    line.append(",\"detail\":\"");
    for (char c : detail) {
      if (c == '\\' || c == '"') {
        line.push_back('\\');
      }
      line.push_back(c);
    }
    line.append("\"");
  }
  if (title_id) {
    line.append(",\"title_id\":\"");
    line.append(string_util::to_hex_string(title_id));
    line.append("\"");
  }
  if (lr) {
    line.append(",\"lr\":\"");
    line.append(string_util::to_hex_string(lr));
    line.append("\"");
  }
  line.append("}\n");
  AppendJsonl(path.string(), line);
  debug::PhoenixProbeNotifyXmaDivergence();
  std::string obs_detail = std::string(site);
  if (!detail.empty()) {
    obs_detail.append(":");
    obs_detail.append(detail);
  }
  obs::Invariant("XmaDivergence", obs::ChannelId::kApuXma, true, obs_detail);
}

void LogPcmHashWindow(uint32_t title_id, uint64_t window_start_ms,
                      std::string_view sha256_hex, uint32_t sample_rate_hz,
                      uint32_t channels) {
  const auto& path = cvars::apu_pcm_hash_log;
  if (path.empty()) {
    return;
  }

  std::string line;
  line.reserve(192);
  line.append("{\"title_id\":\"");
  line.append(string_util::to_hex_string(title_id));
  line.append("\",\"window_start_ms\":");
  line.append(std::to_string(window_start_ms));
  line.append(",\"sha256_pcm_mix\":\"");
  line.append(sha256_hex);
  line.append("\",\"sample_rate\":");
  line.append(std::to_string(sample_rate_hz));
  line.append(",\"channels\":");
  line.append(std::to_string(channels));
  line.append("}\n");
  AppendJsonl(path.string(), line);
  debug::PhoenixProbeNotifyPcmHash(sha256_hex);
}

void OnSubmitFramePcm(const float* frame, uint32_t channel_count,
                      uint32_t samples_per_channel, uint32_t sample_rate_hz,
                      uint32_t title_id) {
  if (cvars::apu_pcm_hash_log.empty() || !frame || !samples_per_channel) {
    return;
  }

  static std::mutex pcm_mutex;
  static uint64_t window_start_ms = 0;
  static std::vector<int16_t> accum;

  const uint64_t now_ms = static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now().time_since_epoch())
          .count());

  std::lock_guard<std::mutex> lock(pcm_mutex);
  if (!window_start_ms) {
    window_start_ms = now_ms;
  }

  const size_t prev = accum.size();
  accum.resize(prev + samples_per_channel * 2);
  for (uint32_t i = 0; i < samples_per_channel; ++i) {
    float l = 0.f;
    float r = 0.f;
    if (channel_count >= 1) {
      l = frame[i];
    }
    if (channel_count >= 2) {
      r = frame[samples_per_channel + i];
    } else {
      r = l;
    }
    const auto clamp = [](float v) -> int16_t {
      v = std::max(-1.f, std::min(1.f, v));
      return static_cast<int16_t>(v * 32767.f);
    };
    accum[prev + i * 2] = clamp(l);
    accum[prev + i * 2 + 1] = clamp(r);
  }

  const uint32_t interval = std::max(1u, cvars::apu_pcm_hash_interval_ms);
  if (now_ms - window_start_ms < interval) {
    return;
  }

  sha256::SHA256 sha;
  sha.add(accum.data(), accum.size() * sizeof(int16_t));
  const std::string hex = sha.getHash();

  LogPcmHashWindow(title_id, window_start_ms, hex, sample_rate_hz,
                   channel_count >= 2 ? 2u : 1u);
  accum.clear();
  window_start_ms = now_ms;
}

}  // namespace apu
}  // namespace xe
