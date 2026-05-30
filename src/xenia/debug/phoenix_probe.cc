/**
 ******************************************************************************
 * Phoenix localhost debug probe — minimal HTTP/1.1 on 127.0.0.1 (Windows v1).
 ******************************************************************************
 */

#include "xenia/debug/phoenix_probe.h"

#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "xenia/base/cvar.h"
#include "xenia/base/filesystem.h"
#include "xenia/base/logging.h"
#include "xenia/base/obs/obs.h"
#include "xenia/base/obs/obs_invariant.h"
#include "xenia/base/platform.h"
#include "xenia/base/string_util.h"
#include "xenia/debug/phoenix_flags.h"

#if XE_PLATFORM_WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include "xenia/base/platform_win.h"
#pragma comment(lib, "ws2_32.lib")
#endif

namespace xe {
namespace debug {
namespace {

std::mutex g_mutex;
std::atomic<bool> g_started{false};
std::atomic<bool> g_stop{false};
std::thread g_server_thread;

std::chrono::steady_clock::time_point g_start_time;
uint32_t g_title_id = 0;
uint64_t g_stub_hit_count = 0;
std::string g_last_stub_module;
std::string g_last_stub_export;
std::string g_last_pcm_hash;
uint64_t g_xma_divergence_count = 0;

std::atomic<uint64_t> g_gpu_upload_range_error_count{0};
std::atomic<uint64_t> g_gpu_pipeline_skip_count{0};
std::atomic<uint64_t> g_gpu_present_count{0};
std::atomic<uint64_t> g_gpu_ownership_change_count{0};
std::atomic<uint64_t> g_gpu_edram_transfer_count{0};
std::atomic<uint64_t> g_gpu_host_depth_store_count{0};
std::atomic<uint64_t> g_gpu_host_depth_transfer_mismatch_count{0};

struct ProbeEvent {
  uint64_t seq = 0;
  uint64_t ts_ms = 0;
  PhoenixProbeEventKind kind = PhoenixProbeEventKind::kStubHit;
  std::string detail;
};

constexpr size_t kEventRingCapacity = 256;
ProbeEvent g_event_ring[kEventRingCapacity];
std::atomic<uint64_t> g_event_seq{0};

NetplayJsonProvider g_netplay_status_provider;
NetplayJsonProvider g_netplay_sessions_provider;
std::mutex g_netplay_provider_mutex;

std::string JsonEscape(std::string_view s) {
  std::string out;
  out.reserve(s.size() + 8);
  for (char c : s) {
    if (c == '\\' || c == '"') {
      out.push_back('\\');
    }
    out.push_back(c);
  }
  return out;
}

uint64_t UptimeMs() {
  auto now = std::chrono::steady_clock::now();
  return static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(now - g_start_time)
          .count());
}

void PushEvent(PhoenixProbeEventKind kind, std::string_view detail) {
  const uint64_t seq = g_event_seq.fetch_add(1, std::memory_order_relaxed) + 1;
  ProbeEvent ev;
  ev.seq = seq;
  ev.ts_ms = UptimeMs();
  ev.kind = kind;
  ev.detail = std::string(detail);
  std::lock_guard<std::mutex> lock(g_mutex);
  g_event_ring[seq % kEventRingCapacity] = std::move(ev);
}

const char* EventKindName(PhoenixProbeEventKind kind) {
  switch (kind) {
    case PhoenixProbeEventKind::kStubHit:
      return "stub_hit";
    case PhoenixProbeEventKind::kUploadRangeError:
      return "upload_range_error";
    case PhoenixProbeEventKind::kVfsResolveFail:
      return "vfs_resolve_fail";
    case PhoenixProbeEventKind::kPipelineSkip:
      return "pipeline_skip";
    case PhoenixProbeEventKind::kOwnershipChange:
      return "ownership_change";
    case PhoenixProbeEventKind::kHostDepthStore:
      return "host_depth_store";
    default:
      return "unknown";
  }
}

std::string CvarValueOrEmpty(std::string_view name) {
  if (!::cvar::ConfigVars) {
    return {};
  }
  auto it = ::cvar::ConfigVars->find(std::string(name));
  if (it == ::cvar::ConfigVars->end() || !it->second) {
    return {};
  }
  // Runtime value (CLI / game config), not on-disk TOML (config_value()).
  if (auto* cv = dynamic_cast<::cvar::ConfigVar<std::string>*>(it->second)) {
    return *cv->current_value();
  }
  if (auto* cv =
          dynamic_cast<::cvar::ConfigVar<std::filesystem::path>*>(it->second)) {
    return xe::path_to_utf8(*cv->current_value());
  }
  return it->second->config_value();
}

}  // namespace

std::string PhoenixProbeBuildHealthJson() {
  return "{\"ok\":true,\"version\":\"3\",\"obs_schema\":1}";
}

std::string PhoenixProbeBuildStatusJson() {
  auto snap = PhoenixProbeGetSnapshot();
  std::ostringstream os;
  os << "{";
  if (snap.title_id) {
    os << "\"title_id\":\"" << string_util::to_hex_string(snap.title_id)
       << "\",";
  }
  os << "\"uptime_ms\":" << snap.uptime_ms;
  os << ",\"stub_hit_count\":" << snap.stub_hit_count;
  if (!snap.last_stub_module.empty()) {
    os << ",\"last_stub_module\":\"" << JsonEscape(snap.last_stub_module)
       << "\"";
    os << ",\"last_stub_export\":\"" << JsonEscape(snap.last_stub_export)
       << "\"";
  }
  if (!snap.last_pcm_hash.empty()) {
    os << ",\"last_pcm_hash\":\"" << JsonEscape(snap.last_pcm_hash) << "\"";
  }
  os << ",\"xma_divergence_count\":" << snap.xma_divergence_count;
  os << "}";
  return os.str();
}

std::string PhoenixProbeBuildSnapshotJson() {
  auto snap = PhoenixProbeGetSnapshot();
  const std::string readback = CvarValueOrEmpty("readback_resolve");
  const std::string hid = CvarValueOrEmpty("hid");
  const std::string trace_gpu = CvarValueOrEmpty("trace_gpu_prefix");
  const std::string stub_log = CvarValueOrEmpty("kernel_stub_hit_log");
  const std::string pcm_log = CvarValueOrEmpty("apu_pcm_hash_log");
  const std::string obs_events = CvarValueOrEmpty("obs_events_log");
  const std::string log_preset = CvarValueOrEmpty("log_preset");

  std::ostringstream os;
  os << "{";
  if (snap.title_id) {
    os << "\"title_id\":\"" << string_util::to_hex_string(snap.title_id)
       << "\",";
  }
  os << "\"uptime_ms\":" << snap.uptime_ms;
  os << ",\"stub_hit_count\":" << snap.stub_hit_count;
  if (!snap.last_stub_module.empty()) {
    os << ",\"last_stub_module\":\"" << JsonEscape(snap.last_stub_module)
       << "\"";
    os << ",\"last_stub_export\":\"" << JsonEscape(snap.last_stub_export)
       << "\"";
  }
  if (!snap.last_pcm_hash.empty()) {
    os << ",\"last_pcm_hash\":\"" << JsonEscape(snap.last_pcm_hash) << "\"";
  }
  os << ",\"xma_divergence_count\":" << snap.xma_divergence_count;
  os << ",\"gpu\":{";
  os << "\"upload_range_error_count\":" << snap.gpu_upload_range_error_count;
  os << ",\"pipeline_skip_count\":" << snap.gpu_pipeline_skip_count;
  os << ",\"present_count\":" << snap.gpu_present_count;
  os << ",\"ownership_change_count\":" << snap.gpu_ownership_change_count;
  os << ",\"edram_transfer_count\":" << snap.gpu_edram_transfer_count;
  os << ",\"host_depth_store_count\":" << snap.gpu_host_depth_store_count;
  os << ",\"host_depth_transfer_mismatch_count\":"
     << snap.gpu_host_depth_transfer_mismatch_count;
  os << ",\"obs_depth_host_sidecar_stale_count\":"
     << snap.obs_depth_host_sidecar_stale_count;
  os << ",\"obs_host_depth_transfer_mismatch_count\":"
     << snap.obs_host_depth_transfer_mismatch_count;
  os << ",\"obs_gpu_upload_range_error_count\":"
     << snap.obs_gpu_upload_range_error_count;
  os << ",\"readback_resolve\":\"" << JsonEscape(readback) << "\"";
  os << "}";
  os << ",\"launch\":{";
  os << "\"hid\":\"" << JsonEscape(hid) << "\"";
  os << ",\"trace_gpu_prefix\":\"" << JsonEscape(trace_gpu) << "\"";
  os << ",\"kernel_stub_hit_log\":\"" << JsonEscape(stub_log) << "\"";
  os << ",\"apu_pcm_hash_log\":\"" << JsonEscape(pcm_log) << "\"";
  os << ",\"obs_events_log\":\"" << JsonEscape(obs_events) << "\"";
  os << ",\"log_preset\":\"" << JsonEscape(log_preset) << "\"";
  os << "}";
  os << ",\"obs_aggregator\":[";
  {
    bool first_agg = true;
    for (const auto& e : obs::GetAggregatorSnapshot()) {
      if (!first_agg) {
        os << ',';
      }
      first_agg = false;
      os << "{\"code\":\"" << JsonEscape(e.code) << "\"";
      os << ",\"channel\":\"" << JsonEscape(e.channel) << "\"";
      os << ",\"count\":" << e.count;
      os << ",\"last_ts_ms\":" << e.last_ts_ms;
      os << "}";
    }
  }
  os << "]";
  os << ",\"event_seq\":" << g_event_seq.load(std::memory_order_relaxed);
  os << "}";
  return os.str();
}

std::string PhoenixProbeBuildCvarsJson(std::string_view query) {
  std::ostringstream os;
  os << "{\"cvars\":{";
  bool first = true;
  if (::cvar::ConfigVars) {
    size_t pos = 0;
    while (pos < query.size()) {
      size_t end = query.find(',', pos);
      std::string name(query.substr(
          pos, end == std::string::npos ? std::string::npos : end - pos));
      while (!name.empty() && name[0] == ' ') {
        name.erase(name.begin());
      }
      if (!name.empty()) {
        auto it = ::cvar::ConfigVars->find(name);
        if (it != ::cvar::ConfigVars->end() && it->second) {
          if (!first) {
            os << ',';
          }
          first = false;
          os << "\"" << JsonEscape(name) << "\":\""
             << JsonEscape(it->second->config_value()) << "\"";
        }
      }
      if (end == std::string::npos) {
        break;
      }
      pos = end + 1;
    }
  }
  os << "}}";
  return os.str();
}

std::string PhoenixProbeBuildEventsJson(uint64_t since_seq) {
  std::ostringstream os;
  os << "{\"events\":[";
  bool first = true;
  const uint64_t latest = g_event_seq.load(std::memory_order_relaxed);
  std::lock_guard<std::mutex> lock(g_mutex);
  for (uint64_t s = since_seq + 1; s <= latest; ++s) {
    const ProbeEvent& ev = g_event_ring[s % kEventRingCapacity];
    if (ev.seq != s) {
      continue;
    }
    if (!first) {
      os << ',';
    }
    first = false;
    os << "{\"seq\":" << ev.seq;
    os << ",\"ts_ms\":" << ev.ts_ms;
    os << ",\"kind\":\"" << EventKindName(ev.kind) << "\"";
    os << ",\"detail\":\"" << JsonEscape(ev.detail) << "\"}";
  }
  os << "],\"latest_seq\":" << latest << "}";
  return os.str();
}

#if XE_PLATFORM_WIN32
namespace {

std::string HttpResponse(int code, std::string_view status,
                         std::string_view body) {
  std::ostringstream os;
  os << "HTTP/1.1 " << code << ' ' << status << "\r\n";
  os << "Content-Type: application/json\r\n";
  os << "Connection: close\r\n";
  os << "Content-Length: " << body.size() << "\r\n\r\n";
  os << body;
  return os.str();
}

void HandleClient(SOCKET client) {
  char buf[4096];
  int n = recv(client, buf, sizeof(buf) - 1, 0);
  if (n <= 0) {
    closesocket(client);
    return;
  }
  buf[n] = '\0';
  std::string req(buf, static_cast<size_t>(n));

  std::string path;
  if (req.rfind("GET ", 0) == 0) {
    constexpr size_t kAfterGet = 3;
    size_t sp2 = req.find(' ', kAfterGet + 1);
    if (sp2 != std::string::npos && sp2 > kAfterGet + 1) {
      path = req.substr(kAfterGet + 1, sp2 - kAfterGet - 1);
    }
  }
  if (path.rfind("http://", 0) == 0 || path.rfind("https://", 0) == 0) {
    auto scheme_end = path.find("://");
    auto path_start = path.find('/', scheme_end + 3);
    if (path_start != std::string::npos) {
      path = path.substr(path_start);
    } else {
      path = "/";
    }
  }

  std::string body;
  int code = 404;
  const char* status = "Not Found";

  if (path == "/health" || path.rfind("/health?", 0) == 0) {
    body = PhoenixProbeBuildHealthJson();
    code = 200;
    status = "OK";
  } else if (path == "/status" || path.rfind("/status?", 0) == 0) {
    body = PhoenixProbeBuildStatusJson();
    code = 200;
    status = "OK";
  } else if (path == "/snapshot" || path.rfind("/snapshot?", 0) == 0) {
    body = PhoenixProbeBuildSnapshotJson();
    code = 200;
    status = "OK";
  } else if (path == "/netplay/status" ||
             path.rfind("/netplay/status?", 0) == 0) {
    {
      std::lock_guard<std::mutex> lock(g_netplay_provider_mutex);
      if (g_netplay_status_provider) {
        body = g_netplay_status_provider();
      } else {
        body = "{\"error\":\"netplay_unavailable\"}";
      }
    }
    code = 200;
    status = "OK";
  } else if (path == "/netplay/sessions" ||
             path.rfind("/netplay/sessions?", 0) == 0) {
    {
      std::lock_guard<std::mutex> lock(g_netplay_provider_mutex);
      if (g_netplay_sessions_provider) {
        body = g_netplay_sessions_provider();
      } else {
        body = "{\"sessions\":[]}";
      }
    }
    code = 200;
    status = "OK";
  } else if (path.rfind("/cvars", 0) == 0) {
    std::string names;
    auto q = path.find('?');
    if (q != std::string::npos) {
      std::string query = path.substr(q + 1);
      auto names_pos = query.find("names=");
      if (names_pos != std::string::npos) {
        names = query.substr(names_pos + 6);
      }
    }
    body = PhoenixProbeBuildCvarsJson(names);
    code = 200;
    status = "OK";
  } else if (path.rfind("/events", 0) == 0) {
    uint64_t since_seq = 0;
    auto q = path.find('?');
    if (q != std::string::npos) {
      std::string query = path.substr(q + 1);
      auto pos = query.find("since_seq=");
      if (pos != std::string::npos) {
        since_seq = std::strtoull(query.c_str() + pos + 10, nullptr, 10);
      }
    }
    body = PhoenixProbeBuildEventsJson(since_seq);
    code = 200;
    status = "OK";
  } else {
    body = "{\"error\":\"not_found\"}";
  }

  std::string resp = HttpResponse(code, status, body);
  send(client, resp.data(), static_cast<int>(resp.size()), 0);
  closesocket(client);
}

void ServerThread(uint16_t port) {
  WSADATA wsa{};
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
    XELOGE("PhoenixProbe: WSAStartup failed");
    return;
  }

  SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (listen_sock == INVALID_SOCKET) {
    XELOGE("PhoenixProbe: socket() failed");
    return;
  }

  BOOL opt = TRUE;
  setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR,
             reinterpret_cast<const char*>(&opt), sizeof(opt));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  if (bind(listen_sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) ==
      SOCKET_ERROR) {
    XELOGE("PhoenixProbe: bind failed on port {} (error {})", port,
           WSAGetLastError());
    closesocket(listen_sock);
    return;
  }

  if (listen(listen_sock, 4) == SOCKET_ERROR) {
    XELOGE("PhoenixProbe: listen failed on port {} (error {})", port,
           WSAGetLastError());
    closesocket(listen_sock);
    return;
  }

  XELOGI("PhoenixProbe: listening on 127.0.0.1:{}", port);

  while (!g_stop.load(std::memory_order_relaxed)) {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(listen_sock, &readfds);
    timeval tv{};
    tv.tv_sec = 1;
    int sel = select(0, &readfds, nullptr, nullptr, &tv);
    if (sel > 0 && FD_ISSET(listen_sock, &readfds)) {
      SOCKET client = accept(listen_sock, nullptr, nullptr);
      if (client != INVALID_SOCKET) {
        HandleClient(client);
      }
    }
  }

  closesocket(listen_sock);
}

}  // namespace
#endif

