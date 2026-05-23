/**
 ******************************************************************************
 * Sum-across family: vsumsws, vsum2sws, vsum4{s,u}{b,h}s (QEMU semantics).
 ******************************************************************************
 */

#include <cstdint>

#include "fuzz_internal.h"
#include "vec128_fuzz_ref.h"

namespace xe::vmx128_fuzz {
namespace {

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

static vec128_t Ref_vsumsws(const vec128_t& va, const vec128_t& vb) {
  int64_t t = vb.i32[3];
  for (int E = 0; E < 4; ++E) {
    t += va.i32[E];
  }
  vec128_t o = {};
  o.i32[3] = sat_i32_i64(t);
  return o;
}

static vec128_t Ref_vsum2sws(const vec128_t& va, const vec128_t& vb) {
  vec128_t o = {};
  const int upper = 1;
  for (int grp = 0; grp < 2; ++grp) {
    const int bw = upper + grp * 2;
    int64_t t = vb.i32[bw];
    const int w0 = grp * 2;
    t += static_cast<int16_t>(va.u32[w0] >> 16);
    t += static_cast<int16_t>(va.u32[w0] & 0xFFFFu);
    o.i32[bw] = sat_i32_i64(t);
  }
  return o;
}

static vec128_t Ref_vsum4sbs(const vec128_t& va, const vec128_t& vb) {
  vec128_t o = {};
  for (int W = 0; W < 4; ++W) {
    int64_t t = vb.i32[W];
    for (int j = 0; j < 4; ++j) {
      const int be = FuzzVec128B(static_cast<uint8_t>(4 * W + j));
      t += va.i8[be];
    }
    o.i32[W] = sat_i32_i64(t);
  }
  return o;
}

static vec128_t Ref_vsum4shs(const vec128_t& va, const vec128_t& vb) {
  vec128_t o = {};
  for (int W = 0; W < 4; ++W) {
    int64_t t = vb.i32[W];
    t += static_cast<int16_t>(va.u32[W] >> 16);
    t += static_cast<int16_t>(va.u32[W] & 0xFFFFu);
    o.i32[W] = sat_i32_i64(t);
  }
  return o;
}

static vec128_t Ref_vsum4ubs(const vec128_t& va, const vec128_t& vb) {
  vec128_t o = {};
  for (int W = 0; W < 4; ++W) {
    uint64_t t = vb.u32[W];
    for (int j = 0; j < 4; ++j) {
      const int be = FuzzVec128B(static_cast<uint8_t>(4 * W + j));
      t += va.u8[be];
    }
    o.u32[W] = sat_u32_u64(t);
  }
  return o;
}

uint32_t RunVxPair(cpu::testing::TestGuestPpcBlock& block, uint32_t iters,
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

static uint32_t Run_vsumsws(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                            std::mt19937& rng) noexcept {
  return RunVxPair(b, it, rng, "vsumsws", 0x10000788u, Ref_vsumsws);
}
static uint32_t Run_vsum2sws(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                             std::mt19937& rng) noexcept {
  return RunVxPair(b, it, rng, "vsum2sws", 0x10000688u, Ref_vsum2sws);
}
static uint32_t Run_vsum4sbs(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                             std::mt19937& rng) noexcept {
  return RunVxPair(b, it, rng, "vsum4sbs", 0x10000708u, Ref_vsum4sbs);
}
static uint32_t Run_vsum4shs(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                             std::mt19937& rng) noexcept {
  return RunVxPair(b, it, rng, "vsum4shs", 0x10000648u, Ref_vsum4shs);
}
static uint32_t Run_vsum4ubs(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                              std::mt19937& rng) noexcept {
  return RunVxPair(b, it, rng, "vsum4ubs", 0x10000608u, Ref_vsum4ubs);
}

}  // namespace

void RegisterSums() {
  auto& v = FuzzCases();
  v.push_back({"vsum2sws", Run_vsum2sws});
  v.push_back({"vsum4sbs", Run_vsum4sbs});
  v.push_back({"vsum4shs", Run_vsum4shs});
  v.push_back({"vsum4ubs", Run_vsum4ubs});
  v.push_back({"vsumsws", Run_vsumsws});
}

}  // namespace xe::vmx128_fuzz
