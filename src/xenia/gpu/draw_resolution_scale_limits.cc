/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/gpu/draw_resolution_scale_limits.h"

#include "xenia/gpu/texture_cache.h"

namespace xe {
namespace gpu {

DrawResolutionScaleLimits QueryDrawResolutionScaleLimits(
    bool sparse_or_tiled_supported,
    uint32_t virtual_address_bits_per_resource) {
  DrawResolutionScaleLimits limits;
  limits.hard_max = TextureCache::kMaxDrawResolutionScaleAlongAxis;
  limits.sparse_or_tiled_ok = sparse_or_tiled_supported;

  bool not_clamped = TextureCache::GetConfigDrawResolutionScale(
      limits.requested_x, limits.requested_y);
  limits.effective_x = limits.requested_x;
  limits.effective_y = limits.requested_y;

  if (!TextureCache::ClampDrawResolutionScaleToMaxSupported(
          limits.effective_x, limits.effective_y, sparse_or_tiled_supported,
          virtual_address_bits_per_resource)) {
    if (!not_clamped) {
      limits.clamp_reason =
          "Requested scale exceeds the maximum (7x7 per axis).";
    } else if (!sparse_or_tiled_supported) {
      limits.clamp_reason =
          "GPU lacks sparse/tiled resources required for internal scaling "
          "above 1x.";
    } else {
      limits.clamp_reason =
          "Scale reduced to fit GPU virtual address space per resource.";
    }
  } else if (!sparse_or_tiled_supported &&
             (limits.requested_x > 1 || limits.requested_y > 1)) {
    limits.clamp_reason =
        "Enable D3D12 tiled resources or Vulkan sparse binding for scaling.";
  }

  return limits;
}

}  // namespace gpu
}  // namespace xe
