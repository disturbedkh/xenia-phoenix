/**
 ******************************************************************************
 * Integer add/sub modulo + saturate (classic VMX).
 ******************************************************************************
 */

#include <algorithm>
#include <cmath>

#include "fuzz_internal.h"
#include "xenia/cpu/testing/guest_ppc_test_util.h"

namespace xe::vmx128_fuzz {
namespace {

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
    o.u32[i] = (a.u32[i] < b.u32[i]) ? 0u : (a.u32[i] - b.u32[i]);
  }
  return o;
}
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
    o.i32[i] = static_cast<int32_t>(
        std::clamp(s, int64_t(-2147483648LL), int64_t(2147483647LL)));
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
    o.i32[i] = static_cast<int32_t>(
        std::clamp(s, int64_t(-2147483648LL), int64_t(2147483647LL)));
  }
  return o;
}

using Ref2 = vec128_t (*)(const vec128_t&, const vec128_t&);

uint32_t RunVxAltivec(cpu::testing::TestGuestPpcBlock& block, uint32_t iters,
                      std::mt19937& rng, const char* name, uint32_t xo,
                      Ref2 ref) {
  const std::vector<uint32_t> ins = {
      cpu::testing::EncodeVxAltivec(kVd, kVa, kVb, xo), kPpcInsnBlr};
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

#define DEF_ADDSUB(name, xo_const, ref_fn)                                     \
  static uint32_t Run_##name(cpu::testing::TestGuestPpcBlock& b, uint32_t it,  \
                             std::mt19937& rng) noexcept {                      \
    return RunVxAltivec(b, it, rng, #name, xo_const, ref_fn);                \
  }

DEF_ADDSUB(vaddubm, cpu::testing::kVxXO_vaddubm, Ref_vaddubm)
DEF_ADDSUB(vadduhm, cpu::testing::kVxXO_vadduhm, Ref_vadduhm)
DEF_ADDSUB(vadduwm, cpu::testing::kVxXO_vadduwm, Ref_vadduwm)
DEF_ADDSUB(vsububm, cpu::testing::kVxXO_vsububm, Ref_vsububm)
DEF_ADDSUB(vsubuhm, cpu::testing::kVxXO_vsubuhm, Ref_vsubuhm)
DEF_ADDSUB(vsubuwm, cpu::testing::kVxXO_vsubuwm, Ref_vsubuwm)
DEF_ADDSUB(vaddubs, cpu::testing::kVxXO_vaddubs, Ref_vaddubs)
DEF_ADDSUB(vadduhs, cpu::testing::kVxXO_vadduhs, Ref_vadduhs)
DEF_ADDSUB(vadduws, cpu::testing::kVxXO_vadduws, Ref_vadduws)
DEF_ADDSUB(vsububs, cpu::testing::kVxXO_vsububs, Ref_vsububs)
DEF_ADDSUB(vsubuhs, cpu::testing::kVxXO_vsubuhs, Ref_vsubuhs)
DEF_ADDSUB(vsubuws, cpu::testing::kVxXO_vsubuws, Ref_vsubuws)
DEF_ADDSUB(vaddsbs, cpu::testing::kVxXO_vaddsbs, Ref_vaddsbs)
DEF_ADDSUB(vaddshs, cpu::testing::kVxXO_vaddshs, Ref_vaddshs)
DEF_ADDSUB(vaddsws, cpu::testing::kVxXO_vaddsws, Ref_vaddsws)
DEF_ADDSUB(vsubsbs, cpu::testing::kVxXO_vsubsbs, Ref_vsubsbs)
DEF_ADDSUB(vsubshs, cpu::testing::kVxXO_vsubshs, Ref_vsubshs)
DEF_ADDSUB(vsubsws, cpu::testing::kVxXO_vsubsws, Ref_vsubsws)

}  // namespace

void RegisterIntegerAddSub() {
  auto& v = FuzzCases();
  v.push_back({"vaddubm", Run_vaddubm});
  v.push_back({"vadduhm", Run_vadduhm});
  v.push_back({"vadduwm", Run_vadduwm});
  v.push_back({"vsububm", Run_vsububm});
  v.push_back({"vsubuhm", Run_vsubuhm});
  v.push_back({"vsubuwm", Run_vsubuwm});
  v.push_back({"vaddubs", Run_vaddubs});
  v.push_back({"vadduhs", Run_vadduhs});
  v.push_back({"vadduws", Run_vadduws});
  v.push_back({"vsububs", Run_vsububs});
  v.push_back({"vsubuhs", Run_vsubuhs});
  v.push_back({"vsubuws", Run_vsubuws});
  v.push_back({"vaddsbs", Run_vaddsbs});
  v.push_back({"vaddshs", Run_vaddshs});
  v.push_back({"vaddsws", Run_vaddsws});
  v.push_back({"vsubsbs", Run_vsubsbs});
  v.push_back({"vsubshs", Run_vsubshs});
  v.push_back({"vsubsws", Run_vsubsws});
}

}  // namespace xe::vmx128_fuzz
