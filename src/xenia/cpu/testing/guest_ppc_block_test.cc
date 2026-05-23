/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 */

#include "xenia/cpu/testing/guest_ppc_test_util.h"

#include <cmath>

#include "third_party/catch/include/catch.hpp"
#include "xenia/base/vec128.h"
#include "xenia/cpu/ppc/ppc_instr.h"

using namespace xe;
using namespace xe::cpu::testing;
using xe::cpu::ppc::PPCContext;
using xe::cpu::ppc::PPCOpcodeBits;

static vec128_t VecLoHi(uint64_t lo, uint64_t hi) {
  vec128_t v = {};
  v.low = lo;
  v.high = hi;
  return v;
}

static uint32_t PatchVxInsn(uint32_t insn, uint32_t vd, uint32_t va,
                            uint32_t vb) {
  PPCOpcodeBits b{};
  b.code = insn;
  b.VX.VD = vd & 31u;
  b.VX.VA = va & 31u;
  b.VX.VB = vb & 31u;
  return b.code;
}

static uint32_t PatchVx128Insn(uint32_t insn, uint32_t vd, uint32_t va,
                               uint32_t vb) {
  PPCOpcodeBits b{};
  b.code = insn;
  b.VX128.VD128l = vd & 31u;
  b.VX128.VD128h = (vd >> 5) & 3u;
  b.VX128.VA128l = va & 31u;
  b.VX128.VA128h = (va >> 5) & 1u;
  b.VX128.VA128H = (va >> 6) & 1u;
  b.VX128.VB128l = vb & 31u;
  b.VX128.VB128h = (vb >> 5) & 3u;
  return b.code;
}

static uint32_t PatchVx128Unary(uint32_t insn, uint32_t vd, uint32_t vb) {
  PPCOpcodeBits b{};
  b.code = insn;
  b.VX128_3.VD128l = vd & 31u;
  b.VX128_3.VD128h = (vd >> 5) & 3u;
  b.VX128_3.VB128l = vb & 31u;
  b.VX128_3.VB128h = (vb >> 5) & 3u;
  b.VX128_3.IMM = 0;
  return b.code;
}

static vec128_t ReferenceVaddubm(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int i = 0; i < 16; ++i) {
    o.u8[i] = static_cast<uint8_t>(a.u8[i] + b.u8[i]);
  }
  return o;
}

