/**
 ******************************************************************************
 * XMA2 offline test harness (Phase 1.4).
 ******************************************************************************
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace xe {
class Memory;
namespace apu {
namespace xma2_test {

struct FixtureMeta {
  std::string id;
  std::string mode = "structural";
  uint32_t sample_rate_id = 3;
  uint32_t sample_rate_hz = 48000;
  uint32_t channels = 1;
  uint32_t is_stereo = 0;
  uint32_t packet_count = 1;
  uint32_t read_offset_bits = 32;
  uint32_t loop_count = 0;
  bool allow_pcm = false;
};

struct DecodeResult {
  std::vector<int16_t> pcm;
  std::string error;
  bool skipped = false;
};

bool LoadFixtureMeta(const std::string& json_path, FixtureMeta* out);

DecodeResult DecodeViaXeniaContext(Memory* memory, const uint8_t* packets,
                                   size_t packet_bytes, const FixtureMeta& meta);

DecodeResult DecodeViaReferenceFfmpeg(const uint8_t* xma_frame,
                                      size_t xma_frame_size,
                                      uint32_t frame_size_bits,
                                      uint32_t frame_padding_bits,
                                      const FixtureMeta& meta);

int ComparePcm(const std::vector<int16_t>& a, const std::vector<int16_t>& b,
               int max_abs_diff, std::string* detail);

}  // namespace xma2_test
}  // namespace apu
}  // namespace xe
