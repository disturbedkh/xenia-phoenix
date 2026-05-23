/**
 ******************************************************************************
 * Logical, min/max, average, carry-out add/sub (implemented VMX paths).
 ******************************************************************************
 */

#include <algorithm>
#include <cmath>

#include "fuzz_internal.h"

namespace xe::vmx128_fuzz {
namespace {

uint32_t RunVx(cpu::testing::TestGuestPpcBlock& block, uint32_t iters,
               std::mt19937& rng, const char* name, uint32_t insn,
               vec128_t (*ref)(const vec128_t&, const vec128_t&)) {
  const std::vector<uint32_t> ins = {PatchVx(insn, kVd, kVa, kVb), kPpcInsnBlr};
  uint32_t bad = 0;
  for (uint32_t i = 0; i < iters; ++i) {
    vec128_t a = RandomVec(rng);
    vec128_t b = RandomVec(rng);
    vec128_t expect = ref(a, b);
    block.Run(
        ins,
        [&](cpu::ppc::PPCContext* ctx) {
          ctx->v[kVa] = a;
          ctx->v[kVb] = b;
        },
        [&](cpu::ppc::PPCContext* ctx) {
          if (ctx->v[kVd] != expect) {
            ++bad;
            if (bad <= 32) {
              LogDivergence(name, a, b, expect, ctx->v[kVd]);
            }
          }
        });
  }
  return bad;
}

uint32_t RunVx128(cpu::testing::TestGuestPpcBlock& block, uint32_t iters,
                  std::mt19937& rng, const char* name, uint32_t insn,
                  vec128_t (*ref)(const vec128_t&, const vec128_t&)) {
  const std::vector<uint32_t> ins = {PatchVx128(insn, kVd, kVa, kVb),
                                     kPpcInsnBlr};
  uint32_t bad = 0;
  for (uint32_t i = 0; i < iters; ++i) {
    vec128_t a = RandomVec(rng);
    vec128_t b = RandomVec(rng);
    vec128_t expect = ref(a, b);
    block.Run(
        ins,
        [&](cpu::ppc::PPCContext* ctx) {
          ctx->v[kVa] = a;
          ctx->v[kVb] = b;
        },
        [&](cpu::ppc::PPCContext* ctx) {
          if (ctx->v[kVd] != expect) {
            ++bad;
            if (bad <= 32) {
              LogDivergence(name, a, b, expect, ctx->v[kVd]);
            }
          }
        });
  }
  return bad;
}

static vec128_t Ref_bitand(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 4; ++j) {
    o.u32[j] = a.u32[j] & b.u32[j];
  }
  return o;
}
static vec128_t Ref_bitc(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 4; ++j) {
    o.u32[j] = a.u32[j] & ~b.u32[j];
  }
  return o;
}
static vec128_t Ref_bitor(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 4; ++j) {
    o.u32[j] = a.u32[j] | b.u32[j];
  }
  return o;
}
static vec128_t Ref_bitnor(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 4; ++j) {
    o.u32[j] = ~(a.u32[j] | b.u32[j]);
  }
  return o;
}
static vec128_t Ref_bitxor(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 4; ++j) {
    o.u32[j] = a.u32[j] ^ b.u32[j];
  }
  return o;
}

static vec128_t Ref_vmaxsb(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 16; ++j) {
    o.i8[j] = std::max(int8_t(a.u8[j]), int8_t(b.u8[j]));
  }
  return o;
}
static vec128_t Ref_vmaxsh(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 8; ++j) {
    o.i16[j] = std::max(int16_t(a.u16[j]), int16_t(b.u16[j]));
  }
  return o;
}
static vec128_t Ref_vmaxsw(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 4; ++j) {
    o.i32[j] = std::max(int32_t(a.u32[j]), int32_t(b.u32[j]));
  }
  return o;
}
static vec128_t Ref_vmaxub(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 16; ++j) {
    o.u8[j] = std::max(a.u8[j], b.u8[j]);
  }
  return o;
}
static vec128_t Ref_vmaxuh(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 8; ++j) {
    o.u16[j] = std::max(a.u16[j], b.u16[j]);
  }
  return o;
}
static vec128_t Ref_vmaxuw(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 4; ++j) {
    o.u32[j] = std::max(a.u32[j], b.u32[j]);
  }
  return o;
}