TEST_CASE("GUEST_PPC_vaddubm_smoke", "[guest_ppc][vmx128]") {
  TestGuestPpcBlock block;
  const std::vector<uint32_t> ins = {EncodeVaddubm(5, 3, 4), kPpcInsnBlr};

  block.Run(
      ins,
      [](PPCContext* ctx) {
        ctx->v[3] =
            vec128b(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
        ctx->v[4] =
            vec128b(100, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
      },
      [](PPCContext* ctx) {
        vec128_t expect = ReferenceVaddubm(
            vec128b(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15),
            vec128b(100, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15));
        REQUIRE(ctx->v[5] == expect);
      });
}

TEST_CASE("GUEST_PPC_vaddubm_wrap", "[guest_ppc][vmx128]") {
  TestGuestPpcBlock block;
  const std::vector<uint32_t> ins = {EncodeVaddubm(5, 3, 4), kPpcInsnBlr};

  block.Run(
      ins,
      [](PPCContext* ctx) {
        ctx->v[3] = vec128b(UINT8_MAX);
        ctx->v[4] = vec128b(1);
      },
      [](PPCContext* ctx) {
        vec128_t expect = ReferenceVaddubm(vec128b(UINT8_MAX), vec128b(1));
        REQUIRE(ctx->v[5] == expect);
      });
}

// Minimized vmx128-fuzz / permute regression (Tier 1.1).
TEST_CASE("GUEST_PPC_vmx128_vmrghh_instr1", "[guest_ppc][vmx128][div-01]") {
  TestGuestPpcBlock block;
  const std::vector<uint32_t> ins = {EncodeVxAltivec(5, 3, 4, 0x4c),
                                     kPpcInsnBlr};
  const vec128_t va =
      vec128i(0x00010203u, 0x04050607u, 0x08090A0Bu, 0x0C0D0E0Fu);
  const vec128_t vb =
      vec128i(0x10111213u, 0x14151617u, 0x18191A1Bu, 0x1C1D1E1Fu);
  // Per-word high-halfword merge (matches MergeHalfwordsPerWord / vmx128-fuzz
  // ref).
  const vec128_t expect =
      vec128i(0x00011011u, 0x04051415u, 0x08091819u, 0x0C0D1C1Du);
  block.Run(
      ins,
      [&](PPCContext* ctx) {
        ctx->v[3] = va;
        ctx->v[4] = vb;
      },
      [&](PPCContext* ctx) { REQUIRE(ctx->v[5] == expect); });
}

// Fused multiply-add where FMA vs multiply-then-add differs (Tier 1.1.1).
TEST_CASE("GUEST_PPC_vmx128_vnmsubfp_tuple", "[guest_ppc][vmx128][fma]") {
  TestGuestPpcBlock block;
  xe::cpu::ppc::PPCOpcodeBits enc{};
  enc.code = 0x1000002fu;
  enc.VXA.VD = 5;
  enc.VXA.VA = 3;
  enc.VXA.VB = 4;
  enc.VXA.VC = 6;
  const std::vector<uint32_t> ins = {enc.code, kPpcInsnBlr};
  vec128_t va = vec128i(0);
  vec128_t vb = vec128i(0);
  vec128_t vc = vec128i(0);
  const float a = 1.f + std::ldexp(1.f, -23);
  va.f32[0] = a;
  vb.f32[0] = 1.f;
  vc.f32[0] = -1.f;
  vec128_t expect = vec128i(0);
#if XE_ARCH_AMD64
  {
    const float t = std::fma(a, vc.f32[0], -vb.f32[0]);
    uint32_t u;
    std::memcpy(&u, &t, sizeof(u));
    u ^= 0x80000000u;
    std::memcpy(&expect.f32[0], &u, sizeof(u));
  }
#else
  expect.f32[0] = -(a * vc.f32[0] - vb.f32[0]);
#endif
  block.Run(
      ins,
      [&](PPCContext* ctx) {
        ctx->v[3] = va;
        ctx->v[4] = vb;
        ctx->v[6] = vc;
      },
      [&](PPCContext* ctx) { REQUIRE(ctx->v[5].u32[0] == expect.u32[0]); });
}

TEST_CASE("GUEST_PPC_vrsqrtefp_instr1", "[guest_ppc][vmx128]") {
  TestGuestPpcBlock block;
  const std::vector<uint32_t> ins = {PatchVxInsn(0x1000014au, 5, 0, 4),
                                     kPpcInsnBlr};
  const vec128_t vb = vec128f(1.0f, 4.0f, 16.0f, 100.0f);
  // JIT output for vb under VMX128 vrsqrtefp (matches vmx128-fuzz oracle).
  const vec128_t expect = VecLoHi(0x3EFFF4003F7FF400ull, 0x3DCCCA003E7FF400ull);
  block.Run(
      ins, [&](PPCContext* ctx) { ctx->v[4] = vb; },
      [&](PPCContext* ctx) { REQUIRE(ctx->v[5] == expect); });
}

// vmx128-fuzz pins (seed 3735928559 @ 50k) — JIT outputs frozen as expect.
TEST_CASE("GUEST_PPC_vaddfp_pin", "[guest_ppc][vmx128][pin]") {
  TestGuestPpcBlock block;
  const std::vector<uint32_t> ins = {PatchVxInsn(0x1000000au, 5, 3, 4),
                                     kPpcInsnBlr};
  const vec128_t va = VecLoHi(0x7FF23679ull, 0x7FBAC4AD80000000ull);
  const vec128_t vb = VecLoHi(0x6C46BFFF800000ull, 0x7FF82B1CFF800000ull);
  const vec128_t expect = VecLoHi(0x7FF23679ull, 0x7FFAC4ADFF800000ull);
  block.Run(
      ins,
      [&](PPCContext* ctx) {
        ctx->v[3] = va;
        ctx->v[4] = vb;
      },
      [&](PPCContext* ctx) { REQUIRE(ctx->v[5] == expect); });
}

TEST_CASE("GUEST_PPC_vmx128_fma_tuple", "[guest_ppc][vmx128][fma]") {
  TestGuestPpcBlock block;
  xe::cpu::ppc::PPCOpcodeBits enc{};
  enc.code = 0x1000002eu;
  enc.VXA.VD = 5;
  enc.VXA.VA = 3;
  enc.VXA.VB = 4;
  enc.VXA.VC = 6;
  const std::vector<uint32_t> ins = {enc.code, kPpcInsnBlr};
  vec128_t va = {};
  vec128_t vb = {};
  vec128_t vc = {};
  const float a = 1.f + std::ldexp(1.f, -23);
  va.f32[0] = a;
  vb.f32[0] = 1.f;
  vc.f32[0] = -1.f;
  const float expect0 = std::fma(a, vc.f32[0], vb.f32[0]);
  vec128_t expect = {};
  expect.f32[0] = expect0;
  block.Run(
      ins,
      [&](PPCContext* ctx) {
        ctx->v[3] = va;
        ctx->v[4] = vb;
        ctx->v[6] = vc;
      },
      [&](PPCContext* ctx) { REQUIRE(ctx->v[5] == expect); });
}
