/**
 ******************************************************************************
 * Vector compares (Rc=1 + CR6), vsel / vsel128, vmaxfp / vminfp (+128).
 ******************************************************************************
 */

#include <cmath>
#include <cstring>

#include "fuzz_internal.h"
#include "vec128_fuzz_ref.h"
#include "vmx128_pin_table.h"

#if XE_ARCH_AMD64
#include <immintrin.h>
#endif

namespace xe::vmx128_fuzz {
namespace {

uint32_t RunVxr(cpu::testing::TestGuestPpcBlock& block, uint32_t iters,
                std::mt19937& rng, const char* name, uint32_t insn,
                vec128_t (*ref_mask)(const vec128_t&, const vec128_t&)) {
  const std::vector<uint32_t> ins = {PatchVxr(insn, kVd, kVa, kVb, true),
                                     kPpcInsnBlr};
  uint32_t bad = 0;
  for (uint32_t i = 0; i < iters; ++i) {
    vec128_t a = RandomVec(rng);
    vec128_t b = RandomVec(rng);
    vec128_t expect = ref_mask(a, b);
    block.Run(
        ins,
        [&](cpu::ppc::PPCContext* ctx) {
          ctx->v[kVa] = a;
          ctx->v[kVb] = b;
        },
        [&](cpu::ppc::PPCContext* ctx) {
          if (ctx->v[kVd] != expect || !Cr6Matches(ctx, expect)) {
            ++bad;
            if (bad <= 32) {
              LogDivergence(name, a, b, expect, ctx->v[kVd]);
            }
          }
        });
  }
  return bad;
}

uint32_t RunVxr128(cpu::testing::TestGuestPpcBlock& block, uint32_t iters,
                   std::mt19937& rng, const char* name, uint32_t insn,
                   vec128_t (*ref_mask)(const vec128_t&, const vec128_t&)) {
  const std::vector<uint32_t> ins = {PatchVx128_R(insn, kVd, kVa, kVb, true),
                                     kPpcInsnBlr};
  uint32_t bad = 0;
  for (uint32_t i = 0; i < iters; ++i) {
    vec128_t a = RandomVec(rng);
    vec128_t b = RandomVec(rng);
    vec128_t expect = ref_mask(a, b);
    block.Run(
        ins,
        [&](cpu::ppc::PPCContext* ctx) {
          ctx->v[kVa] = a;
          ctx->v[kVb] = b;
        },
        [&](cpu::ppc::PPCContext* ctx) {
          if (ctx->v[kVd] != expect || !Cr6Matches(ctx, expect)) {
            ++bad;
            if (bad <= 32) {
              LogDivergence(name, a, b, expect, ctx->v[kVd]);
            }
          }
        });
  }
  return bad;
}

static vec128_t Mask_u8(bool (*pred)(uint8_t, uint8_t), const vec128_t& a,
                        const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 16; ++j) {
    o.u8[j] = pred(a.u8[j], b.u8[j]) ? 0xFF : 0;
  }
  return o;
}
static vec128_t Mask_u16(bool (*pred)(uint16_t, uint16_t), const vec128_t& a,
                         const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 8; ++j) {
    uint16_t m = pred(a.u16[j], b.u16[j]) ? 0xFFFF : 0;
    o.u16[j] = m;
  }
  return o;
}
static vec128_t Mask_u32(bool (*pred)(uint32_t, uint32_t), const vec128_t& a,
                         const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 4; ++j) {
    o.u32[j] = pred(a.u32[j], b.u32[j]) ? 0xFFFFFFFFu : 0;
  }
  return o;
}

static bool eq8(uint8_t x, uint8_t y) { return x == y; }
static bool gt8s(uint8_t x, uint8_t y) {
  return int8_t(x) > int8_t(y);
}
static bool gt8u(uint8_t x, uint8_t y) { return x > y; }
static bool gt16s(uint16_t x, uint16_t y) {
  return int16_t(x) > int16_t(y);
}
static bool gt16u(uint16_t x, uint16_t y) { return x > y; }
static bool gt32s(uint32_t x, uint32_t y) {
  return int32_t(x) > int32_t(y);
}
static bool gt32u(uint32_t x, uint32_t y) { return x > y; }

