/**
 ******************************************************************************
 * vmx128-fuzz: registry-driven differential VMX/VMX128 fuzz vs scalar / SSE
 * references (guest PPC block + JIT).
 ******************************************************************************
 */

#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <string_view>
#include <vector>

#include "fuzz_internal.h"

#include "xenia/base/console_app_main.h"
#include "xenia/base/cvar.h"
#include "xenia/cpu/testing/guest_ppc_test_util.h"

using namespace xe;

DEFINE_uint32(vmx128_fuzz_iters, 100000,
              "Iterations per VMX opcode in vmx128-fuzz differential run.", "CPU");
DEFINE_uint32(vmx128_fuzz_seed, 0xC0FFEEu,
              "RNG seed for vmx128-fuzz (0 = use wall-clock).", "CPU");
DEFINE_string(
    vmx128_fuzz_filter, "",
    "If non-empty, only run opcodes whose name contains this substring.", "CPU");
DEFINE_string(
    vmx128_fuzz_report_out, "",
    "If non-empty, write JSON summary of the run to this path.", "CPU");
DEFINE_string(
    vmx128_fuzz_rm_pass, "rn",
    "Rounding pass: 'rn' (default) or 'all' (iterate RN/RZ/RP/RM for "
    "rounding-sensitive opcodes in fuzz refs).",
    "CPU");

namespace {

bool NameMatchesFilter(std::string_view name, std::string_view filter) {
  if (filter.empty()) {
    return true;
  }
  if (filter.front() == '^') {
    return name == filter.substr(1);
  }
  return name.find(filter) != std::string_view::npos;
}

bool IsRoundingSensitive(std::string_view name) {
  return name.find("vmadd") != std::string_view::npos ||
         name.find("vnmsub") != std::string_view::npos ||
         name.find("vrefp") != std::string_view::npos ||
         name.find("vrfi") != std::string_view::npos ||
         name.find("vaddfp") != std::string_view::npos ||
         name.find("vsubfp") != std::string_view::npos ||
         name.find("vmaxfp") != std::string_view::npos ||
         name.find("vminfp") != std::string_view::npos;
}

void WriteJsonReport(const std::string& path, uint32_t seed, uint32_t iters,
                     std::string_view filter, std::string_view rm_pass,
                     uint64_t total_mismatches, uint32_t op_count) {
  std::ofstream out(path, std::ios::binary);
  if (!out) {
    std::cerr << "vmx128-fuzz: could not open report file: " << path << '\n';
    return;
  }
  out << "{\"seed\":" << seed << ",\"iters_per_op\":" << iters << ",\"filter\":\""
      << filter << "\",\"vmx128_fuzz_rm_pass\":\"" << rm_pass
      << "\",\"total_mismatches\":" << total_mismatches
      << ",\"opcode_count\":" << op_count << "}\n";
}

uint32_t RunCase(const xe::vmx128_fuzz::FuzzCase& c, uint32_t iters,
                   std::mt19937& rng, xe::cpu::testing::TestGuestPpcBlock& block,
                   std::string_view rm_pass) {
  const bool rm_all = (rm_pass == "all");
  if (!rm_all || !IsRoundingSensitive(c.name)) {
    xe::cpu::testing::GuestPpcFuzzRoundingMode() = -1;
    return c.run(block, iters, rng);
  }
  uint32_t bad = 0;
  for (int rn = 0; rn < 4; ++rn) {
    xe::cpu::testing::GuestPpcFuzzRoundingMode() = rn;
    bad += c.run(block, iters, rng);
  }
  xe::cpu::testing::GuestPpcFuzzRoundingMode() = -1;
  return bad;
}

}  // namespace

int Main(const std::vector<std::string>&) {
  xe::vmx128_fuzz::RegisterAllFuzzCases();
  auto& cases = xe::vmx128_fuzz::FuzzCases();

  const uint32_t iters = cvars::vmx128_fuzz_iters;
  uint32_t seed = cvars::vmx128_fuzz_seed;
  if (!seed) {
    seed = static_cast<uint32_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
  }
  std::mt19937 rng(seed);

  const std::string_view filter = cvars::vmx128_fuzz_filter;
  const std::string_view rm_pass = cvars::vmx128_fuzz_rm_pass;

  std::remove("vmx128_divergences.jsonl");

  uint64_t total_bad = 0;
  uint32_t ran = 0;

  for (const auto& c : cases) {
    if (!NameMatchesFilter(c.name, filter)) {
      continue;
    }
    ++ran;
    // Fresh processor/backend per opcode so x64 unwind table slots do not
    // accumulate across 100+ guest compiles in one 1M-iter registry run.
    xe::cpu::testing::TestGuestPpcBlock block;
    const uint32_t bad = RunCase(c, iters, rng, block, rm_pass);
    if (bad) {
      std::cerr << c.name << ": " << bad << " / " << iters << " mismatches\n";
      total_bad += bad;
    } else {
      std::cout << c.name << ": OK (" << iters << ")\n";
    }
  }

  if (!cvars::vmx128_fuzz_report_out.empty()) {
    WriteJsonReport(cvars::vmx128_fuzz_report_out, seed, iters, filter, rm_pass,
                    total_bad, ran);
  }

  if (!ran) {
    std::cerr << "vmx128-fuzz: no opcodes matched filter.\n";
    return 2;
  }

  if (total_bad) {
    std::cerr << "vmx128-fuzz: total mismatches " << total_bad
              << " (see vmx128_divergences.jsonl in cwd)\n";
    return 1;
  }
  std::cout << "vmx128-fuzz: all " << ran << " selected opcodes OK\n";
  return 0;
}

XE_DEFINE_CONSOLE_APP("vmx128-fuzz", Main, "", "");
