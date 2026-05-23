/**
 ******************************************************************************
 * Observability hub — init, config dump, cvar helpers, central runtime state.
 ******************************************************************************
 */

#include "xenia/base/obs/obs.h"

#include <atomic>
#include <fstream>
#include <mutex>
#include <random>
#include <vector>

#if XE_PLATFORM_WIN32
#include <stdlib.h>
#endif

#include "third_party/fmt/include/fmt/format.h"
#include "xenia/base/cvar.h"
#include "xenia/base/filesystem.h"
#include "xenia/base/logging.h"
#include "xenia/base/obs/obs_channel.h"
#include "xenia/base/obs/obs_internal.h"
#include "xenia/base/obs/obs_invariant.h"
#include "xenia/base/string_util.h"

DEFINE_string(log_channel_overrides, "",
              "Per-channel levels: Gpu.Edram=debug,Kernel.Stub=info",
              "Logging");

DECLARE_bool(obs_write_summary_on_shutdown);

namespace xe {
namespace obs {
namespace {

std::mutex g_state_mutex;
std::string g_session_id;
uint32_t g_title_id = 0;
Preset g_preset = Preset::kPlay;
uint32_t g_frame_id = 0;
std::atomic<uint64_t> g_event_seq{0};

std::string MakeSessionId() {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFFu);
  return fmt::format("{:08x}{:08x}", dist(gen), dist(gen));
}

template <typename T>
bool SetCvarValue(const char* name, const T& value) {
  if (!cvar::ConfigVars) {
    return false;
  }
  auto it = cvar::ConfigVars->find(name);
  if (it == cvar::ConfigVars->end() || !it->second) {
    return false;
  }
  if (auto* cv = dynamic_cast<cvar::ConfigVar<T>*>(it->second)) {
    cv->SetConfigValue(value);
    return true;
  }
  return false;
}

}  // namespace

void SetRuntimeState(std::string_view session_id, uint32_t title_id,
                     Preset preset) {
  std::lock_guard<std::mutex> lock(g_state_mutex);
  g_session_id = std::string(session_id);
  g_title_id = title_id;
  g_preset = preset;
}

std::string_view SessionIdStorage() { return g_session_id; }
uint32_t TitleIdStorage() { return g_title_id; }
Preset CurrentPresetStorage() { return g_preset; }
uint32_t CurrentFrameStorage() { return g_frame_id; }
uint64_t NextEventSeq() { return g_event_seq.fetch_add(1) + 1; }

uint64_t CurrentEventSeq() {
  return g_event_seq.load(std::memory_order_relaxed);
}

uint32_t CurrentFrame() { return CurrentFrameStorage(); }

bool SetCvarInt32(const char* name, int32_t value) {
  return SetCvarValue(name, value);
}

bool SetCvarUint32(const char* name, uint32_t value) {
  return SetCvarValue(name, value);
}

bool SetCvarBool(const char* name, bool value) {
  return SetCvarValue(name, value);
}

bool SetCvarPath(const char* name, const std::filesystem::path& value) {
  return SetCvarValue(name, value);
}

std::string CvarRuntimeString(const char* name) {
  if (!cvar::ConfigVars) {
    return {};
  }
  auto it = cvar::ConfigVars->find(name);
  if (it == cvar::ConfigVars->end() || !it->second) {
    return {};
  }
  if (auto* cv = dynamic_cast<cvar::ConfigVar<std::string>*>(it->second)) {
    return *cv->current_value();
  }
  if (auto* cv =
          dynamic_cast<cvar::ConfigVar<std::filesystem::path>*>(it->second)) {
    return path_to_utf8(*cv->current_value());
  }
  return it->second->config_value();
}

void Init() {
  SetRuntimeState(MakeSessionId(), 0, Preset::kPlay);
  ConfigureInvariantPreset(Preset::kPlay);
}

void Shutdown() {
  FlushAggregatorSummaries();
  EventsSink().Flush();
  EventsSink().Close();
  GuestSink().Close();
  WriteForensicBundleOnShutdown();
  WriteObsSummaryIfConfigured();
}

void SetTitleId(uint32_t title_id) {
  {
    std::lock_guard<std::mutex> lock(g_state_mutex);
    g_title_id = title_id;
  }
  if (CurrentPreset() != Preset::kPlay) {
    ApplyPreset(PresetName(CurrentPreset()));
  }
}

uint32_t TitleId() { return TitleIdStorage(); }

std::string_view SessionId() { return SessionIdStorage(); }

Preset CurrentPreset() { return CurrentPresetStorage(); }

void OnFrameBegin(uint32_t frame_id) {
  g_frame_id = frame_id;
  ResetInvariantFrameBudget();
  TryDrainGuestDebugRing();
}

void LogObservabilityBlock() {
  std::string block;
  AppendConfigDump(block);
  XELOGI("{}", block);
}

void AppendConfigDump(std::string& out) {
  out.append("\n----------- OBSERVABILITY -----------\n");
  out.append(fmt::format("obs_preset = {}\n", PresetName(CurrentPreset())));
  out.append(fmt::format("obs_session_id = {}\n", g_session_id));
  if (TitleIdStorage()) {
    out.append(fmt::format("obs_title_id = {}\n",
                           string_util::to_hex_string(TitleIdStorage())));
  }
  out.append(fmt::format("log_level = {}\n", CvarRuntimeString("log_level")));
  out.append(fmt::format("log_mask = {}\n", CvarRuntimeString("log_mask")));
  out.append(fmt::format("log_disable_mask = {}\n",
                         CvarRuntimeString("log_disable_mask")));
  out.append(
      fmt::format("obs_events_log = {}\n", path_to_utf8(EventsLogPath())));
  out.append(fmt::format("obs_guest_log = {}\n", path_to_utf8(GuestLogPath())));
  out.append(fmt::format("kernel_stub_hit_log = {}\n",
                         CvarRuntimeString("kernel_stub_hit_log")));
  out.append(fmt::format("phoenix_debug_port = {}\n",
                         CvarRuntimeString("phoenix_debug_port")));
  out.append(fmt::format("obs_events_enabled = {}\n",
                         EventsEnabled() ? "true" : "false"));
  out.append("channels:");
  for (size_t i = 0; i < static_cast<size_t>(ChannelId::kCount); ++i) {
    const auto id = static_cast<ChannelId>(i);
    const auto& meta = ChannelMetaFor(id);
    out.append(fmt::format("\n  {} = {}", meta.name,
                           static_cast<int>(ChannelLevel(id))));
  }
  out.append("\n----------- END OBSERVABILITY -----");
}

void WriteObsSummaryIfConfigured() {
  if (!cvars::obs_write_summary_on_shutdown || !TitleIdStorage()) {
    return;
  }
  const auto path =
      std::filesystem::path("telemetry") /
      (string_util::to_hex_string(TitleIdStorage()) + "_obs_summary.json");
  std::filesystem::create_directories(path.parent_path());
  std::ofstream out(path);
  if (!out) {
    return;
  }
  out << "{\n";
  out << fmt::format("  \"session\": \"{}\",\n", g_session_id);
  out << fmt::format("  \"preset\": \"{}\",\n", PresetName(CurrentPreset()));
  out << fmt::format("  \"events_log\": \"{}\",\n",
                     path_to_utf8(EventsLogPath()));
  out << "  \"note\": \"Run phoenixctl log summarize for full "
         "classification\"\n";
  out << "}\n";
}

void WriteForensicBundleOnShutdown() {
  if (CurrentPreset() != Preset::kForensic || !TitleIdStorage()) {
    return;
  }
  const auto tid_hex = string_util::to_hex_string(TitleIdStorage());
  const auto bundle_dir =
      std::filesystem::path("telemetry") / (tid_hex + "_crash_bundle");
  std::filesystem::create_directories(bundle_dir);

  const auto events = EventsLogPath();
  if (!events.empty() && std::filesystem::exists(events)) {
    std::error_code ec;
    std::filesystem::copy_file(
        events, bundle_dir / "events_tail.jsonl",
        std::filesystem::copy_options::overwrite_existing, ec);
    std::ifstream ev_in(events, std::ios::binary);
    std::vector<char> tail;
    if (ev_in) {
      ev_in.seekg(0, std::ios::end);
      const auto sz = ev_in.tellg();
      const auto take = std::min<std::streamoff>(sz, 256 * 1024);
      ev_in.seekg(sz - take);
      tail.resize(static_cast<size_t>(take));
      ev_in.read(tail.data(), take);
    }
    std::ofstream ev_tail(bundle_dir / "events_last256k.jsonl",
                          std::ios::binary);
    if (ev_tail && !tail.empty()) {
      ev_tail.write(tail.data(), static_cast<std::streamsize>(tail.size()));
    }
  }

  const std::string log_path = CvarRuntimeString("log_file");
  if (!log_path.empty() && std::filesystem::exists(log_path)) {
    std::ifstream log_in(log_path, std::ios::binary);
    std::vector<char> tail;
    if (log_in) {
      log_in.seekg(0, std::ios::end);
      const auto sz = log_in.tellg();
      const auto take = std::min<std::streamoff>(sz, 64 * 1024);
      log_in.seekg(sz - take);
      tail.resize(static_cast<size_t>(take));
      log_in.read(tail.data(), take);
    }
    std::ofstream log_tail(bundle_dir / "crash_log_tail.txt", std::ios::binary);
    if (log_tail && !tail.empty()) {
      log_tail.write(tail.data(), static_cast<std::streamsize>(tail.size()));
    }
  }

  {
    std::ofstream agg(bundle_dir / "obs_aggregator.json");
    if (agg) {
      agg << "[\n";
      bool first = true;
      for (const auto& e : GetAggregatorSnapshot()) {
        if (!first) {
          agg << ",\n";
        }
        first = false;
        agg << fmt::format(
            "  {{\"code\":\"{}\",\"channel\":\"{}\",\"count\":{},"
            "\"last_detail\":\"{}\"}}",
            e.code, e.channel, e.count, e.last_detail);
      }
      agg << "\n]\n";
    }
  }

  const auto cfg_tail = bundle_dir / "observability.txt";
  std::ofstream cfg(cfg_tail);
  if (cfg) {
    std::string block;
    AppendConfigDump(block);
    cfg << block;
  }

#if XE_PLATFORM_WIN32
  const auto zip_path =
      std::filesystem::path("telemetry") / (tid_hex + "_crash_bundle.zip");
  std::wstring ps =
      L"powershell -NoProfile -Command \"Compress-Archive -Path '";
  ps += bundle_dir.wstring();
  ps += L'\\';
  ps += L"*' -DestinationPath '";
  ps += zip_path.wstring();
  ps += L"' -Force\"";
  _wsystem(ps.c_str());
#endif
}

}  // namespace obs
}  // namespace xe
