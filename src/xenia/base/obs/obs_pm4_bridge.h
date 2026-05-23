/**
 ******************************************************************************
 * PM4 ↔ observability bridge (included from GPU command processor headers).
 ******************************************************************************
 */
#pragma once

#include <cstdint>

namespace xe {
namespace obs_pm4_bridge {

void NotifyUnimplementedOpcode(uint32_t opcode, uint32_t count);

}  // namespace obs_pm4_bridge
}  // namespace xe
