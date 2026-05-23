/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_GPU_DRAW_RESOLUTION_SCALE_LIMITS_H_
#define XENIA_GPU_DRAW_RESOLUTION_SCALE_LIMITS_H_

#include <cstdint>
#include <string>

namespace xe {
namespace gpu {

struct DrawResolutionScaleLimits {
  uint32_t requested_x = 1;
  uint32_t requested_y = 1;
  uint32_t effective_x = 1;
  uint32_t effective_y = 1;
  uint32_t hard_max = 7;
  bool sparse_or_tiled_ok = false;
  std::string clamp_reason;
};

// Query effective internal scale for UI (uses cvars + device caps when known).
DrawResolutionScaleLimits QueryDrawResolutionScaleLimits(
    bool sparse_or_tiled_supported, uint32_t virtual_address_bits_per_resource);

}  // namespace gpu
}  // namespace xe

#endif  // XENIA_GPU_DRAW_RESOLUTION_SCALE_LIMITS_H_
