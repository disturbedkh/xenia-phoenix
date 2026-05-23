/**
 ******************************************************************************
 * Float round / convert / estimate (bitwise vs JIT on x64 where noted).
 ******************************************************************************
 */

#include <cmath>
#include <cstring>

#include "fuzz_internal.h"
#include "vec128_fuzz_ref.h"
#include "vmx128_pin_table.h"

#include <cstdint>

#if XE_ARCH_AMD64
#include <immintrin.h>
#endif

namespace xe::vmx128_fuzz {
namespace {

uint32_t RunVxUnaryFp(cpu::testing::TestGuestPpcBlock& block, uint32_t iters,
                      std::mt19937& rng, const char* name, uint32_t insn,
                      vec128_t (*ref)(cpu::testing::TestGuestPpcBlock&,
                                      const vec128_t&)) {
  const std::vector<uint32_t> ins = {PatchVx(insn, kVd, 0, kVb), kPpcInsnBlr};
  uint32_t bad = 0;
  for (uint32_t i = 0; i < iters; ++i) {
    vec128_t vb = RandomFpVec(rng);
    vec128_t expect = ref(block, vb);
    block.Run(
        ins,
        [&](cpu::ppc::PPCContext* ctx) { ctx->v[kVb] = vb; },
        [&](cpu::ppc::PPCContext* ctx) {
          if (ctx->v[kVd] != expect) {
            ++bad;
            if (bad <= 32) {
              LogDivergence(name, vb, vb, expect, ctx->v[kVd]);
            }
          }
        });
  }
  return bad;
}

uint32_t RunVx128UnaryFp(cpu::testing::TestGuestPpcBlock& block, uint32_t iters,
                         std::mt19937& rng, const char* name, uint32_t insn,
                         vec128_t (*ref)(cpu::testing::TestGuestPpcBlock&,
                                         const vec128_t&)) {
  const std::vector<uint32_t> ins = {PatchVx128_3(insn, kVd, kVb, 0),
                                     kPpcInsnBlr};
  uint32_t bad = 0;
  for (uint32_t i = 0; i < iters; ++i) {
    vec128_t vb = RandomFpVec(rng);
    vec128_t expect = ref(block, vb);
    block.Run(
        ins,
        [&](cpu::ppc::PPCContext* ctx) { ctx->v[kVb] = vb; },
        [&](cpu::ppc::PPCContext* ctx) {
          if (ctx->v[kVd] != expect) {
            ++bad;
            if (bad <= 32) {
              LogDivergence(name, vb, vb, expect, ctx->v[kVd]);
            }
          }
        });
  }
  return bad;
}

#if XE_ARCH_AMD64
static vec128_t Ref_vrfim(cpu::testing::TestGuestPpcBlock&, const vec128_t& vb) {
  __m128 x = _mm_load_ps(vb.f32);
  __m128 r = _mm_round_ps(x, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC);
  vec128_t o;
  _mm_store_ps(o.f32, r);
  return o;
}
static vec128_t Ref_vrfin(cpu::testing::TestGuestPpcBlock&, const vec128_t& vb) {
  __m128 x = _mm_load_ps(vb.f32);
  __m128 r = _mm_round_ps(x, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
  vec128_t o;
  _mm_store_ps(o.f32, r);
  return o;
}
static vec128_t Ref_vrfip(cpu::testing::TestGuestPpcBlock&, const vec128_t& vb) {
  vec128_t pin = {};
  if (LookupVrfip128Pin(vb, &pin) || LookupVrfipPin(vb, &pin)) {
    return pin;
  }
  return RefVmxVrfip(vb);
}
static vec128_t Ref_vrefp(cpu::testing::TestGuestPpcBlock&, const vec128_t& vb) {
  vec128_t pin = {};
  if (LookupVrefp128Pin(vb, &pin) || LookupVrefpPin(vb, &pin)) {
    return pin;
  }
  __m128 x = _mm_load_ps(vb.f32);
  __m128 one = _mm_set1_ps(1.0f);
  __m128 r = _mm_div_ps(one, x);
  vec128_t o;
  _mm_store_ps(o.f32, r);
  return o;
}
static vec128_t Ref_vrsqrtefp(cpu::testing::TestGuestPpcBlock& b,
                              const vec128_t& vb) {
  return b.ReferenceVrsqrtefpVector(vb);
}
#else
static vec128_t Ref_vrfim(cpu::testing::TestGuestPpcBlock&, const vec128_t& vb) {
  vec128_t o = {};
  for (int i = 0; i < 4; ++i) {
    o.f32[i] = std::floor(vb.f32[i]);
  }
  return o;
}
static vec128_t Ref_vrfin(cpu::testing::TestGuestPpcBlock&, const vec128_t& vb) {
  vec128_t o = {};
  for (int i = 0; i < 4; ++i) {
    o.f32[i] = std::nearbyint(vb.f32[i]);
  }
  return o;
}
static vec128_t Ref_vrfip(cpu::testing::TestGuestPpcBlock&, const vec128_t& vb) {
  vec128_t pin = {};
  if (LookupVrfip128Pin(vb, &pin) || LookupVrfipPin(vb, &pin)) {
    return pin;
  }
  return RefVmxVrfip(vb);
}
static vec128_t Ref_vrefp(cpu::testing::TestGuestPpcBlock&, const vec128_t& vb) {
  vec128_t pin = {};
  if (LookupVrefp128Pin(vb, &pin) || LookupVrefpPin(vb, &pin)) {
    return pin;
  }
  vec128_t o = {};
  for (int i = 0; i < 4; ++i) {
    o.f32[i] = 1.0f / vb.f32[i];
  }
  return o;
}
static vec128_t Ref_vrsqrtefp(cpu::testing::TestGuestPpcBlock&,
                              const vec128_t& vb) {
  vec128_t o = {};
  for (int i = 0; i < 4; ++i) {
    o.f32[i] = 1.0f / std::sqrt(vb.f32[i]);
  }
  return o;
}
#endif

static int32_t FloatToI32SatTrunc(float f) {
  if (std::isnan(f)) {
    return 0;
  }
  if (f >= 2147483520.f) {
    return INT32_MAX;
  }
  if (f <= -2147483648.f) {
    return INT32_MIN;
  }
  return static_cast<int32_t>(std::trunc(static_cast<double>(f)));
}

static vec128_t Ref_vcfpsxws128_imm0(const vec128_t& vb) {
  vec128_t o{};
  for (int i = 0; i < 4; ++i) {
    o.i32[i] = FloatToI32SatTrunc(vb.f32[i]);
  }
  return o;
}

static vec128_t Ref_vcfpuxws128_imm0(const vec128_t& vb) {
  vec128_t o{};
  for (int i = 0; i < 4; ++i) {
    const float f = vb.f32[i];
    if (std::isnan(f) || f <= 0.f) {
      o.u32[i] = 0;
    } else if (f >= static_cast<float>(UINT32_MAX)) {
      o.u32[i] = UINT32_MAX;
    } else {
      o.u32[i] = static_cast<uint32_t>(std::trunc(static_cast<double>(f)));
    }
  }
  return o;
}

static vec128_t Ref_vcsxwfp128_imm0(const vec128_t& vb) {
  vec128_t o{};
  for (int i = 0; i < 4; ++i) {
    o.f32[i] = static_cast<float>(vb.i32[i]);
  }
  return o;
}

static vec128_t Ref_vcuxwfp128_imm0(const vec128_t& vb) {
  vec128_t o{};
  for (int i = 0; i < 4; ++i) {
    o.f32[i] = static_cast<float>(vb.u32[i]);
  }
  return o;
}

static vec128_t Ref_vcfsx_imm3(const vec128_t& vb) {
  vec128_t o = {};
  const float scale = std::ldexp(1.0f, -3);
  for (int i = 0; i < 4; ++i) {
    o.f32[i] = float(int32_t(vb.i32[i])) * scale;
  }
  return o;
}

uint32_t RunVcfsxImm(cpu::testing::TestGuestPpcBlock& block, uint32_t iters,
                     std::mt19937& rng, const char* name, uint32_t insn,
                     uint32_t uimm) {
  const std::vector<uint32_t> ins = {PatchVx(insn, kVd, uimm & 31u, kVb),
                                     kPpcInsnBlr};
  uint32_t bad = 0;
  for (uint32_t i = 0; i < iters; ++i) {
    vec128_t vb = RandomVec(rng);
    vec128_t expect = Ref_vcfsx_imm3(vb);
    block.Run(
        ins,
        [&](cpu::ppc::PPCContext* ctx) { ctx->v[kVb] = vb; },
        [&](cpu::ppc::PPCContext* ctx) {
          if (ctx->v[kVd] != expect) {
            ++bad;
            if (bad <= 32) {
              LogDivergence(name, vb, vb, expect, ctx->v[kVd]);
            }
          }
        });
  }
  return bad;
}

static uint32_t Run_vrfim(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                          std::mt19937& rng) noexcept {
  return RunVxUnaryFp(b, it, rng, "vrfim", 0x100002cau, Ref_vrfim);
}
static uint32_t Run_vrfin(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                          std::mt19937& rng) noexcept {
  return RunVxUnaryFp(b, it, rng, "vrfin", 0x1000020au, Ref_vrfin);
}
static uint32_t Run_vrfip(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                          std::mt19937& rng) noexcept {
  return RunVxUnaryFp(b, it, rng, "vrfip", 0x1000028au, Ref_vrfip);
}
static uint32_t Run_vrefp(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                          std::mt19937& rng) noexcept {
  return RunVxUnaryFp(b, it, rng, "vrefp", 0x1000010au, Ref_vrefp);
}
static uint32_t Run_vrsqrtefp(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                              std::mt19937& rng) noexcept {
  return RunVxUnaryFp(b, it, rng, "vrsqrtefp", 0x1000014au, Ref_vrsqrtefp);
}

static uint32_t Run_vrfim128(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                             std::mt19937& rng) noexcept {
  return RunVx128UnaryFp(b, it, rng, "vrfim128", 0x18000330u, Ref_vrfim);
}
static uint32_t Run_vrfin128(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                             std::mt19937& rng) noexcept {
  return RunVx128UnaryFp(b, it, rng, "vrfin128", 0x18000370u, Ref_vrfin);
}
static uint32_t Run_vrfip128(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                             std::mt19937& rng) noexcept {
  return RunVx128UnaryFp(b, it, rng, "vrfip128", 0x180003b0u, Ref_vrfip);
}
static uint32_t Run_vrefp128(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                             std::mt19937& rng) noexcept {
  return RunVx128UnaryFp(b, it, rng, "vrefp128", 0x18000630u, Ref_vrefp);
}
static uint32_t Run_vrsqrtefp128(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                                 std::mt19937& rng) noexcept {
  return RunVx128UnaryFp(b, it, rng, "vrsqrtefp128", 0x18000670u,
                         Ref_vrsqrtefp);
}

static uint32_t Run_vcfpsxws128(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                                std::mt19937& rng) noexcept {
  constexpr uint32_t kInsn = 0x18000230u;
  const std::vector<uint32_t> ins = {PatchVx128_3(kInsn, kVd, kVb, 0u),
                                       kPpcInsnBlr};
  uint32_t bad = 0;
  for (uint32_t i = 0; i < it; ++i) {
    vec128_t vb = RandomFpVec(rng);
    vec128_t expect = Ref_vcfpsxws128_imm0(vb);
    b.Run(
        ins,
        [&](cpu::ppc::PPCContext* ctx) { ctx->v[kVb] = vb; },
        [&](cpu::ppc::PPCContext* ctx) {
          if (ctx->v[kVd] != expect) {
            ++bad;
            if (bad <= 32) {
              LogDivergence("vcfpsxws128", vb, vb, expect, ctx->v[kVd]);
            }
          }
        });
  }
  return bad;
}
static uint32_t Run_vcfpuxws128(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                                std::mt19937& rng) noexcept {
  constexpr uint32_t kInsn = 0x18000270u;
  const std::vector<uint32_t> ins = {PatchVx128_3(kInsn, kVd, kVb, 0u),
                                       kPpcInsnBlr};
  uint32_t bad = 0;
  for (uint32_t i = 0; i < it; ++i) {
    vec128_t vb = RandomFpVec(rng);
    vec128_t expect = Ref_vcfpuxws128_imm0(vb);
    b.Run(
        ins,
        [&](cpu::ppc::PPCContext* ctx) { ctx->v[kVb] = vb; },
        [&](cpu::ppc::PPCContext* ctx) {
          if (ctx->v[kVd] != expect) {
            ++bad;
            if (bad <= 32) {
              LogDivergence("vcfpuxws128", vb, vb, expect, ctx->v[kVd]);
            }
          }
        });
  }
  return bad;
}
static uint32_t Run_vcsxwfp128(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                               std::mt19937& rng) noexcept {
  constexpr uint32_t kInsn = 0x180002b0u;
  const std::vector<uint32_t> ins = {PatchVx128_3(kInsn, kVd, kVb, 0u),
                                       kPpcInsnBlr};
  uint32_t bad = 0;
  for (uint32_t i = 0; i < it; ++i) {
    vec128_t vb = RandomVec(rng);
    vec128_t expect = Ref_vcsxwfp128_imm0(vb);
    b.Run(
        ins,
        [&](cpu::ppc::PPCContext* ctx) { ctx->v[kVb] = vb; },
        [&](cpu::ppc::PPCContext* ctx) {
          if (ctx->v[kVd] != expect) {
            ++bad;
            if (bad <= 32) {
              LogDivergence("vcsxwfp128", vb, vb, expect, ctx->v[kVd]);
            }
          }
        });
  }
  return bad;
}
static uint32_t Run_vcuxwfp128(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                               std::mt19937& rng) noexcept {
  constexpr uint32_t kInsn = 0x180002f0u;
  const std::vector<uint32_t> ins = {PatchVx128_3(kInsn, kVd, kVb, 0u),
                                       kPpcInsnBlr};
  uint32_t bad = 0;
  for (uint32_t i = 0; i < it; ++i) {
    vec128_t vb = RandomVec(rng);
    vec128_t expect = Ref_vcuxwfp128_imm0(vb);
    b.Run(
        ins,
        [&](cpu::ppc::PPCContext* ctx) { ctx->v[kVb] = vb; },
        [&](cpu::ppc::PPCContext* ctx) {
          if (ctx->v[kVd] != expect) {
            ++bad;
            if (bad <= 32) {
              LogDivergence("vcuxwfp128", vb, vb, expect, ctx->v[kVd]);
            }
          }
        });
  }
  return bad;
}

static uint32_t Run_vcfsx(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                          std::mt19937& rng) noexcept {
  return RunVcfsxImm(b, it, rng, "vcfsx", 0x1000034au, 3u);
}

}  // namespace

void RegisterFloatConverts() {
  auto& v = FuzzCases();
  v.push_back({"vcfpsxws128", Run_vcfpsxws128});
  v.push_back({"vcfpuxws128", Run_vcfpuxws128});
  v.push_back({"vcsxwfp128", Run_vcsxwfp128});
  v.push_back({"vcuxwfp128", Run_vcuxwfp128});
  v.push_back({"vcfsx", Run_vcfsx});
  v.push_back({"vrefp", Run_vrefp});
  v.push_back({"vrefp128", Run_vrefp128});
  v.push_back({"vrfim", Run_vrfim});
  v.push_back({"vrfim128", Run_vrfim128});
  v.push_back({"vrfin", Run_vrfin});
  v.push_back({"vrfin128", Run_vrfin128});
  v.push_back({"vrfip", Run_vrfip});
  v.push_back({"vrfip128", Run_vrfip128});
  v.push_back({"vrsqrtefp", Run_vrsqrtefp});
  v.push_back({"vrsqrtefp128", Run_vrsqrtefp128});
}

}  // namespace xe::vmx128_fuzz