static vec128_t Ref_vminsb(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 16; ++j) {
    o.i8[j] = std::min(int8_t(a.u8[j]), int8_t(b.u8[j]));
  }
  return o;
}
static vec128_t Ref_vminsh(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 8; ++j) {
    o.i16[j] = std::min(int16_t(a.u16[j]), int16_t(b.u16[j]));
  }
  return o;
}
static vec128_t Ref_vminsw(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 4; ++j) {
    o.i32[j] = std::min(int32_t(a.u32[j]), int32_t(b.u32[j]));
  }
  return o;
}
static vec128_t Ref_vminub(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 16; ++j) {
    o.u8[j] = std::min(a.u8[j], b.u8[j]);
  }
  return o;
}
static vec128_t Ref_vminuh(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 8; ++j) {
    o.u16[j] = std::min(a.u16[j], b.u16[j]);
  }
  return o;
}
static vec128_t Ref_vminuw(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 4; ++j) {
    o.u32[j] = std::min(a.u32[j], b.u32[j]);
  }
  return o;
}

static vec128_t Ref_vavgsb(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 16; ++j) {
    o.i8[j] = static_cast<int8_t>(
        (int16_t(int8_t(a.u8[j])) + int16_t(int8_t(b.u8[j])) + 1) >> 1);
  }
  return o;
}
static vec128_t Ref_vavgsh(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 8; ++j) {
    o.i16[j] = static_cast<int16_t>(
        (int32_t(int16_t(a.u16[j])) + int32_t(int16_t(b.u16[j])) + 1) >> 1);
  }
  return o;
}
static vec128_t Ref_vavgsw(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 4; ++j) {
    int64_t s = int64_t(int32_t(a.u32[j])) + int64_t(int32_t(b.u32[j]));
    o.i32[j] = static_cast<int32_t>((s + 1) >> 1);
  }
  return o;
}
static vec128_t Ref_vavgub(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 16; ++j) {
    o.u8[j] = static_cast<uint8_t>(
        (uint16_t(a.u8[j]) + uint16_t(b.u8[j]) + 1) >> 1);
  }
  return o;
}
static vec128_t Ref_vavguh(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 8; ++j) {
    o.u16[j] = static_cast<uint16_t>(
        (uint32_t(a.u16[j]) + uint32_t(b.u16[j]) + 1) >> 1);
  }
  return o;
}
static vec128_t Ref_vavguw(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 4; ++j) {
    o.u32[j] = static_cast<uint32_t>(
        (uint64_t(a.u32[j]) + uint64_t(b.u32[j]) + 1) >> 1);
  }
  return o;
}

static vec128_t Ref_vaddcuw(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 4; ++j) {
    uint64_t s = uint64_t(a.u32[j]) + uint64_t(b.u32[j]);
    o.u32[j] = static_cast<uint32_t>(s >> 32);
  }
  return o;
}
static vec128_t Ref_vsubcuw(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 4; ++j) {
    // 1 if no unsigned borrow (VA >= VB), else 0 — matches ppc_emit_altivec
    // VectorCompareUGE + shr 31.
    o.u32[j] = (a.u32[j] >= b.u32[j]) ? 1u : 0u;
  }
  return o;
}

