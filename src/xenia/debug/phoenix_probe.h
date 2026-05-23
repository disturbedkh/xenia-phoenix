/**
 ******************************************************************************
 * Phoenix localhost debug probe — in-process status API.
 ******************************************************************************
 */
#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace xe {
namespace debug {

struct PhoenixProbeSnapshot {
  uint32_t title_id = 0;
  uint64_t uptime_ms = 0;
  uint64_t stub_hit_count = 0;
  std::string last_stub_module;
  std::string last_stub_export;
  std::string last_pcm_hash;
  uint64_t xma_divergence_count = 0;
  uint64_t gpu_upload_range_error_count = 0;
  uint64_t gpu_pipeline_skip_count = 0;
  uint64_t gpu_present_count = 0;
  uint64_t gpu_ownership_change_count = 0;
  uint64_t gpu_edram_transfer_count = 0;
  uint64_t gpu_host_depth_store_count = 0;
  uint64_t gpu_host_depth_transfer_mismatch_count = 0;
  uint64_t obs_depth_host_sidecar_stale_count = 0;
  uint64_t obs_host_depth_transfer_mismatch_count = 0;
  uint64_t obs_gpu_upload_range_error_count = 0;
};

enum class PhoenixProbeEventKind : uint8_t {
  kStubHit = 0,
  kUploadRangeError = 1,
  kVfsResolveFail = 2,
  kPipelineSkip = 3,
  kOwnershipChange = 4,
  kHostDepthStore = 5,
};

void PhoenixProbeEnsureStarted();
void PhoenixProbeShutdown();

void PhoenixProbeSetTitleId(uint32_t title_id);
void PhoenixProbeNotifyStubHit(std::string_view module,
                              std::string_view export_name);
void PhoenixProbeNotifyPcmHash(std::string_view sha256_hex);
void PhoenixProbeNotifyXmaDivergence();
void PhoenixProbeNotifyUploadRangeError();
void PhoenixProbeNotifyPipelineSkip();
void PhoenixProbeNotifyPresent();
void PhoenixProbeNotifyVfsResolveFail(std::string_view path);
void PhoenixProbeNotifyOwnershipChange(std::string_view detail);
void PhoenixProbeNotifyEdramTransfer(std::string_view detail);
void PhoenixProbeNotifyHostDepthStore(std::string_view detail);
void PhoenixProbeNotifyHostDepthTransferMismatch();

PhoenixProbeSnapshot PhoenixProbeGetSnapshot();

// JSON payloads for HTTP handlers (v2).
std::string PhoenixProbeBuildHealthJson();
std::string PhoenixProbeBuildStatusJson();
std::string PhoenixProbeBuildSnapshotJson();
std::string PhoenixProbeBuildCvarsJson(std::string_view names_csv);
std::string PhoenixProbeBuildEventsJson(uint64_t since_seq);

}  // namespace debug
}  // namespace xe