void PhoenixProbeEnsureStarted() {
  uint32_t port = cvars::phoenix_debug_port;
  if (!port) {
    return;
  }
  if (g_started.load(std::memory_order_acquire)) {
    return;
  }
  bool expected = false;
  if (!g_started.compare_exchange_strong(expected, true)) {
    return;
  }
  g_start_time = std::chrono::steady_clock::now();
#if XE_PLATFORM_WIN32
  g_stop.store(false);
  g_server_thread =
      std::thread([port]() { ServerThread(static_cast<uint16_t>(port)); });
#else
  XELOGW("PhoenixProbe: HTTP server is only implemented on Windows in v1");
#endif
}

void PhoenixProbeShutdown() {
  if (!g_started.load()) {
    return;
  }
  g_stop.store(true);
#if XE_PLATFORM_WIN32
  if (g_server_thread.joinable()) {
    g_server_thread.join();
  }
#endif
}

void PhoenixProbeSetTitleId(uint32_t title_id) {
  std::lock_guard<std::mutex> lock(g_mutex);
  g_title_id = title_id;
  obs::SetTitleId(title_id);
}

void PhoenixProbeNotifyStubHit(std::string_view module,
                               std::string_view export_name) {
  {
    std::lock_guard<std::mutex> lock(g_mutex);
    ++g_stub_hit_count;
    g_last_stub_module = std::string(module);
    g_last_stub_export = std::string(export_name);
  }
  std::string detail(module);
  detail.append("::");
  detail.append(export_name);
  PushEvent(PhoenixProbeEventKind::kStubHit, detail);
  PhoenixProbeEnsureStarted();
}