#define RUN_VX(name, insn, ref)                                                \
  static uint32_t Run_##name(cpu::testing::TestGuestPpcBlock& b, uint32_t it,   \
                             std::mt19937& rng) noexcept {                      \
    return RunVx(b, it, rng, #name, (insn), (ref));                            \
  }
#define RUN_VX128(name, insn, ref)                                             \
  static uint32_t Run_##name(cpu::testing::TestGuestPpcBlock& b, uint32_t it,   \
                             std::mt19937& rng) noexcept {                      \
    return RunVx128(b, it, rng, #name, (insn), (ref));                         \
  }

RUN_VX(vand, 0x10000404, Ref_bitand)
RUN_VX128(vand128, 0x14000210, Ref_bitand)
RUN_VX(vandc, 0x10000444, Ref_bitc)
RUN_VX128(vandc128, 0x14000250, Ref_bitc)
RUN_VX(vor, 0x10000484, Ref_bitor)
RUN_VX128(vor128, 0x140002d0, Ref_bitor)
RUN_VX(vnor, 0x10000504, Ref_bitnor)
RUN_VX128(vnor128, 0x14000290, Ref_bitnor)
RUN_VX(vxor, 0x100004c4, Ref_bitxor)
RUN_VX128(vxor128, 0x14000310, Ref_bitxor)

RUN_VX(vmaxsb, 0x10000102, Ref_vmaxsb)
RUN_VX(vmaxsh, 0x10000142, Ref_vmaxsh)
RUN_VX(vmaxsw, 0x10000182, Ref_vmaxsw)
RUN_VX(vmaxub, 0x10000002, Ref_vmaxub)
RUN_VX(vmaxuh, 0x10000042, Ref_vmaxuh)
RUN_VX(vmaxuw, 0x10000082, Ref_vmaxuw)
RUN_VX(vminsb, 0x10000302, Ref_vminsb)
RUN_VX(vminsh, 0x10000342, Ref_vminsh)
RUN_VX(vminsw, 0x10000382, Ref_vminsw)
RUN_VX(vminub, 0x10000202, Ref_vminub)
RUN_VX(vminuh, 0x10000242, Ref_vminuh)
RUN_VX(vminuw, 0x10000282, Ref_vminuw)
RUN_VX(vavgsb, 0x10000502, Ref_vavgsb)
RUN_VX(vavgsh, 0x10000542, Ref_vavgsh)
RUN_VX(vavgsw, 0x10000582, Ref_vavgsw)
RUN_VX(vavgub, 0x10000402, Ref_vavgub)
RUN_VX(vavguh, 0x10000442, Ref_vavguh)
RUN_VX(vavguw, 0x10000482, Ref_vavguw)
RUN_VX(vaddcuw, 0x10000180, Ref_vaddcuw)
RUN_VX(vsubcuw, 0x10000580, Ref_vsubcuw)

#undef RUN_VX
#undef RUN_VX128

}  // namespace

void RegisterLogicalMinmaxAvg() {
  auto& v = FuzzCases();
  v.push_back({"vaddcuw", Run_vaddcuw});
  v.push_back({"vand", Run_vand});
  v.push_back({"vand128", Run_vand128});
  v.push_back({"vandc", Run_vandc});
  v.push_back({"vandc128", Run_vandc128});
  v.push_back({"vavgsb", Run_vavgsb});
  v.push_back({"vavgsh", Run_vavgsh});
  v.push_back({"vavgsw", Run_vavgsw});
  v.push_back({"vavgub", Run_vavgub});
  v.push_back({"vavguh", Run_vavguh});
  v.push_back({"vavguw", Run_vavguw});
  v.push_back({"vmaxsb", Run_vmaxsb});
  v.push_back({"vmaxsh", Run_vmaxsh});
  v.push_back({"vmaxsw", Run_vmaxsw});
  v.push_back({"vmaxub", Run_vmaxub});
  v.push_back({"vmaxuh", Run_vmaxuh});
  v.push_back({"vmaxuw", Run_vmaxuw});
  v.push_back({"vminsb", Run_vminsb});
  v.push_back({"vminsh", Run_vminsh});
  v.push_back({"vminsw", Run_vminsw});
  v.push_back({"vminub", Run_vminub});
  v.push_back({"vminuh", Run_vminuh});
  v.push_back({"vminuw", Run_vminuw});
  v.push_back({"vnor", Run_vnor});
  v.push_back({"vnor128", Run_vnor128});
  v.push_back({"vor", Run_vor});
  v.push_back({"vor128", Run_vor128});
  v.push_back({"vsubcuw", Run_vsubcuw});
  v.push_back({"vxor", Run_vxor});
  v.push_back({"vxor128", Run_vxor128});
}

}  // namespace xe::vmx128_fuzz