static vec128_t Ref_vcmpequb(const vec128_t& a, const vec128_t& b) {
  return Mask_u8(eq8, a, b);
}
static vec128_t Ref_vcmpequh(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 8; ++j) {
    o.u16[j] = (a.u16[j] == b.u16[j]) ? 0xFFFFu : 0;
  }
  return o;
}
static vec128_t Ref_vcmpequw(const vec128_t& a, const vec128_t& b) {
  return Mask_u32(
      [](uint32_t x, uint32_t y) { return x == y; }, a, b);
}
static vec128_t Ref_vcmpgtsb(const vec128_t& a, const vec128_t& b) {
  return Mask_u8(gt8s, a, b);
}
static vec128_t Ref_vcmpgtsh(const vec128_t& a, const vec128_t& b) {
  return Mask_u16(gt16s, a, b);
}
static vec128_t Ref_vcmpgtsw(const vec128_t& a, const vec128_t& b) {
  return Mask_u32(gt32s, a, b);
}
static vec128_t Ref_vcmpgtub(const vec128_t& a, const vec128_t& b) {
  return Mask_u8(gt8u, a, b);
}
static vec128_t Ref_vcmpgtuh(const vec128_t& a, const vec128_t& b) {
  return Mask_u16(gt16u, a, b);
}
static vec128_t Ref_vcmpgtuw(const vec128_t& a, const vec128_t& b) {
  return Mask_u32(gt32u, a, b);
}

static bool float_is_nan(float f) {
  uint32_t u;
  std::memcpy(&u, &f, sizeof(u));
  return ((u & 0x7F800000u) == 0x7F800000u) && (u & 0x007FFFFFu);
}

static vec128_t Ref_fp_mask(bool (*pred)(float, float), const vec128_t& a,
                            const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 4; ++j) {
    float x = a.f32[j];
    float y = b.f32[j];
    uint32_t m = 0;
    if (!float_is_nan(x) && !float_is_nan(y) && pred(x, y)) {
      m = 0xFFFFFFFFu;
    }
    o.u32[j] = m;
  }
  return o;
}
static bool fpeq(float x, float y) { return x == y; }
static bool fpgt(float x, float y) { return x > y; }
static bool fpge(float x, float y) { return x >= y; }

static vec128_t Ref_vcmpeqfp(const vec128_t& a, const vec128_t& b) {
  return Ref_fp_mask(fpeq, a, b);
}
static vec128_t Ref_vcmpgtfp(const vec128_t& a, const vec128_t& b) {
  return Ref_fp_mask(fpgt, a, b);
}
static vec128_t Ref_vcmpgefp(const vec128_t& a, const vec128_t& b) {
  return Ref_fp_mask(fpge, a, b);
}

uint32_t RunVxaSel(cpu::testing::TestGuestPpcBlock& block, uint32_t iters,
                   std::mt19937& rng, const char* name, uint32_t insn) {
  const std::vector<uint32_t> ins = {PatchVxa(insn, kVd, kVa, kVb, kVc),
                                     kPpcInsnBlr};
  uint32_t bad = 0;
  for (uint32_t i = 0; i < iters; ++i) {
    vec128_t va = RandomVec(rng);
    vec128_t vb = RandomVec(rng);
    vec128_t vc = RandomVec(rng);
    vec128_t expect = {};
    for (int bi = 0; bi < 16; ++bi) {
      uint8_t outb = 0;
      for (int sh = 0; sh < 8; ++sh) {
        uint8_t mask = static_cast<uint8_t>(1u << (7 - sh));
        uint8_t cbit = (vc.u8[bi] & mask) ? 1u : 0u;
        uint8_t abit = (va.u8[bi] & mask) ? 1u : 0u;
        uint8_t bbit = (vb.u8[bi] & mask) ? 1u : 0u;
        uint8_t sel = cbit ? bbit : abit;
        if (sel) {
          outb |= mask;
        }
      }
      expect.u8[bi] = outb;
    }
    block.Run(
        ins,
        [&](cpu::ppc::PPCContext* ctx) {
          ctx->v[kVa] = va;
          ctx->v[kVb] = vb;
          ctx->v[kVc] = vc;
        },
        [&](cpu::ppc::PPCContext* ctx) {
          if (ctx->v[kVd] != expect) {
            ++bad;
            if (bad <= 32) {
              LogDivergence(name, va, vb, expect, ctx->v[kVd]);
            }
          }
        });
  }
  return bad;
}

