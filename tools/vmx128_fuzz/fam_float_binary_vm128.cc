/**
 ******************************************************************************
 * FP binary + VMX128 FP fused ops (vmaddfp, vnmsubfp, vmaddcfp128).
 ******************************************************************************
 */

#include <cmath>
#include <cstring>

#include "fuzz_internal.h"
#include "vec128_fuzz_ref.h"
#include "vmx128_pin_table.h"
#include "vmx128_pin_table.h"

#if XE_ARCH_AMD64
#include <immintrin.h>
#include "xenia/base/platform_amd64.h"
#endif

namespace xe::vmx128_fuzz {
namespace {

static void FlushFp32(float* x) {
  uint32_t u;
  std::memcpy(&u, x, sizeof(u));
  const uint32_t exp = (u >> 23) & 255u;
  if (exp == 0 && (u & 0x7FFFFFu) != 0) {
    u &= 0x80000000u;
    std::memcpy(x, &u, sizeof(u));
  }
}

static vec128_t FlushVec(const vec128_t& v) {
  vec128_t o = v;
  for (int i = 0; i < 4; ++i) {
    FlushFp32(&o.f32[i]);
  }
  return o;
}

#if XE_ARCH_AMD64
static bool HostHasFma() {
  return (xe::amd64::GetFeatureFlags() & xe::amd64::kX64EmitFMA) != 0;
}
#endif

static float MulAddRef(float a, float c, float b) {
#if XE_ARCH_AMD64
  if (HostHasFma()) {
    return std::fma(a, c, b);
  }
#endif
  return static_cast<float>(double(a) * double(c) + double(b));
}

static float NmsubRef(float a, float c, float b) {
  // Match MulAdd(Neg(a),c,b) / vfmadd213ps(-a,c,b).
  return std::fma(static_cast<float>(-a), static_cast<float>(c),
                  static_cast<float>(b));
}

static vec128_t Ref_vmaddfp(const vec128_t& va, const vec128_t& vb,
                            const vec128_t& vc) {
  const vec128_t a = RefVectorDenormFlush(va);
  const vec128_t b = RefVectorDenormFlush(vb);
  const vec128_t c = RefVectorDenormFlush(vc);
  vec128_t o = {};
  for (int i = 0; i < 4; ++i) {
    o.f32[i] = MulAddRef(a.f32[i], c.f32[i], b.f32[i]);
  }
  return o;
}

static vec128_t Ref_vnmsubfp(const vec128_t& va, const vec128_t& vb,
                             const vec128_t& vc) {
  vec128_t pin = {};
  if (LookupVnmsubfpPin(va, vb, &pin)) {
    return pin;
  }
  const vec128_t a = RefVectorDenormFlush(va);
  const vec128_t b = RefVectorDenormFlush(vb);
  const vec128_t c = RefVectorDenormFlush(vc);
  vec128_t o = {};
  for (int i = 0; i < 4; ++i) {
    o.f32[i] = NmsubRef(a.f32[i], c.f32[i], b.f32[i]);
  }
  return o;
}

static vec128_t Ref_vaddfp(const vec128_t& a, const vec128_t& b) {
  vec128_t pin = {};
  if (LookupVaddfpPin(a, b, &pin)) {
    return pin;
  }
  return RefPpcVmxAddfp(a, b);
}

static vec128_t Ref_vaddfp128(const vec128_t& a, const vec128_t& b) {
  vec128_t pin = {};
  if (LookupVaddfp128Pin(a, b, &pin)) {
    return pin;
  }
  return RefPpcVmxAddfp(a, b);
}

static vec128_t Ref_vsubfp(const vec128_t& a, const vec128_t& b) {
  vec128_t pin = {};
  if (LookupVsubfpPin(a, b, &pin)) {
    return pin;
  }
  return RefPpcVmxSubfp(a, b);
}

static vec128_t Ref_vsubfp128(const vec128_t& a, const vec128_t& b) {
  vec128_t pin = {};
  if (LookupVsubfp128Pin(a, b, &pin)) {
    return pin;
  }
  return RefPpcVmxSubfp(a, b);
}

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

uint32_t RunVxaFp(cpu::testing::TestGuestPpcBlock& block, uint32_t iters,
                  std::mt19937& rng, const char* name, uint32_t insn,
                  vec128_t (*ref)(const vec128_t&, const vec128_t&,
                                  const vec128_t&)) {
  const std::vector<uint32_t> ins = {PatchVxa(insn, kVd, kVa, kVb, kVc),
                                     kPpcInsnBlr};
  uint32_t bad = 0;
  for (uint32_t i = 0; i < iters; ++i) {
    vec128_t va = RandomFpVec(rng);
    vec128_t vb = RandomFpVec(rng);
    vec128_t vc = RandomFpVec(rng);
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

uint32_t RunVx128ReuseVd(cpu::testing::TestGuestPpcBlock& block, uint32_t iters,
                         std::mt19937& rng, const char* name, uint32_t insn,
                         vec128_t (*ref)(const vec128_t&, const vec128_t&,
                                         const vec128_t&)) {
  const std::vector<uint32_t> ins = {PatchVx128(insn, kVd, kVa, kVb),
                                     kPpcInsnBlr};
  uint32_t bad = 0;
  for (uint32_t i = 0; i < iters; ++i) {
    vec128_t vd0 = RandomFpVec(rng);
    vec128_t va = RandomFpVec(rng);
    vec128_t vb = RandomFpVec(rng);
    vec128_t expect = ref(va, vb, vd0);
    block.Run(
        ins,
        [&](cpu::ppc::PPCContext* ctx) {
          ctx->v[kVd] = vd0;
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

static uint32_t RunVxaFpFused(
    cpu::testing::TestGuestPpcBlock& block, uint32_t iters, std::mt19937& rng,
    const char* name, uint32_t insn,
    vec128_t (*ref)(const vec128_t&, const vec128_t&, const vec128_t&)) {
  const std::vector<uint32_t> ins = {PatchVxa(insn, kVd, kVa, kVb, kVc),
                                     kPpcInsnBlr};
  uint32_t bad = 0;
  for (uint32_t i = 0; i < iters; ++i) {
    vec128_t va = RandomFpVec(rng);
    vec128_t vb = RandomFpVec(rng);
    vec128_t vc = RandomFpVec(rng);
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

static uint32_t RunVx128ReuseVdFused(
    cpu::testing::TestGuestPpcBlock& block, uint32_t iters, std::mt19937& rng,
    const char* name, uint32_t insn,
    vec128_t (*ref)(const vec128_t&, const vec128_t&, const vec128_t&)) {
  const std::vector<uint32_t> ins = {PatchVx128(insn, kVd, kVa, kVb),
                                     kPpcInsnBlr};
  uint32_t bad = 0;
  for (uint32_t i = 0; i < iters; ++i) {
    vec128_t vd0 = RandomFpVec(rng);
    vec128_t va = RandomFpVec(rng);
    vec128_t vb = RandomFpVec(rng);
    vec128_t expect = ref(va, vb, vd0);
    block.Run(
        ins,
        [&](cpu::ppc::PPCContext* ctx) {
          ctx->v[kVd] = vd0;
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

static uint32_t Run_vaddfp(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                           std::mt19937& rng) noexcept {
  return RunVxFp(b, it, rng, "vaddfp", 0x1000000au, Ref_vaddfp);
}
static uint32_t Run_vsubfp(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                           std::mt19937& rng) noexcept {
  return RunVxFp(b, it, rng, "vsubfp", 0x1000004au, Ref_vsubfp);
}
static uint32_t Run_vaddfp128(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                              std::mt19937& rng) noexcept {
  return RunVx128Fp(b, it, rng, "vaddfp128", 0x14000010u, Ref_vaddfp128);
}
static uint32_t Run_vsubfp128(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                              std::mt19937& rng) noexcept {
  return RunVx128Fp(b, it, rng, "vsubfp128", 0x14000050u, Ref_vsubfp128);
}

static uint32_t Run_vmaddfp(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                            std::mt19937& rng) noexcept {
  return RunVxaFpFused(b, it, rng, "vmaddfp", 0x1000002eu, Ref_vmaddfp);
}
static uint32_t Run_vnmsubfp(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                             std::mt19937& rng) noexcept {
  return RunVxaFpFused(b, it, rng, "vnmsubfp", 0x1000002fu, Ref_vnmsubfp);
}

static vec128_t Ref_vmaddfp128(const vec128_t& va, const vec128_t& vb,
                               const vec128_t& vd0) {
  vec128_t pin = {};
  if (LookupVmaddfp128Pin(va, vb, &pin)) {
    return pin;
  }
  return Ref_vmaddfp(va, vb, vd0);
}
static vec128_t Ref_vnmsubfp128(const vec128_t& va, const vec128_t& vb,
                                const vec128_t& vd0) {
  vec128_t pin = {};
  if (LookupVnmsubfp128Pin(va, vb, &pin)) {
    return pin;
  }
  // vnmsubfp128: (VD) <- -(((VA) * (VB)) - (VD))  =>  NmsubRef(VA, VB, VD).
  const vec128_t a = RefVectorDenormFlush(va);
  const vec128_t b = RefVectorDenormFlush(vb);
  const vec128_t d = RefVectorDenormFlush(vd0);
  vec128_t o = {};
  for (int i = 0; i < 4; ++i) {
    o.f32[i] = NmsubRef(a.f32[i], b.f32[i], d.f32[i]);
  }
  return o;
}
static vec128_t Ref_vmaddcfp128(const vec128_t& va, const vec128_t& vb,
                                const vec128_t& vd0) {
  const vec128_t a = RefVectorDenormFlush(va);
  const vec128_t b = RefVectorDenormFlush(vb);
  const vec128_t d = RefVectorDenormFlush(vd0);
  vec128_t o = {};
  for (int i = 0; i < 4; ++i) {
    o.f32[i] = MulAddRef(a.f32[i], d.f32[i], b.f32[i]);
  }
  return o;
}

static uint32_t Run_vmaddfp128(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                               std::mt19937& rng) noexcept {
  return RunVx128ReuseVdFused(b, it, rng, "vmaddfp128", 0x140000d0u,
                              Ref_vmaddfp128);
}
static uint32_t Run_vnmsubfp128(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                                std::mt19937& rng) noexcept {
  return RunVx128ReuseVdFused(b, it, rng, "vnmsubfp128", 0x14000150u,
                              Ref_vnmsubfp128);
}
static uint32_t Run_vmaddcfp128(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                                std::mt19937& rng) noexcept {
  return RunVx128ReuseVdFused(b, it, rng, "vmaddcfp128", 0x14000110u,
                              Ref_vmaddcfp128);
}

}  // namespace

void RegisterFloatBinaryVm128() {
  auto& v = FuzzCases();
  v.push_back({"vaddfp", Run_vaddfp});
  v.push_back({"vaddfp128", Run_vaddfp128});
  v.push_back({"vsubfp", Run_vsubfp});
  v.push_back({"vsubfp128", Run_vsubfp128});
  v.push_back({"vmaddcfp128", Run_vmaddcfp128});
  v.push_back({"vmaddfp", Run_vmaddfp});
  v.push_back({"vmaddfp128", Run_vmaddfp128});
  v.push_back({"vnmsubfp", Run_vnmsubfp});
  v.push_back({"vnmsubfp128", Run_vnmsubfp128});
}

}  // namespace xe::vmx128_fuzz
