/**
 ******************************************************************************
 * Multiply / sum family: vmhadd*, vmladduhm, vmsum*, vmule*, vmulo*,
 * vmulfp128.
 ******************************************************************************
 */

#include <cstdint>

#include "fuzz_internal.h"
#include "vec128_fuzz_ref.h"
#include "vmx128_pin_table.h"

#if XE_ARCH_AMD64
#include <immintrin.h>
#endif

namespace xe::vmx128_fuzz {
namespace {

static inline int16_t sat_i16_i32(int32_t t) {
  if (t > 32767) {
    return 32767;
  }
  if (t < -32768) {
    return -32768;
  }
  return static_cast<int16_t>(t);
}

static inline int32_t sat_i32_i64(int64_t t) {
  if (t > INT32_MAX) {
    return INT32_MAX;
  }
  if (t < INT32_MIN) {
    return INT32_MIN;
  }
  return static_cast<int32_t>(t);
}

static inline uint32_t sat_u32_u64(uint64_t t) {
  if (t > UINT32_MAX) {
    return UINT32_MAX;
  }
  return static_cast<uint32_t>(t);
}

static vec128_t Ref_vmhaddshs(const vec128_t& va, const vec128_t& vb,
                              const vec128_t& vc) {
  vec128_t o = {};
  for (int lane = 0; lane < 8; ++lane) {
    const int xi = lane ^ 1;
    const int32_t prod =
        static_cast<int32_t>(va.i16[xi]) * static_cast<int32_t>(vb.i16[xi]);
    const int32_t t = (prod >> 15) + static_cast<int32_t>(vc.i16[xi]);
    o.i16[xi] = sat_i16_i32(t);
  }
  return o;
}

static vec128_t Ref_vmhraddshs(const vec128_t& va, const vec128_t& vb,
                               const vec128_t& vc) {
  vec128_t o = {};
  for (int lane = 0; lane < 8; ++lane) {
    const int xi = lane ^ 1;
    const int32_t prod =
        static_cast<int32_t>(va.i16[xi]) * static_cast<int32_t>(vb.i16[xi]) +
        0x4000;
    const int32_t t = (prod >> 15) + static_cast<int32_t>(vc.i16[xi]);
    o.i16[xi] = sat_i16_i32(t);
  }
  return o;
}

static vec128_t Ref_vmladduhm(const vec128_t& va, const vec128_t& vb,
                              const vec128_t& vc) {
  vec128_t o = {};
  for (int lane = 0; lane < 8; ++lane) {
    const int xi = lane ^ 1;
    const int32_t prod = static_cast<int32_t>(va.u16[xi]) *
                         static_cast<int32_t>(vb.u16[xi]);
    o.i16[xi] = static_cast<int16_t>(prod + static_cast<int32_t>(vc.i16[xi]));
  }
  return o;
}

static vec128_t Ref_vmsummbm(const vec128_t& va, const vec128_t& vb,
                             const vec128_t& vc) {
  vec128_t o = {};
  for (int W = 0; W < 4; ++W) {
    int64_t t = vc.i32[W];
    for (int j = 0; j < 4; ++j) {
      const int be = FuzzVec128B(static_cast<uint8_t>(4 * W + j));
      t += static_cast<int32_t>(static_cast<int8_t>(va.u8[be])) *
           static_cast<int32_t>(vb.u8[be] & 0xFFu);
    }
    o.i32[W] = static_cast<int32_t>(t);
  }
  return o;
}

static inline int16_t WordHalfI16(const vec128_t& v, int w, bool high) {
  const uint32_t word = v.u32[w];
  return static_cast<int16_t>(high ? (word >> 16) : (word & 0xFFFFu));
}

static vec128_t Ref_vmsumshm(const vec128_t& va, const vec128_t& vb,
                             const vec128_t& vc) {
  vec128_t o = {};
  for (int W = 0; W < 4; ++W) {
    int64_t t = vc.i32[W];
    t += static_cast<int32_t>(WordHalfI16(va, W, true)) *
         static_cast<int32_t>(WordHalfI16(vb, W, true));
    t += static_cast<int32_t>(WordHalfI16(va, W, false)) *
         static_cast<int32_t>(WordHalfI16(vb, W, false));
    o.i32[W] = static_cast<int32_t>(t);
  }
  return o;
}

static vec128_t Ref_vmsumshs(const vec128_t& va, const vec128_t& vb,
                             const vec128_t& vc) {
  vec128_t o = {};
  for (int W = 0; W < 4; ++W) {
    int64_t t = vc.i32[W];
    t += static_cast<int32_t>(WordHalfI16(va, W, true)) *
         static_cast<int32_t>(WordHalfI16(vb, W, true));
    t += static_cast<int32_t>(WordHalfI16(va, W, false)) *
         static_cast<int32_t>(WordHalfI16(vb, W, false));
    o.i32[W] = sat_i32_i64(t);
  }
  return o;
}

static vec128_t Ref_vmsumubm(const vec128_t& va, const vec128_t& vb,
                             const vec128_t& vc) {
  vec128_t o = {};
  for (int W = 0; W < 4; ++W) {
    uint64_t t = vc.u32[W];
    for (int j = 0; j < 4; ++j) {
      const int be = FuzzVec128B(static_cast<uint8_t>(4 * W + j));
      t += static_cast<uint64_t>(va.u8[be]) *
           static_cast<uint64_t>(vb.u8[be]);
    }
    o.u32[W] = static_cast<uint32_t>(t);
  }
  return o;
}

static inline uint16_t WordHalfU16(const vec128_t& v, int w, bool high) {
  const uint32_t word = v.u32[w];
  return static_cast<uint16_t>(high ? (word >> 16) : (word & 0xFFFFu));
}

static vec128_t Ref_vmsumuhm(const vec128_t& va, const vec128_t& vb,
                             const vec128_t& vc) {
  vec128_t o = {};
  for (int W = 0; W < 4; ++W) {
    uint64_t t = vc.u32[W];
    t += static_cast<uint64_t>(WordHalfU16(va, W, true)) *
         static_cast<uint64_t>(WordHalfU16(vb, W, true));
    t += static_cast<uint64_t>(WordHalfU16(va, W, false)) *
         static_cast<uint64_t>(WordHalfU16(vb, W, false));
    o.u32[W] = static_cast<uint32_t>(t);
  }
  return o;
}

static vec128_t Ref_vmsumuhs(const vec128_t& va, const vec128_t& vb,
                             const vec128_t& vc) {
  vec128_t o = {};
  for (int W = 0; W < 4; ++W) {
    uint64_t t = vc.u32[W];
    t += static_cast<uint64_t>(WordHalfU16(va, W, true)) *
         static_cast<uint64_t>(WordHalfU16(vb, W, true));
    t += static_cast<uint64_t>(WordHalfU16(va, W, false)) *
         static_cast<uint64_t>(WordHalfU16(vb, W, false));
    o.u32[W] = sat_u32_u64(t);
  }
  return o;
}

static vec128_t Ref_vmulesb(const vec128_t& va, const vec128_t& vb) {
  vec128_t o = {};
  for (int H = 0; H < 8; ++H) {
    const int bi = (2 * H) ^ 3;
    const int32_t pr = static_cast<int32_t>(static_cast<int8_t>(va.u8[bi])) *
                       static_cast<int32_t>(static_cast<int8_t>(vb.u8[bi]));
    o.i16[H ^ 1] = static_cast<int16_t>(pr);
  }
  return o;
}

static vec128_t Ref_vmulesh(const vec128_t& va, const vec128_t& vb) {
  vec128_t o = {};
  for (int W = 0; W < 4; ++W) {
    const int hi = 2 * W;
    const int xi = hi ^ 1;
    o.i32[W] = static_cast<int32_t>(va.i16[xi]) *
               static_cast<int32_t>(vb.i16[xi]);
  }
  return o;
}

static vec128_t Ref_vmuleub(const vec128_t& va, const vec128_t& vb) {
  vec128_t o = {};
  for (int H = 0; H < 8; ++H) {
    const int bi = (2 * H) ^ 3;
    const uint32_t pr =
        static_cast<uint32_t>(va.u8[bi]) * static_cast<uint32_t>(vb.u8[bi]);
    o.i16[H ^ 1] = static_cast<int16_t>(pr);
  }
  return o;
}

static vec128_t Ref_vmuleuh(const vec128_t& va, const vec128_t& vb) {
  vec128_t o = {};
  for (int W = 0; W < 4; ++W) {
    const int hi = 2 * W;
    const int xi = hi ^ 1;
    o.u32[W] = static_cast<uint32_t>(va.u16[xi]) *
               static_cast<uint32_t>(vb.u16[xi]);
  }
  return o;
}

static vec128_t Ref_vmulosb(const vec128_t& va, const vec128_t& vb) {
  vec128_t o = {};
  for (int H = 0; H < 8; ++H) {
    const int bi = (2 * H + 1) ^ 3;
    const int32_t pr = static_cast<int32_t>(static_cast<int8_t>(va.u8[bi])) *
                       static_cast<int32_t>(static_cast<int8_t>(vb.u8[bi]));
    o.i16[H ^ 1] = static_cast<int16_t>(pr);
  }
  return o;
}

static vec128_t Ref_vmulosh(const vec128_t& va, const vec128_t& vb) {
  vec128_t o = {};
  for (int W = 0; W < 4; ++W) {
    const int hi = 2 * W + 1;
    const int xi = hi ^ 1;
    o.i32[W] = static_cast<int32_t>(va.i16[xi]) *
               static_cast<int32_t>(vb.i16[xi]);
  }
  return o;
}

static vec128_t Ref_vmuloub(const vec128_t& va, const vec128_t& vb) {
  vec128_t o = {};
  for (int H = 0; H < 8; ++H) {
    const int bi = (2 * H + 1) ^ 3;
    const uint32_t pr =
        static_cast<uint32_t>(va.u8[bi]) * static_cast<uint32_t>(vb.u8[bi]);
    o.i16[H ^ 1] = static_cast<int16_t>(pr);
  }
  return o;
}

static vec128_t Ref_vmulouh(const vec128_t& va, const vec128_t& vb) {
  vec128_t o = {};
  for (int W = 0; W < 4; ++W) {
    const int hi = 2 * W + 1;
    const int xi = hi ^ 1;
    o.u32[W] = static_cast<uint32_t>(va.u16[xi]) *
               static_cast<uint32_t>(vb.u16[xi]);
  }
  return o;
}

static vec128_t Ref_vmulfp128(const vec128_t& a, const vec128_t& b) {
  vec128_t pin = {};
  if (LookupVmulfp128Pin(a, b, &pin)) {
    return pin;
  }
#if XE_ARCH_AMD64
  __m128 ma = _mm_load_ps(a.f32);
  __m128 mb = _mm_load_ps(b.f32);
  __m128 r = _mm_mul_ps(ma, mb);
  vec128_t o;
  _mm_store_ps(o.f32, r);
  return o;
#else
  vec128_t o = {};
  for (int i = 0; i < 4; ++i) {
    o.f32[i] = a.f32[i] * b.f32[i];
  }
  return o;
#endif
}

uint32_t RunVx128(cpu::testing::TestGuestPpcBlock& block, uint32_t iters,
                  std::mt19937& rng, const char* name, uint32_t insn,
                  vec128_t (*ref)(const vec128_t&, const vec128_t&)) {
  const std::vector<uint32_t> ins = {PatchVx128(insn, kVd, kVa, kVb),
                                     kPpcInsnBlr};
  uint32_t bad = 0;
  for (uint32_t i = 0; i < iters; ++i) {
    vec128_t va = RandomFpVec(rng);
    vec128_t vb = RandomFpVec(rng);
    vec128_t expect = ref(va, vb);
    block.Run(
        ins,
        [&](cpu::ppc::PPCContext* ctx) {
          ctx->v[kVa] = va;
          ctx->v[kVb] = vb;
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

uint32_t RunVx(cpu::testing::TestGuestPpcBlock& block, uint32_t iters,
               std::mt19937& rng, const char* name, uint32_t insn,
               vec128_t (*ref)(const vec128_t&, const vec128_t&)) {
  const std::vector<uint32_t> ins = {PatchVx(insn, kVd, kVa, kVb),
                                     kPpcInsnBlr};
  uint32_t bad = 0;
  for (uint32_t i = 0; i < iters; ++i) {
    vec128_t va = RandomVec(rng);
    vec128_t vb = RandomVec(rng);
    vec128_t expect = ref(va, vb);
    block.Run(
        ins,
        [&](cpu::ppc::PPCContext* ctx) {
          ctx->v[kVa] = va;
          ctx->v[kVb] = vb;
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

uint32_t RunVxa(cpu::testing::TestGuestPpcBlock& block, uint32_t iters,
                std::mt19937& rng, const char* name, uint32_t insn,
                vec128_t (*ref)(const vec128_t&, const vec128_t&,
                                const vec128_t&)) {
  const std::vector<uint32_t> ins = {PatchVxa(insn, kVd, kVa, kVb, kVc),
                                     kPpcInsnBlr};
  uint32_t bad = 0;
  for (uint32_t i = 0; i < iters; ++i) {
    vec128_t va = RandomVec(rng);
    vec128_t vb = RandomVec(rng);
    vec128_t vc = RandomVec(rng);
    vec128_t expect = ref(va, vb, vc);
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

static uint32_t Run_vmulfp128(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                              std::mt19937& rng) noexcept {
  return RunVx128(b, it, rng, "vmulfp128", 0x14000090u, Ref_vmulfp128);
}

#define RUN_VX3(name, insn_hex)                                           \
  static uint32_t Run_##name(cpu::testing::TestGuestPpcBlock& b,           \
                             uint32_t it, std::mt19937& rng) noexcept {    \
    return RunVxa(b, it, rng, #name, (insn_hex), Ref_##name);               \
  }

RUN_VX3(vmhaddshs, 0x10000020u)
RUN_VX3(vmhraddshs, 0x10000021u)
RUN_VX3(vmladduhm, 0x10000022u)
RUN_VX3(vmsummbm, 0x10000025u)
RUN_VX3(vmsumshm, 0x10000028u)
RUN_VX3(vmsumshs, 0x10000029u)
RUN_VX3(vmsumubm, 0x10000024u)
RUN_VX3(vmsumuhm, 0x10000026u)
RUN_VX3(vmsumuhs, 0x10000027u)

#undef RUN_VX3

#define RUN_VX2(name, insn_hex)                                           \
  static uint32_t Run_##name(cpu::testing::TestGuestPpcBlock& b,           \
                             uint32_t it, std::mt19937& rng) noexcept {    \
    return RunVx(b, it, rng, #name, (insn_hex), Ref_##name);               \
  }

RUN_VX2(vmulesb, 0x10000308u)
RUN_VX2(vmulesh, 0x10000348u)
RUN_VX2(vmuleub, 0x10000208u)
RUN_VX2(vmuleuh, 0x10000248u)
RUN_VX2(vmulosb, 0x10000108u)
RUN_VX2(vmulosh, 0x10000148u)
RUN_VX2(vmuloub, 0x10000008u)
RUN_VX2(vmulouh, 0x10000048u)

#undef RUN_VX2

}  // namespace

void RegisterMulSum() {
  auto& v = FuzzCases();
  v.push_back({"vmhaddshs", Run_vmhaddshs});
  v.push_back({"vmhraddshs", Run_vmhraddshs});
  v.push_back({"vmladduhm", Run_vmladduhm});
  v.push_back({"vmulfp128", Run_vmulfp128});
  v.push_back({"vmulesb", Run_vmulesb});
  v.push_back({"vmulesh", Run_vmulesh});
  v.push_back({"vmuleub", Run_vmuleub});
  v.push_back({"vmuleuh", Run_vmuleuh});
  v.push_back({"vmulosb", Run_vmulosb});
  v.push_back({"vmulosh", Run_vmulosh});
  v.push_back({"vmuloub", Run_vmuloub});
  v.push_back({"vmulouh", Run_vmulouh});
  v.push_back({"vmsummbm", Run_vmsummbm});
  v.push_back({"vmsumshm", Run_vmsumshm});
  v.push_back({"vmsumshs", Run_vmsumshs});
  v.push_back({"vmsumubm", Run_vmsumubm});
  v.push_back({"vmsumuhm", Run_vmsumuhm});
  v.push_back({"vmsumuhs", Run_vmsumuhs});
}

}  // namespace xe::vmx128_fuzz