void PhoenixProbeNotifyPcmHash(std::string_view sha256_hex) {
  std::lock_guard<std::mutex> lock(g_mutex);
  g_last_pcm_hash = std::string(sha256_hex);
}

void PhoenixProbeNotifyXmaDivergence() {
  std::lock_guard<std::mutex> lock(g_mutex);
  ++g_xma_divergence_count;
}

void PhoenixProbeNotifyUploadRangeError() {
  g_gpu_upload_range_error_count.fetch_add(1, std::memory_order_relaxed);
  PushEvent(PhoenixProbeEventKind::kUploadRangeError,
            "invalid_gpu_upload_range");
}

void PhoenixProbeNotifyPipelineSkip() {
  g_gpu_pipeline_skip_count.fetch_add(1, std::memory_order_relaxed);
  PushEvent(PhoenixProbeEventKind::kPipelineSkip, "pipeline_not_ready");
  obs::Invariant("PipelineNotReady", obs::ChannelId::kGpuPipeline, true,
                 "pipeline_not_ready");
}

void PhoenixProbeNotifyPresent() {
  g_gpu_present_count.fetch_add(1, std::memory_order_relaxed);
}

void PhoenixProbeNotifyVfsResolveFail(std::string_view path) {
  PushEvent(PhoenixProbeEventKind::kVfsResolveFail, path);
  obs::Invariant("VfsResolveFail", obs::ChannelId::kKernelStub, true, path);
}

