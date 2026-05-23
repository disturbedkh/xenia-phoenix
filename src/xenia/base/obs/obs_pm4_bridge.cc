/**
 ******************************************************************************
 * PM4 ↔ observability bridge implementation.
 ******************************************************************************
 */

#include "xenia/base/obs/obs_pm4_bridge.h"

#include "third_party/fmt/include/fmt/format.h"
#include "xenia/base/obs/obs_invariant.h"

namespace xe {
namespace obs_pm4_bridge {

void NotifyUnimplementedOpcode(uint32_t opcode, uint32_t count) {
  obs::Invariant("PM4UnimplementedOpcode", obs::ChannelId::kGpuPipeline, true,
                 fmt::format("opcode=0x{:02X} count={}", opcode, count));
}

}  // namespace obs_pm4_bridge
}  // namespace xe