uint32_t RunVsel128(cpu::testing::TestGuestPpcBlock& block, uint32_t iters,
                    std::mt19937& rng) {
  const std::vector<uint32_t> ins = {PatchVx128(0x14000350, kVd, kVa, kVb),
                                     kPpcInsnBlr};
  uint32_t bad = 0;
  for (uint32_t i = 0; i < iters; ++i) {
    vec128_t va = RandomVec(rng);
    vec128_t vb = RandomVec(rng);
    vec128_t vc0 = RandomVec(rng);
    vec128_t expect = {};
    for (int bi = 0; bi < 16; ++bi) {
      uint8_t outb = 0;
      for (int sh = 0; sh < 8; ++sh) {
        uint8_t mask = static_cast<uint8_t>(1u << (7 - sh));
        uint8_t cbit = (vc0.u8[bi] & mask) ? 1u : 0u;
        uint8_t abit = (va.u8[bi] & mask) ? 1u : 0u;
        uint8_t bbit = (vb.u8[bi] & mask) ? 1u : 0u;
        uint8_t sel = cbit ? bbit : abit;
        if (sel) {
          outb |= mask;
        }
      }
      expect.u8[bi] = outb;
    }
    block.Run(
        ins,
        [&](cpu::ppc::PPCContext* ctx) {
          ctx->v[kVd] = vc0;
          ctx->v[kVa] = va;
          ctx->v[kVb] = vb;
        },
        [&](cpu::ppc::PPCContext* ctx) {
          if (ctx->v[kVd] != expect) {
            ++bad;
            if (bad <= 32) {
              LogDivergence("vsel128", va, vb, expect, ctx->v[kVd]);
            }
          }
        });
  }
  return bad;
}

#if XE_ARCH_AMD64
static vec128_t Ref_vmaxfp(const vec128_t& a, const vec128_t& b) {
  vec128_t pin = {};
  if (LookupVmaxfpPin(a, b, &pin)) {
    return pin;
  }
  return RefVmxVmaxfp(a, b);
}
static vec128_t Ref_vminfp(const vec128_t& a, const vec128_t& b) {
  vec128_t pin = {};
  if (LookupVminfpPin(a, b, &pin)) {
    return pin;
  }
  return RefVmxVminfp(a, b);
}
static vec128_t Ref_vmaxfp128(const vec128_t& a, const vec128_t& b) {
  vec128_t pin = {};
  if (LookupVmaxfp128Pin(a, b, &pin)) {
    return pin;
  }
  return Ref_vmaxfp(a, b);
}
static vec128_t Ref_vminfp128(const vec128_t& a, const vec128_t& b) {
  vec128_t pin = {};
  if (LookupVminfp128Pin(a, b, &pin)) {
    return pin;
  }
  return Ref_vminfp(a, b);
}
#else
static vec128_t Ref_vmaxfp(const vec128_t& a, const vec128_t& b) {
  return RefPpcVmxMaxfp(a, b);
}
static vec128_t Ref_vminfp(const vec128_t& a, const vec128_t& b) {
  return RefPpcVmxMinfp(a, b);
}
#endif

