/**
 ******************************************************************************
 * XMA2 offline test harness (Phase 1.4).
 ******************************************************************************
 */

#include "xma2_test_harness.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>

#include "xenia/apu/xma_context.h"
#include "xenia/apu/xma_context_new.h"
#include "xenia/apu/xma_helpers.h"
#include "xenia/base/logging.h"
#include "xenia/memory.h"

extern "C" {
#if XE_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 4101 4244 5033)
#endif
#include "third_party/FFmpeg/libavcodec/avcodec.h"
#include "third_party/FFmpeg/libavutil/channel_layout.h"
#include "third_party/FFmpeg/libavutil/error.h"
#if XE_COMPILER_MSVC
#pragma warning(pop)
#endif
}  // extern "C"

namespace xe {
namespace apu {
namespace xma2_test {
namespace {

uint32_t ParseJsonUint(const std::string& text, const char* key,
                       uint32_t default_value) {
  const std::string key_needle = std::string("\"") + key + "\"";
  const size_t key_pos = text.find(key_needle);
  if (key_pos == std::string::npos) {
    return default_value;
  }
  size_t colon = text.find(':', key_pos + key_needle.size());
  if (colon == std::string::npos) {
    return default_value;
  }
  size_t start = colon + 1;
  while (start < text.size() && (text[start] == ' ' || text[start] == '\t')) {
    ++start;
  }
  return static_cast<uint32_t>(
      std::strtoul(text.c_str() + start, nullptr, 10));
}

std::string ParseJsonString(const std::string& text, const char* key) {
  const std::string key_needle = std::string("\"") + key + "\"";
  const size_t key_pos = text.find(key_needle);
  if (key_pos == std::string::npos) {
    return {};
  }
  size_t colon = text.find(':', key_pos + key_needle.size());
  if (colon == std::string::npos) {
    return {};
  }
  size_t start = colon + 1;
  while (start < text.size() && (text[start] == ' ' || text[start] == '\t')) {
    ++start;
  }
  if (start >= text.size() || text[start] != '"') {
    return {};
  }
  ++start;
  const size_t end = text.find('"', start);
  if (end == std::string::npos) {
    return {};
  }
  return text.substr(start, end - start);
}

bool ReferenceDecodeFrame(const uint8_t* frame_data, size_t frame_size_bits,
                         uint32_t frame_padding_bits, const FixtureMeta& meta,
                         std::vector<int16_t>* pcm_out, std::string* error) {
  const AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_XMAFRAMES);
  if (!codec) {
    *error = "FFmpeg XMAFRAMES codec missing";
    return false;
  }

  AVCodecContext* ctx = avcodec_alloc_context3(codec);
  AVFrame* frame = av_frame_alloc();
  AVPacket* pkt = av_packet_alloc();
  if (!ctx || !frame || !pkt) {
    *error = "FFmpeg alloc failed";
    avcodec_free_context(&ctx);
    av_frame_free(&frame);
    av_packet_free(&pkt);
    return false;
  }

  ctx->sample_rate = static_cast<int>(meta.sample_rate_hz);
  av_channel_layout_default(&ctx->ch_layout, meta.is_stereo ? 2 : 1);
  ctx->flags2 |= AV_CODEC_FLAG2_SKIP_MANUAL;
  if (avcodec_open2(ctx, codec, nullptr) < 0) {
    *error = "avcodec_open2 failed";
    avcodec_free_context(&ctx);
    av_frame_free(&frame);
    av_packet_free(&pkt);
    return false;
  }

  std::array<uint8_t, 128 * 1024> xma_frame{};
  std::memcpy(xma_frame.data() + 1, frame_data, std::min(xma_frame.size() - 1,
                                                         size_t(64 * 1024)));

