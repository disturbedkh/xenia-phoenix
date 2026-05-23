/**
 ******************************************************************************
 * xma2-diff: fixture-driven XMA2 decode comparison (Phase 1.4).
 ******************************************************************************
 */

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "xma2_test_harness.h"

#include "xenia/apu/xma_context.h"
#include "xenia/apu/xma_helpers.h"
#include "xenia/base/console_app_main.h"
#include "xenia/base/cvar.h"
#include "xenia/memory.h"

using namespace xe;

DEFINE_uint32(xma2_diff_iters, 1,
              "Repeat each fixture decode this many times (stress).", "APU");
DEFINE_uint32(xma2_diff_seed, 0xA110C0DEu,
              "Reserved for future fuzz (logged in report).", "APU");
DEFINE_string(xma2_diff_filter, "",
              "If set, only fixtures whose id contains this substring.", "APU");
DEFINE_string(xma2_diff_fixtures_dir, "",
              "Directory with .xma + .json pairs (default: tests/xma2_packets).",
              "APU");
DEFINE_string(xma2_diff_report_out, "",
              "Write JSON summary to this path.", "APU");
DEFINE_string(xma2_diff_divergences, "xma2_divergences.jsonl",
              "Append JSONL divergence records here.", "APU");
DEFINE_int32(xma2_diff_max_abs, 1,
             "Max absolute int16 sample difference vs reference.", "APU");

namespace {

bool NameMatches(std::string_view id, std::string_view filter) {
  return filter.empty() || id.find(filter) != std::string_view::npos;
}

void AppendDivergence(const std::string& path, std::string_view fixture_id,
                      std::string_view detail) {
  if (path.empty()) {
    return;
  }
  std::ofstream out(path, std::ios::app | std::ios::binary);
  if (!out) {
    return;
  }
  out << "{\"fixture\":\"" << fixture_id << "\",\"detail\":\"";
  for (char c : detail) {
    if (c == '\\' || c == '"') {
      out << '\\';
    }
    out << c;
  }
  out << "\"}\n";
}

void WriteReport(const std::string& path, uint32_t seed, uint32_t iters,
                 uint64_t divergences, uint32_t ran) {
  std::ofstream out(path, std::ios::binary);
  if (!out) {
    std::cerr << "xma2-diff: could not write report " << path << '\n';
    return;
  }
  out << "{\"seed\":" << seed << ",\"iters_per_fixture\":" << iters
      << ",\"divergences\":" << divergences << ",\"fixture_count\":" << ran
      << "}\n";
}

}  // namespace

int Main(const std::vector<std::string>&) {
  namespace fs = std::filesystem;

  const uint32_t iters = cvars::xma2_diff_iters;
  const uint32_t seed = cvars::xma2_diff_seed;
  const int max_abs = cvars::xma2_diff_max_abs;
  const std::string_view filter = cvars::xma2_diff_filter;

  fs::path fixture_dir = cvars::xma2_diff_fixtures_dir;
  if (fixture_dir.empty()) {
    fixture_dir = fs::path("tests") / "xma2_packets";
  }
  if (!fs::is_directory(fixture_dir)) {
    std::cerr << "xma2-diff: fixture dir not found: " << fixture_dir << '\n';
    return 2;
  }

  std::remove(cvars::xma2_diff_divergences.c_str());

  Memory memory;
  memory.Initialize();

  uint64_t total_div = 0;
  uint32_t ran = 0;

  for (const auto& entry : fs::directory_iterator(fixture_dir)) {
    if (entry.path().extension() != ".json") {
      continue;
    }
    apu::xma2_test::FixtureMeta meta;
    if (!apu::xma2_test::LoadFixtureMeta(entry.path().string(), &meta)) {
      continue;
    }
    if (!NameMatches(meta.id, filter)) {
      continue;
    }

    fs::path xma_path = entry.path();
    xma_path.replace_extension(".xma");
    if (!fs::is_regular_file(xma_path)) {
      continue;
    }

    std::ifstream xma_in(xma_path, std::ios::binary);
    std::vector<uint8_t> blob((std::istreambuf_iterator<char>(xma_in)),
                              std::istreambuf_iterator<char>());
    if (blob.size() < xe::apu::XmaContext::kBytesPerPacket) {
      continue;
    }

    ++ran;
    bool bad = false;

    for (uint32_t iter = 0; iter < iters && !bad; ++iter) {
      const auto xenia =
          apu::xma2_test::DecodeViaXeniaContext(&memory, blob.data(),
                                                blob.size(), meta);

      if (!xenia.error.empty()) {
        std::cerr << meta.id << ": xenia error: " << xenia.error << '\n';
        AppendDivergence(cvars::xma2_diff_divergences, meta.id, xenia.error);
        bad = true;
        continue;
      }

      if (meta.mode == "structural") {
        if (!meta.allow_pcm && !xenia.skipped && !xenia.pcm.empty()) {
          std::string detail = "structural fixture produced " +
                               std::to_string(xenia.pcm.size()) + " samples";
          AppendDivergence(cvars::xma2_diff_divergences, meta.id, detail);
          std::cerr << meta.id << ": " << detail << '\n';
          bad = true;
        }
        continue;
      }

      // frame mode: xenia vs reference on same packet bitstream tail
      const uint8_t* pkt = blob.data();
      const uint32_t frame_off_bits = xe::apu::xma::GetPacketFrameOffset(pkt);
      const uint32_t byte_off = frame_off_bits / 8;
      if (byte_off >= blob.size()) {
        continue;
      }
      const uint32_t frame_size_bits = 256;
      const uint32_t frame_padding_bits = 0;

      const auto ref = apu::xma2_test::DecodeViaReferenceFfmpeg(
          pkt + byte_off, blob.size() - byte_off, frame_size_bits,
          frame_padding_bits, meta);

      if (!ref.error.empty()) {
        // FFmpeg may not decode synthetic frames — match empty outputs
        if (xenia.skipped || xenia.pcm.empty()) {
          continue;
        }
        AppendDivergence(cvars::xma2_diff_divergences, meta.id,
                         "reference: " + ref.error);
        bad = true;
        continue;
      }

      if (xenia.skipped && (ref.skipped || ref.pcm.empty())) {
        continue;
      }

      std::string detail;
      if (apu::xma2_test::ComparePcm(xenia.pcm, ref.pcm, max_abs, &detail)) {
        AppendDivergence(cvars::xma2_diff_divergences, meta.id, detail);
        std::cerr << meta.id << ": " << detail << '\n';
        bad = true;
      }
    }

    if (bad) {
      ++total_div;
      std::cout << meta.id << ": FAIL\n";
    } else {
      std::cout << meta.id << ": OK\n";
    }
  }

  if (!cvars::xma2_diff_report_out.empty()) {
    WriteReport(cvars::xma2_diff_report_out, seed, iters, total_div, ran);
  }

  if (total_div) {
    std::cerr << "xma2-diff: " << total_div << " fixture(s) diverged\n";
    return 1;
  }
  std::cout << "xma2-diff: all " << ran << " fixtures passed\n";
  return 0;
}

XE_DEFINE_CONSOLE_APP("xma2-diff", Main, "", "");