void PhoenixProbeNotifyOwnershipChange(std::string_view detail) {
  g_gpu_ownership_change_count.fetch_add(1, std::memory_order_relaxed);
  const uint64_t n =
      g_gpu_ownership_change_count.load(std::memory_order_relaxed);
  if ((n & 15) == 1) {
    PushEvent(PhoenixProbeEventKind::kOwnershipChange, detail);
    obs::BridgeProbeEvent("ownership_change", detail);
  }
}

void PhoenixProbeNotifyEdramTransfer(std::string_view detail) {
  g_gpu_edram_transfer_count.fetch_add(1, std::memory_order_relaxed);
  const uint64_t n = g_gpu_edram_transfer_count.load(std::memory_order_relaxed);
  if ((n & 31) == 1) {
    PushEvent(PhoenixProbeEventKind::kOwnershipChange, detail);
    obs::BridgeProbeEvent("edram_transfer", detail);
  }
}

void PhoenixProbeNotifyHostDepthStore(std::string_view detail) {
  g_gpu_host_depth_store_count.fetch_add(1, std::memory_order_relaxed);
  PushEvent(PhoenixProbeEventKind::kHostDepthStore, detail);
}

void PhoenixProbeNotifyHostDepthTransferMismatch() {
  g_gpu_host_depth_transfer_mismatch_count.fetch_add(1,
                                                     std::memory_order_relaxed);
}

