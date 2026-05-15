/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Tier 0 / Tier 1.1: differential fuzz for VMX integer add/sub vs scalar C++
 * references (guest PPC block + JIT).
 ******************************************************************************
 */

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "xenia/base/console_app_main.h"
#include "xenia/base/cvar.h"
#include "xenia/base/vec128.h"
#include "xenia/cpu/testing/guest_ppc_test_util.h"

using namespace xe;

DEFINE_uint32(vmx128_fuzz_iters, 100000,
              "Iterations per VMX opcode in vmx128-fuzz differential run.", "CPU");

namespace {

constexpr uint32_t kVd = 5;
constexpr uint32_t kVa = 3;
constexpr uint32_t kVb = 4;

xe::vec128_t RandomVec(std::mt19937& rng) {
  xe::vec128_t v = {};
  for (int i = 0; i < 16; ++i) {
    v.u8[i] = static_cast<uint8_t>(rng());
  }
  return v;
}

void AppendVecFields(std::ostringstream& oss, std::string_view key,
                     const vec128_t& v) {
  oss << '"' << key << "_lo\":\"" << std::hex << std::uppercase << v.low
      << "\",\"" << key << "_hi\":\"" << v.high << '"' << std::dec;
}

// --- Modulo (unsigned wrap) -------------------------------------------------

vec128_t Ref_vaddubm(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int i = 0; i < 16; ++i) {
    o.u8[i] = static_cast<uint8_t>(uint16_t(a.u8[i]) + uint16_t(b.u8[i]));
  }
  return o;
}

vec128_t Ref_vadduhm(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int i = 0; i < 8; ++i) {
    o.u16[i] = static_cast<uint16_t>(uint32_t(a.u16[i]) + uint32_t(b.u16[i]));
  }
  return o;
}

vec128_t Ref_vadduwm(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int i = 0; i < 4; ++i) {
    o.u32[i] = a.u32[i] + b.u32[i];
  }
  return o;
}

vec128_t Ref_vsububm(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int i = 0; i < 16; ++i) {
    o.u8[i] = static_cast<uint8_t>(uint16_t(a.u8[i]) - uint16_t(b.u8[i]));
  }
  return o;
}

vec128_t Ref_vsubuhm(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int i = 0; i < 8; ++i) {
    o.u16[i] = static_cast<uint16_t>(uint32_t(a.u16[i]) - uint32_t(b.u16[i]));
  }
  return o;
}

vec128_t Ref_vsubuwm(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int i = 0; i < 4; ++i) {
    o.u32[i] = a.u32[i] - b.u32[i];
  }
  return o;
}

// --- Unsigned saturate -------------------------------------------------------

vec128_t Ref_vaddubs(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int i = 0; i < 16; ++i) {
    int s = int(a.u8[i]) + int(b.u8[i]);
    o.u8[i] = static_cast<uint8_t>(std::min(s, 255));
  }
  return o;
}

vec128_t Ref_vadduhs(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int i = 0; i < 8; ++i) {
    uint32_t s = uint32_t(a.u16[i]) + uint32_t(b.u16[i]);
    o.u16[i] = s > 65535u ? 65535u : static_cast<uint16_t>(s);
  }
  return o;
}

vec128_t Ref_vadduws(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int i = 0; i < 4; ++i) {
    uint64_t s = uint64_t(a.u32[i]) + uint64_t(b.u32[i]);
    o.u32[i] = s > 0xFFFFFFFFu ? 0xFFFFFFFFu : static_cast<uint32_t>(s);
  }
  return o;
}

vec128_t Ref_vsububs(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int i = 0; i < 16; ++i) {
    int d = int(a.u8[i]) - int(b.u8[i]);
    o.u8[i] = static_cast<uint8_t>(std::max(d, 0));
  }
  return o;
}

vec128_t Ref_vsubuhs(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int i = 0; i < 8; ++i) {
    int d = int(a.u16[i]) - int(b.u16[i]);
    o.u16[i] = static_cast<uint16_t>(std::max(d, 0));
  }
  return o;
}

vec128_t Ref_vsubuws(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int i = 0; i < 4; ++i) {
    if (a.u32[i] < b.u32[i]) {
      o.u32[i] = 0;
    } else {
      o.u32[i] = a.u32[i] - b.u32[i];
    }
  }
  return o;
}

// --- Signed saturate ---------------------------------------------------------

vec128_t Ref_vaddsbs(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int i = 0; i < 16; ++i) {
    int s = int(int32_t(int8_t(a.u8[i]))) + int(int32_t(int8_t(b.u8[i])));
    o.i8[i] = static_cast<int8_t>(std::clamp(s, -128, 127));
  }
  return o;
}

vec128_t Ref_vaddshs(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int i = 0; i < 8; ++i) {
    int s = int(int32_t(int16_t(a.u16[i]))) + int(int32_t(int16_t(b.u16[i])));
    o.i16[i] = static_cast<int16_t>(std::clamp(s, -32768, 32767));
  }
  return o;
}

vec128_t Ref_vaddsws(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int i = 0; i < 4; ++i) {
    int64_t s = int64_t(int32_t(a.u32[i])) + int64_t(int32_t(b.u32[i]));
    int64_t c = std::clamp(s, int64_t(-2147483648LL), int64_t(2147483647LL));
    o.i32[i] = static_cast<int32_t>(c);
  }
  return o;
}

vec128_t Ref_vsubsbs(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int i = 0; i < 16; ++i) {
    int s = int(int32_t(int8_t(a.u8[i]))) - int(int32_t(int8_t(b.u8[i])));
    o.i8[i] = static_cast<int8_t>(std::clamp(s, -128, 127));
  }
  return o;
}

