/**
 ******************************************************************************
 * Pack / unpack (representative modulo + signed-sat + unpack).
 ******************************************************************************
 */

#include <algorithm>

#include "fuzz_internal.h"
#include "vec128_fuzz_ref.h"

namespace xe::vmx128_fuzz {
namespace {

// After vpack*, backend applies vpshufb with XMMByteOrderMask (see x64_seq_vector.cc).
static vec128_t ApplyPackByteOrder(const vec128_t& v) {
  static const uint8_t k[16] = {0x02, 0x03, 0x00, 0x01, 0x06, 0x07, 0x04, 0x05,
                                 0x0A, 0x0B, 0x08, 0x09, 0x0E, 0x0F, 0x0C, 0x0D};
  vec128_t o = {};
  for (int i = 0; i < 16; ++i) {
    o.u8[i] = v.u8[k[i] & 0x1Fu];
  }
  return o;
}

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

static vec128_t Ref_vpkuhum(const vec128_t& va, const vec128_t& vb) {
  vec128_t stage = {};
  for (int i = 0; i < 8; ++i) {
    stage.u8[i] = va.u8[i * 2];
    stage.u8[i + 8] = vb.u8[i * 2];
  }
  return ApplyPackByteOrder(stage);
}

static vec128_t Ref_vpkshss(const vec128_t& va, const vec128_t& vb) {
  vec128_t stage = {};
  auto packh = [](int16_t x) -> int8_t {
    int v = int(x);
    return static_cast<int8_t>(std::clamp(v, -128, 127));
  };
  for (int i = 0; i < 8; ++i) {
    stage.i8[i] = packh(int16_t(va.u16[i]));
  }
  for (int i = 0; i < 8; ++i) {
    stage.i8[i + 8] = packh(int16_t(vb.u16[i]));
  }
  return ApplyPackByteOrder(stage);
}

static uint32_t Unpack5655To8888U32(uint16_t e) {
  const uint32_t _e = e;
  const uint32_t a = (_e >> 15) ? 0xffu : 0u;
  const uint32_t r = (_e >> 10) & 0x1fu;
  const uint32_t g = (_e >> 5) & 0x1fu;
  const uint32_t b = _e & 0x1fu;
  return (a << 24) | (r << 16) | (g << 8) | b;
}

static vec128_t Ref_vupkhpx(const vec128_t&, const vec128_t& vb) {
  vec128_t o = {};
  for (int wi = 0; wi < 4; ++wi) {
    const uint16_t px = static_cast<uint16_t>((vb.u32[wi] >> 16) & 0xFFFFu);
    o.u32[wi] = Unpack5655To8888U32(px);
  }
  return o;
}

static vec128_t Ref_vupklpx(const vec128_t&, const vec128_t& vb) {
  vec128_t o = {};
  for (int wi = 0; wi < 4; ++wi) {
    const uint16_t px = static_cast<uint16_t>(vb.u32[wi] & 0xFFFFu);
    o.u32[wi] = Unpack5655To8888U32(px);
  }
  return o;
}

#define RUN(name, insn, ref)                                                  \
  static uint32_t Run_##name(cpu::testing::TestGuestPpcBlock& b, uint32_t it,   \
                             std::mt19937& rng) noexcept {                      \
    return RunVx(b, it, rng, #name, (insn), (ref));                            \
  }
#define RUN128(name, insn, ref)                                               \
  static uint32_t Run_##name(cpu::testing::TestGuestPpcBlock& b, uint32_t it,   \
                             std::mt19937& rng) noexcept {                      \
    return RunVx128(b, it, rng, #name, (insn), (ref));                         \
  }

RUN(vpkuhum, 0x1000000eu, Ref_vpkuhum)
RUN128(vpkuhum128, 0x14000300u, Ref_vpkuhum)
RUN(vpkshss, 0x1000018eu, Ref_vpkshss)
RUN128(vpkshss128, 0x14000200u, Ref_vpkshss)
RUN(vupkhpx, 0x1000034eu, Ref_vupkhpx)
RUN(vupklpx, 0x100003ceu, Ref_vupklpx)

#undef RUN
#undef RUN128

}  // namespace

void RegisterPackUnpack() {
  // Note: `vpkd3d128` / `vupkd3d128` have emitters + `.s` tests but no registry
  // fuzz family yet (large typed sub-matrix); track under GPU/pack backlog if needed.
  auto& v = FuzzCases();
  v.push_back({"vpkuhum", Run_vpkuhum});
  v.push_back({"vpkuhum128", Run_vpkuhum128});
  v.push_back({"vpkshss", Run_vpkshss});
  v.push_back({"vpkshss128", Run_vpkshss128});
  v.push_back({"vupkhpx", Run_vupkhpx});
  v.push_back({"vupklpx", Run_vupklpx});
}

}  // namespace xe::vmx128_fuzz
