/**
 ******************************************************************************
 * GPU trace ↔ observability event correlation sidecars.
 ******************************************************************************
 */
#pragma once

#include <cstdint>
#include <filesystem>

namespace xe {
namespace obs {

void WriteTraceObsSidecar(const std::filesystem::path& xtr_path,
                          uint64_t seq_first, uint64_t seq_last,
                          uint32_t frame_id = 0);

}  // namespace obs
}  // namespace xe