vec128_t Ref_vsubshs(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int i = 0; i < 8; ++i) {
    int s = int(int32_t(int16_t(a.u16[i]))) - int(int32_t(int16_t(b.u16[i])));
    o.i16[i] = static_cast<int16_t>(std::clamp(s, -32768, 32767));
  }
  return o;
}

vec128_t Ref_vsubsws(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int i = 0; i < 4; ++i) {
    int64_t s = int64_t(int32_t(a.u32[i])) - int64_t(int32_t(b.u32[i]));
    int64_t c = std::clamp(s, int64_t(-2147483648LL), int64_t(2147483647LL));
    o.i32[i] = static_cast<int32_t>(c);
  }
  return o;
}

struct OpcodeCase {
  const char* name;
  uint32_t xo;
  vec128_t (*ref)(const vec128_t&, const vec128_t&);
};

const OpcodeCase kCases[] = {
    {"vaddubm", xe::cpu::testing::kVxXO_vaddubm, Ref_vaddubm},
    {"vadduhm", xe::cpu::testing::kVxXO_vadduhm, Ref_vadduhm},
    {"vadduwm", xe::cpu::testing::kVxXO_vadduwm, Ref_vadduwm},
    {"vsububm", xe::cpu::testing::kVxXO_vsububm, Ref_vsububm},
    {"vsubuhm", xe::cpu::testing::kVxXO_vsubuhm, Ref_vsubuhm},
    {"vsubuwm", xe::cpu::testing::kVxXO_vsubuwm, Ref_vsubuwm},
    {"vaddubs", xe::cpu::testing::kVxXO_vaddubs, Ref_vaddubs},
    {"vadduhs", xe::cpu::testing::kVxXO_vadduhs, Ref_vadduhs},
    {"vadduws", xe::cpu::testing::kVxXO_vadduws, Ref_vadduws},
    {"vsububs", xe::cpu::testing::kVxXO_vsububs, Ref_vsububs},
    {"vsubuhs", xe::cpu::testing::kVxXO_vsubuhs, Ref_vsubuhs},
    {"vsubuws", xe::cpu::testing::kVxXO_vsubuws, Ref_vsubuws},
    {"vaddsbs", xe::cpu::testing::kVxXO_vaddsbs, Ref_vaddsbs},
    {"vaddshs", xe::cpu::testing::kVxXO_vaddshs, Ref_vaddshs},
    {"vaddsws", xe::cpu::testing::kVxXO_vaddsws, Ref_vaddsws},
    {"vsubsbs", xe::cpu::testing::kVxXO_vsubsbs, Ref_vsubsbs},
    {"vsubshs", xe::cpu::testing::kVxXO_vsubshs, Ref_vsubshs},
    {"vsubsws", xe::cpu::testing::kVxXO_vsubsws, Ref_vsubsws},
};

void LogDivergence(const char* op, const vec128_t& a, const vec128_t& b,
                   const vec128_t& expect, const vec128_t& got) {
  std::ofstream f("vmx128_divergences.jsonl", std::ios::app);
  if (!f) {
    return;
  }
  std::ostringstream oss;
  oss << "{\"op\":\"" << op << "\",";
  AppendVecFields(oss, "a", a);
  oss << ',';
  AppendVecFields(oss, "b", b);
  oss << ',';
  AppendVecFields(oss, "expect", expect);
  oss << ',';
  AppendVecFields(oss, "got", got);
  oss << "}\n";
  f << oss.str();
}

uint32_t RunCase(cpu::testing::TestGuestPpcBlock& block, const OpcodeCase& op,
                 uint32_t iters) {
  const std::vector<uint32_t> ins = {
      xe::cpu::testing::EncodeVxAltivec(kVd, kVa, kVb, op.xo),
      xe::cpu::testing::kPpcInsnBlr,
  };
  std::mt19937 rng(0xC0FFEEu);
  uint32_t mismatches = 0;
  for (uint32_t i = 0; i < iters; ++i) {
    vec128_t a = RandomVec(rng);
    vec128_t b = RandomVec(rng);
    vec128_t expect = op.ref(a, b);
    block.Run(
        ins,
        [&](cpu::ppc::PPCContext* ctx) {
          ctx->v[kVa] = a;
          ctx->v[kVb] = b;
        },
        [&](cpu::ppc::PPCContext* ctx) {
          if (ctx->v[kVd] != expect) {
            ++mismatches;
            if (mismatches <= 32) {
              LogDivergence(op.name, a, b, expect, ctx->v[kVd]);
            }
          }
        });
  }
  return mismatches;
}

}  // namespace

int Main(const std::vector<std::string>&) {
  xe::cpu::testing::TestGuestPpcBlock block;
  const uint32_t iters = cvars::vmx128_fuzz_iters;
  uint32_t total_bad = 0;
  std::remove("vmx128_divergences.jsonl");
  for (const auto& op : kCases) {
    uint32_t bad = RunCase(block, op, iters);
    if (bad) {
      std::cerr << op.name << ": " << bad << " / " << iters << " mismatches\n";
      total_bad += bad;
    } else {
      std::cout << op.name << ": OK (" << iters << ")\n";
    }
  }
  if (total_bad) {
    std::cerr << "vmx128-fuzz: total mismatches " << total_bad << " (see "
                 "vmx128_divergences.jsonl in cwd)\n";
    return 1;
  }
  std::cout << "vmx128-fuzz: all " << std::size(kCases) << " opcodes OK\n";
  return 0;
}

XE_DEFINE_CONSOLE_APP("vmx128-fuzz", Main, "", "");