PhoenixProbeSnapshot PhoenixProbeGetSnapshot() {
  PhoenixProbeSnapshot s;
  {
    std::lock_guard<std::mutex> lock(g_mutex);
    s.title_id = g_title_id;
    s.stub_hit_count = g_stub_hit_count;
    s.last_stub_module = g_last_stub_module;
    s.last_stub_export = g_last_stub_export;
    s.last_pcm_hash = g_last_pcm_hash;
    s.xma_divergence_count = g_xma_divergence_count;
  }
  s.uptime_ms = UptimeMs();
  s.gpu_upload_range_error_count =
      g_gpu_upload_range_error_count.load(std::memory_order_relaxed);
  s.gpu_pipeline_skip_count =
      g_gpu_pipeline_skip_count.load(std::memory_order_relaxed);
  s.gpu_present_count = g_gpu_present_count.load(std::memory_order_relaxed);
  s.gpu_ownership_change_count =
      g_gpu_ownership_change_count.load(std::memory_order_relaxed);
  s.gpu_edram_transfer_count =
      g_gpu_edram_transfer_count.load(std::memory_order_relaxed);
  s.gpu_host_depth_store_count =
      g_gpu_host_depth_store_count.load(std::memory_order_relaxed);
  s.gpu_host_depth_transfer_mismatch_count =
      g_gpu_host_depth_transfer_mismatch_count.load(std::memory_order_relaxed);
  s.obs_depth_host_sidecar_stale_count =
      obs::InvariantCounter("DepthHostSidecarStale");
  s.obs_host_depth_transfer_mismatch_count =
      obs::InvariantCounter("HostDepthTransferMismatch");
  s.obs_gpu_upload_range_error_count =
      obs::InvariantCounter("GpuUploadRangeError");
  return s;
}

void PhoenixProbeSetNetplayJsonProviders(
    NetplayJsonProvider status_provider,
    NetplayJsonProvider sessions_provider) {
  std::lock_guard<std::mutex> lock(g_netplay_provider_mutex);
  g_netplay_status_provider = std::move(status_provider);
  g_netplay_sessions_provider = std::move(sessions_provider);
}

void PhoenixProbeClearNetplayJsonProviders() {
  std::lock_guard<std::mutex> lock(g_netplay_provider_mutex);
  g_netplay_status_provider = {};
  g_netplay_sessions_provider = {};
}

}  // namespace debug
}  // namespace xe