  pkt->data = xma_frame.data();
  pkt->size = static_cast<int>(
      1 + ((frame_padding_bits + frame_size_bits) / 8) +
      (((frame_padding_bits + frame_size_bits) % 8) ? 1 : 0));
  const auto padding_end =
      pkt->size * 8 - (8 + frame_padding_bits + frame_size_bits);
  xma_frame[0] = static_cast<uint8_t>(((frame_padding_bits & 7) << 5) |
                                      ((padding_end & 7) << 2));

  if (avcodec_send_packet(ctx, pkt) < 0) {
    *error = "avcodec_send_packet failed";
    avcodec_free_context(&ctx);
    av_frame_free(&frame);
    av_packet_free(&pkt);
    return false;
  }

  const int ret = avcodec_receive_frame(ctx, frame);
  if (ret == AVERROR(EAGAIN)) {
    pcm_out->clear();
    avcodec_free_context(&ctx);
    av_frame_free(&frame);
    av_packet_free(&pkt);
    return true;
  }
  if (ret < 0) {
    *error = "avcodec_receive_frame failed";
    avcodec_free_context(&ctx);
    av_frame_free(&frame);
    av_packet_free(&pkt);
    return false;
  }

  const size_t sample_count =
      XmaContext::kSamplesPerFrame * (meta.is_stereo ? 2u : 1u);
  pcm_out->resize(sample_count);
  constexpr float scale = (1 << 15) - 1;
  const bool stereo = meta.is_stereo != 0;
  const auto* ch0 = reinterpret_cast<const float*>(frame->data[0]);
  const auto* ch1 =
      stereo && frame->ch_layout.nb_channels >= 2
          ? reinterpret_cast<const float*>(frame->data[1])
          : ch0;
  for (size_t i = 0; i < XmaContext::kSamplesPerFrame; ++i) {
    const auto clamp = [](float v) -> int16_t {
      v = std::max(-1.f, std::min(1.f, v));
      return static_cast<int16_t>(v * scale);
    };
    if (stereo) {
      (*pcm_out)[i * 2] = clamp(ch0[i]);
      (*pcm_out)[i * 2 + 1] = clamp(ch1[i]);
    } else {
      (*pcm_out)[i] = clamp(ch0[i]);
    }
  }