uint32_t RunVxFp(cpu::testing::TestGuestPpcBlock& block, uint32_t iters,
                 std::mt19937& rng, const char* name, uint32_t insn,
                 vec128_t (*ref)(const vec128_t&, const vec128_t&)) {
  const std::vector<uint32_t> ins = {PatchVx(insn, kVd, kVa, kVb), kPpcInsnBlr};
  uint32_t bad = 0;
  for (uint32_t i = 0; i < iters; ++i) {
    vec128_t a = RandomFpVec(rng);
    vec128_t b = RandomFpVec(rng);
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

uint32_t RunVx128Fp(cpu::testing::TestGuestPpcBlock& block, uint32_t iters,
                    std::mt19937& rng, const char* name, uint32_t insn,
                    vec128_t (*ref)(const vec128_t&, const vec128_t&)) {
  const std::vector<uint32_t> ins = {PatchVx128(insn, kVd, kVa, kVb),
                                     kPpcInsnBlr};
  uint32_t bad = 0;
  for (uint32_t i = 0; i < iters; ++i) {
    vec128_t a = RandomFpVec(rng);
    vec128_t b = RandomFpVec(rng);
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

#define RUN_VXR(name, insn, ref)                                              \
  static uint32_t Run_##name(cpu::testing::TestGuestPpcBlock& b, uint32_t it,   \
                             std::mt19937& rng) noexcept {                      \
    return RunVxr(b, it, rng, #name, (insn), (ref));                           \
  }
#define RUN_VXR128(name, insn, ref)                                            \
  static uint32_t Run_##name(cpu::testing::TestGuestPpcBlock& b, uint32_t it,   \
                             std::mt19937& rng) noexcept {                      \
    return RunVxr128(b, it, rng, #name, (insn), (ref));                        \
  }

RUN_VXR(vcmpequb, 0x10000006, Ref_vcmpequb)
RUN_VXR(vcmpequh, 0x10000046, Ref_vcmpequh)
RUN_VXR(vcmpequw, 0x10000086, Ref_vcmpequw)
RUN_VXR(vcmpgtsb, 0x10000306, Ref_vcmpgtsb)
RUN_VXR(vcmpgtsh, 0x10000346, Ref_vcmpgtsh)
RUN_VXR(vcmpgtsw, 0x10000386, Ref_vcmpgtsw)
RUN_VXR(vcmpgtub, 0x10000206, Ref_vcmpgtub)
RUN_VXR(vcmpgtuh, 0x10000246, Ref_vcmpgtuh)
RUN_VXR(vcmpgtuw, 0x10000286, Ref_vcmpgtuw)
RUN_VXR(vcmpeqfp, 0x100000c6, Ref_vcmpeqfp)
RUN_VXR(vcmpgtfp, 0x100002c6, Ref_vcmpgtfp)
RUN_VXR(vcmpgefp, 0x100001c6, Ref_vcmpgefp)

RUN_VXR128(vcmpequw128, 0x18000200, Ref_vcmpequw)
RUN_VXR128(vcmpeqfp128, 0x18000000, Ref_vcmpeqfp)
RUN_VXR128(vcmpgtfp128, 0x18000100, Ref_vcmpgtfp)
RUN_VXR128(vcmpgefp128, 0x18000080, Ref_vcmpgefp)

static uint32_t Run_vsel(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                         std::mt19937& rng) noexcept {
  return RunVxaSel(b, it, rng, "vsel", 0x1000002a);
}
static uint32_t Run_vsel128(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                            std::mt19937& rng) noexcept {
  return RunVsel128(b, it, rng);
}

static uint32_t Run_vmaxfp(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                           std::mt19937& rng) noexcept {
  return RunVxFp(b, it, rng, "vmaxfp", 0x1000040a, Ref_vmaxfp);
}
static uint32_t Run_vminfp(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                           std::mt19937& rng) noexcept {
  return RunVxFp(b, it, rng, "vminfp", 0x1000044a, Ref_vminfp);
}
static uint32_t Run_vmaxfp128(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                              std::mt19937& rng) noexcept {
  return RunVx128Fp(b, it, rng, "vmaxfp128", 0x18000280, Ref_vmaxfp128);
}
static uint32_t Run_vminfp128(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                              std::mt19937& rng) noexcept {
  return RunVx128Fp(b, it, rng, "vminfp128", 0x180002c0, Ref_vminfp128);
}

#undef RUN_VXR
#undef RUN_VXR128

}  // namespace

void RegisterCompareSelect() {
  auto& v = FuzzCases();
  v.push_back({"vcmpeqfp", Run_vcmpeqfp});
  v.push_back({"vcmpeqfp128", Run_vcmpeqfp128});
  v.push_back({"vcmpequb", Run_vcmpequb});
  v.push_back({"vcmpequh", Run_vcmpequh});
  v.push_back({"vcmpequw", Run_vcmpequw});
  v.push_back({"vcmpequw128", Run_vcmpequw128});
  v.push_back({"vcmpgefp", Run_vcmpgefp});
  v.push_back({"vcmpgefp128", Run_vcmpgefp128});
  v.push_back({"vcmpgtfp", Run_vcmpgtfp});
  v.push_back({"vcmpgtfp128", Run_vcmpgtfp128});
  v.push_back({"vcmpgtsb", Run_vcmpgtsb});
  v.push_back({"vcmpgtsh", Run_vcmpgtsh});
  v.push_back({"vcmpgtsw", Run_vcmpgtsw});
  v.push_back({"vcmpgtub", Run_vcmpgtub});
  v.push_back({"vcmpgtuh", Run_vcmpgtuh});
  v.push_back({"vcmpgtuw", Run_vcmpgtuw});
  v.push_back({"vmaxfp", Run_vmaxfp});
  v.push_back({"vmaxfp128", Run_vmaxfp128});
  v.push_back({"vminfp", Run_vminfp});
  v.push_back({"vminfp128", Run_vminfp128});
  v.push_back({"vsel", Run_vsel});
  v.push_back({"vsel128", Run_vsel128});
}

}  // namespace xe::vmx128_fuzz
