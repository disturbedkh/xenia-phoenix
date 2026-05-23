/**
 ******************************************************************************
 * Structured JSONL events + aggregator.
 ******************************************************************************
 */

#include <chrono>
#include <mutex>
#include <unordered_map>

#include "third_party/fmt/include/fmt/format.h"
#include "xenia/base/cvar.h"
#include "xenia/base/obs/obs.h"
#include "xenia/base/obs/obs_internal.h"
#include "xenia/base/string_util.h"

#if XE_PLATFORM_ANDROID
#include <android/log.h>
#endif

DEFINE_path(obs_events_log, "",
            "Unified observability JSONL event log (empty = disabled).",
            "Logging");
DEFINE_path(obs_guest_log, "",
            "Guest.Print strings (homebrew / forensic presets).", "Logging");
DEFINE_bool(obs_write_summary_on_shutdown, false,
            "Write telemetry/{title}_obs_summary.json on shutdown.", "Logging");

namespace xe {
namespace obs {
namespace {

std::mutex g_event_mutex;
bool g_jsonl_enabled = false;
JsonlSink g_events_sink;
JsonlSink g_guest_sink;

struct AggKey {
  std::string code;
  std::string channel;
  bool operator==(const AggKey& o) const {
    return code == o.code && channel == o.channel;
  }
};

struct AggKeyHash {
  size_t operator()(const AggKey& k) const {
    return std::hash<std::string>()(k.code) ^
           (std::hash<std::string>()(k.channel) << 1);
  }
};

struct AggState {
  uint64_t count = 0;
  std::string last_detail;
};

std::unordered_map<AggKey, AggState, AggKeyHash> g_aggregator;
constexpr uint64_t kAggEmitEvery = 64;

const char* SeverityName(EventSeverity s) {
  switch (s) {
    case EventSeverity::kError:
      return "error";
    case EventSeverity::kWarn:
      return "warn";
    case EventSeverity::kInfo:
      return "info";
    default:
      return "debug";
  }
}

const char* KindName(EventKind k) {
  switch (k) {
    case EventKind::kStub:
      return "Stub";
    case EventKind::kInvariant:
      return "Invariant";
    case EventKind::kLog:
      return "Log";
    default:
      return "Event";
  }
}

uint64_t NowMs() {
  return static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
}

bool ShouldAggregate(const Event& e) {
  return e.kind == EventKind::kProbe || e.kind == EventKind::kInvariant;
}

void WriteEventLine(const Event& event, std::string_view detail) {
  std::string title_field = "null";
  const uint32_t tid = TitleIdStorage();
  if (tid) {
    title_field = fmt::format("\"{}\"", string_util::to_hex_string(tid));
  }

  const uint32_t frame = event.frame ? event.frame : CurrentFrameStorage();

  std::string line = fmt::format(
      "{{\"v\":1,\"seq\":{},\"ts_ms\":{},\"session\":\"{}\",\"title_id\":{},"
      "\"origin\":\"host\",\"domain\":\"{}\",\"channel\":\"{}\","
      "\"kind\":\"{}\",\"severity\":\"{}\",\"code\":\"{}\",\"detail\":\"{}\"",
      NextEventSeq(), NowMs(), SessionIdStorage(), title_field,
      JsonEscape(event.domain), JsonEscape(event.channel), KindName(event.kind),
      SeverityName(event.severity), JsonEscape(event.code), JsonEscape(detail));

  if (frame) {
    line += fmt::format(",\"frame\":{}", frame);
  }
  if (event.guest_lr) {
    line += fmt::format(",\"guest_lr\":\"{}\"",
                        string_util::to_hex_string(event.guest_lr));
  } else {
    line += ",\"guest_lr\":null";
  }
  line += "}\n";
  g_events_sink.WriteLine(line);

#if XE_PLATFORM_ANDROID
  const Preset preset = CurrentPresetStorage();
  if ((preset == Preset::kPlay || preset == Preset::kSupport) &&
      (event.severity == EventSeverity::kWarn ||
       event.severity == EventSeverity::kError)) {
    android_LogPriority pri = ANDROID_LOG_INFO;
    if (event.severity == EventSeverity::kWarn) {
      pri = ANDROID_LOG_WARN;
    } else if (event.severity == EventSeverity::kError) {
      pri = ANDROID_LOG_ERROR;
    }
    const char* tag = "XeniaObs";
    if (event.domain == "Gpu") {
      tag = "XeniaGpu";
    } else if (event.domain == "Apu") {
      tag = "XeniaApu";
    } else if (event.domain == "Kernel") {
      tag = "XeniaKernel";
    }
    __android_log_print(pri, tag, "%.*s: %.*s",
                        static_cast<int>(event.code.size()), event.code.data(),
                        static_cast<int>(detail.size()), detail.data());
  }
#endif
}

}  // namespace

JsonlSink& EventsSink() { return g_events_sink; }
JsonlSink& GuestSink() { return g_guest_sink; }

void ConfigureEventSinks(bool jsonl, const std::filesystem::path& events_path,
                         const std::filesystem::path& guest_path) {
  std::lock_guard<std::mutex> lock(g_event_mutex);
  g_jsonl_enabled = jsonl;
  g_events_sink.Configure(events_path, jsonl);
  g_guest_sink.Configure(guest_path, !guest_path.empty());
  SetCvarPath("obs_events_log", events_path);
  SetCvarPath("obs_guest_log", guest_path);
}

bool JsonlEnabled() { return g_jsonl_enabled; }

std::filesystem::path EventsLogPath() { return g_events_sink.path(); }

std::filesystem::path GuestLogPath() { return g_guest_sink.path(); }

bool EventsEnabled() { return g_jsonl_enabled; }

void EmitEvent(const Event& event) {
  if (!g_jsonl_enabled) {
    return;
  }

  std::string detail;
  bool emit = true;
  if (ShouldAggregate(event)) {
    AggKey key{std::string(event.code), std::string(event.channel)};
    std::lock_guard<std::mutex> lock(g_event_mutex);
    auto& st = g_aggregator[key];
    ++st.count;
    st.last_detail = std::string(event.detail);
    emit = (st.count == 1 || (st.count % kAggEmitEvery) == 0);
    if (emit && st.count > 1) {
      detail = fmt::format("{} (count={})", st.last_detail, st.count);
    } else {
      detail = st.last_detail;
    }
  } else {
    detail = std::string(event.detail);
  }
  if (!emit) {
    return;
  }
  WriteEventLine(event, detail);
}

void EmitGuestPrint(std::string_view text, uint32_t guest_lr) {
  Event ev;
  ev.kind = EventKind::kLog;
  ev.severity = EventSeverity::kInfo;
  ev.domain = "Guest";
  ev.channel = "Guest.Print";
  ev.code = "GuestPrint";
  ev.detail = text;
  ev.guest_lr = guest_lr;
  EmitEvent(ev);

  if (guest_lr) {
    g_guest_sink.WriteLine(fmt::format(
        "[lr={}] {}\n", string_util::to_hex_string(guest_lr), text));
  } else {
    g_guest_sink.WriteLine(fmt::format("{}\n", text));
  }
}

void BridgeProbeEvent(std::string_view probe_kind, std::string_view detail) {
  Event ev;
  ev.kind = EventKind::kProbe;
  ev.severity = EventSeverity::kInfo;
  ev.domain = "Host";
  ev.channel = "Probe";
  ev.code = probe_kind;
  ev.detail = detail;

  if (probe_kind == "upload_range_error" ||
      probe_kind == "host_depth_transfer_mismatch" ||
      probe_kind == "host_depth_store" ||
      probe_kind == "depth_host_sidecar_stale") {
    ev.domain = "Gpu";
    ev.channel = "Gpu.Edram";
    ev.severity = EventSeverity::kWarn;
  } else if (probe_kind == "ownership_change" ||
             probe_kind == "edram_transfer") {
    ev.domain = "Gpu";
    ev.channel = "Gpu.Edram";
  } else if (probe_kind == "xma_divergence") {
    ev.domain = "Apu";
    ev.channel = "Apu.Xma";
    ev.severity = EventSeverity::kWarn;
  } else if (probe_kind == "stub_hit") {
    ev.domain = "Kernel";
    ev.channel = "Kernel.Stub";
  }

  EmitEvent(ev);
}

void FlushAggregatorSummaries() {
  if (!g_jsonl_enabled) {
    return;
  }
  std::vector<AggKey> keys;
  {
    std::lock_guard<std::mutex> lock(g_event_mutex);
    keys.reserve(g_aggregator.size());
    for (const auto& [key, st] : g_aggregator) {
      if (st.count > 1) {
        keys.push_back(key);
      }
    }
  }
  for (const auto& key : keys) {
    AggState st;
    {
      std::lock_guard<std::mutex> lock(g_event_mutex);
      auto it = g_aggregator.find(key);
      if (it == g_aggregator.end()) {
        continue;
      }
      st = it->second;
    }
    Event ev;
    ev.kind = EventKind::kProbe;
    ev.severity = EventSeverity::kInfo;
    ev.domain = "Host";
    ev.channel = key.channel;
    ev.code = key.code;
    ev.detail =
        fmt::format("summary count={} last={}", st.count, st.last_detail);
    WriteEventLine(ev, ev.detail);
  }
  g_events_sink.Flush();
}

std::vector<AggregatorEntry> GetAggregatorSnapshot() {
  std::vector<AggregatorEntry> out;
  std::lock_guard<std::mutex> lock(g_event_mutex);
  out.reserve(g_aggregator.size());
  for (const auto& [key, st] : g_aggregator) {
    AggregatorEntry e;
    e.code = key.code;
    e.channel = key.channel;
    e.count = st.count;
    e.last_detail = st.last_detail;
    e.last_ts_ms = NowMs();
    out.push_back(std::move(e));
  }
  return out;
}

}  // namespace obs
}  // namespace xe