  avcodec_free_context(&ctx);
  av_frame_free(&frame);
  av_packet_free(&pkt);
  return true;
}

}  // namespace

bool LoadFixtureMeta(const std::string& json_path, FixtureMeta* out) {
  std::ifstream in(json_path, std::ios::binary);
  if (!in) {
    return false;
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  const std::string text = ss.str();

  out->id = ParseJsonString(text, "id");
  out->mode = ParseJsonString(text, "mode");
  if (out->mode.empty()) {
    out->mode = "structural";
  }
  out->sample_rate_id = ParseJsonUint(text, "sample_rate_id", 3);
  out->sample_rate_hz = ParseJsonUint(text, "sample_rate_hz", 48000);
  out->channels = ParseJsonUint(text, "channels", 1);
  out->is_stereo = ParseJsonUint(text, "is_stereo", 0);
  out->packet_count = ParseJsonUint(text, "packet_count", 1);
  out->read_offset_bits = ParseJsonUint(text, "read_offset_bits", 32);
  out->loop_count = ParseJsonUint(text, "loop_count", 0);
  out->allow_pcm = text.find("\"allow_pcm\": true") != std::string::npos ||
                   text.find("\"allow_pcm\":true") != std::string::npos;
  return !out->id.empty();
}

DecodeResult DecodeViaXeniaContext(Memory* memory, const uint8_t* packets,
                                 size_t packet_bytes,
                                 const FixtureMeta& meta) {
  DecodeResult result;
  if (!memory || !packets || packet_bytes < XmaContext::kBytesPerPacket) {
    result.error = "invalid input";
    return result;
  }

  const uint32_t packet_count =
      static_cast<uint32_t>(packet_bytes / XmaContext::kBytesPerPacket);
  const uint32_t context_guest =
      memory->SystemHeapAlloc(sizeof(XMA_CONTEXT_DATA), 64);
  const uint32_t input_phys = memory->SystemHeapAlloc(
      static_cast<uint32_t>(packet_bytes), XmaContext::kBytesPerPacket,
      kSystemHeapPhysical);
  const uint32_t output_phys = memory->SystemHeapAlloc(
      XmaContext::kOutputMaxSizeBytes, XmaContext::kOutputBytesPerBlock,
      kSystemHeapPhysical);

  if (!context_guest || !input_phys || !output_phys) {
    result.error = "SystemHeapAlloc failed";
    return result;
  }

  std::memcpy(memory->TranslatePhysical(input_phys), packets, packet_bytes);
  std::memset(memory->TranslatePhysical(output_phys), 0,
              XmaContext::kOutputMaxSizeBytes);

  std::memset(memory->TranslateVirtual(context_guest), 0,
              sizeof(XMA_CONTEXT_DATA));
  XMA_CONTEXT_DATA data(memory->TranslateVirtual(context_guest));
  data.input_buffer_0_packet_count = packet_count;
  data.input_buffer_0_valid = 1;
  data.input_buffer_0_ptr = input_phys;
  data.output_buffer_valid = 1;
  data.output_buffer_ptr = output_phys;
  data.output_buffer_block_count = 31;
  data.sample_rate = meta.sample_rate_id;
  data.is_stereo = meta.is_stereo;
  data.subframe_decode_count = 8;
  data.input_buffer_read_offset = meta.read_offset_bits;
  data.current_buffer = 0;
  data.loop_count = meta.loop_count;
  data.Store(memory->TranslateVirtual(context_guest));

  XmaContextNew ctx;
  if (ctx.Setup(0, memory, context_guest) != 0) {
    result.error = "XmaContextNew::Setup failed";
    return result;
  }
  ctx.set_is_allocated(true);
  ctx.Enable();
  if (!ctx.Work()) {
    result.error = "XmaContextNew::Work returned false";
    return result;
  }

  const XMA_CONTEXT_DATA after(memory->TranslateVirtual(context_guest));
  const uint32_t output_bytes =
      after.output_buffer_write_offset * XmaContext::kOutputBytesPerBlock;
  if (output_bytes == 0) {
    result.skipped = true;
    return result;
  }

  const uint8_t* out_ptr = memory->TranslatePhysical(output_phys);
  const size_t sample_count = output_bytes / sizeof(int16_t);
  result.pcm.resize(sample_count);
  std::memcpy(result.pcm.data(), out_ptr, output_bytes);
  return result;
}

DecodeResult DecodeViaReferenceFfmpeg(const uint8_t* xma_frame,
                                      size_t xma_frame_size,
                                      uint32_t frame_size_bits,
                                      uint32_t frame_padding_bits,
                                      const FixtureMeta& meta) {
  DecodeResult result;
  if (!xma_frame || xma_frame_size == 0) {
    result.skipped = true;
    return result;
  }
  if (!ReferenceDecodeFrame(xma_frame, frame_size_bits, frame_padding_bits,
                            meta, &result.pcm, &result.error)) {
    return result;
  }
  if (result.pcm.empty()) {
    result.skipped = true;
  }
  return result;
}

int ComparePcm(const std::vector<int16_t>& a, const std::vector<int16_t>& b,
               int max_abs_diff, std::string* detail) {
  if (a.size() != b.size()) {
    if (detail) {
      *detail = "length mismatch " + std::to_string(a.size()) + " vs " +
                std::to_string(b.size());
    }
    return 1;
  }
  for (size_t i = 0; i < a.size(); ++i) {
    const int diff = std::abs(static_cast<int>(a[i]) - static_cast<int>(b[i]));
    if (diff > max_abs_diff) {
      if (detail) {
        *detail = "sample " + std::to_string(i) + " diff " +
                  std::to_string(diff) + " (" + std::to_string(a[i]) + " vs " +
                  std::to_string(b[i]) + ")";
      }
      return 1;
    }
  }
  return 0;
}

}  // namespace xma2_test
}  // namespace apu
}  // namespace xe
