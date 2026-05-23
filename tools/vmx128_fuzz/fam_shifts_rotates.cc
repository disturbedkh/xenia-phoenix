/**
 ******************************************************************************
 * Per-element rotates/shifts, whole-vector vsl/vsr, vslo/vsro (+128 where
 * applicable).
 ******************************************************************************
 */

#include "fuzz_internal.h"

#include "xenia/base/vec128.h"

namespace xe::vmx128_fuzz {
namespace {

// Matches x64_sequences.cc SHL_V128::EmulateShlV128 / SHR_V128::EmulateShrV128.
static vec128_t Ref_vsl(const vec128_t& a, const vec128_t& b) {
  vec128_t value = a;
  // PPC byte 15 == physical u8[15^3] (see VEC128_B / x64 EXTRACT_I8).
  const uint8_t shamt = static_cast<uint8_t>(b.u8[15u ^ 3u] & 7u);
  for (int i = 0; i < 15; ++i) {
    value.u8[i ^ 0x3] = static_cast<uint8_t>(
        (uint32_t(value.u8[i ^ 0x3]) << shamt) |
        (uint32_t(value.u8[(i + 1) ^ 0x3]) >> (8 - shamt)));
  }
  value.u8[15 ^ 0x3] = static_cast<uint8_t>(uint32_t(value.u8[15 ^ 0x3]) << shamt);
  return value;
}

static vec128_t Ref_vsr(const vec128_t& a, const vec128_t& b) {
  vec128_t value = a;
  // PPC byte 15 == physical u8[15^3] (see VEC128_B / x64 EXTRACT_I8).
  const uint8_t shamt = static_cast<uint8_t>(b.u8[15u ^ 3u] & 7u);
  for (int i = 15; i > 0; --i) {
    value.u8[i ^ 0x3] = static_cast<uint8_t>(
        (uint32_t(value.u8[i ^ 0x3]) >> shamt) |
        (uint32_t(value.u8[(i - 1) ^ 0x3]) << (8 - shamt)));
  }
  value.u8[0 ^ 0x3] = static_cast<uint8_t>(uint32_t(value.u8[0 ^ 0x3]) >> shamt);
  return value;
}

// Matches backend lvsl_table + INT8 Permute (value.cc Value::Permute).
static const vec128_t kLvslTable[16] = {
    vec128b(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15),
    vec128b(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16),
    vec128b(2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17),
    vec128b(3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18),
    vec128b(4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19),
    vec128b(5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20),
    vec128b(6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21),
    vec128b(7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22),
    vec128b(8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23),
    vec128b(9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24),
    vec128b(10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25),
    vec128b(11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26),
    vec128b(12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27),
    vec128b(13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28),
    vec128b(14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29),
    vec128b(15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30),
};
static const vec128_t kLvsrTable[16] = {
    vec128b(16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31),
    vec128b(15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30),
    vec128b(14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29),
    vec128b(13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28),
    vec128b(12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27),
    vec128b(11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26),
    vec128b(10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25),
    vec128b(9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24),
    vec128b(8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23),
    vec128b(7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22),
    vec128b(6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21),
    vec128b(5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20),
    vec128b(4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19),
    vec128b(3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18),
    vec128b(2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17),
    vec128b(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16),
};

static vec128_t RefPermuteInt8(const vec128_t& control, const vec128_t& va,
                               const vec128_t& vb) {
  uint8_t table[32];
  for (int i = 0; i < 16; ++i) {
    table[i] = va.u8[i];
    table[i + 16] = vb.u8[i];
  }
  vec128_t o = {};
  for (int i = 0; i < 16; ++i) {
    const uint32_t idx = (uint32_t(control.u8[i]) ^ 3u) & 31u;
    o.u8[i] = table[idx];
  }
  return o;
}

static vec128_t Ref_vslo(const vec128_t& a, const vec128_t& b) {
  const uint32_t bytes = (uint32_t(b.u8[15u ^ 3u]) & 0x78u) >> 3;
  vec128_t zero = {};
  return RefPermuteInt8(kLvslTable[bytes & 15u], a, zero);
}

static vec128_t Ref_vsro(const vec128_t& a, const vec128_t& b) {
  const uint32_t bytes = (uint32_t(b.u8[15u ^ 3u]) & 0x78u) >> 3;
  vec128_t zero = {};
  return RefPermuteInt8(kLvsrTable[bytes & 15u], zero, a);
}

static const vec128_t kVsldoiTable[16] = {
    vec128b(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15),
    vec128b(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16),
    vec128b(2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17),
    vec128b(3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18),
    vec128b(4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19),
    vec128b(5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20),
    vec128b(6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21),
    vec128b(7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22),
    vec128b(8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23),
    vec128b(9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24),
    vec128b(10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25),
    vec128b(11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26),
    vec128b(12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27),
    vec128b(13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28),
    vec128b(14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29),
    vec128b(15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30),
};

static vec128_t Ref_vsldoi(const vec128_t& va, const vec128_t& vb, uint32_t sh) {
  sh &= 15u;
  if (sh == 0) {
    return va;
  }
  return RefPermuteInt8(kVsldoiTable[sh], va, vb);
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

uint32_t RunVsldoi(cpu::testing::TestGuestPpcBlock& block, uint32_t iters,
                   std::mt19937& rng, const char* name) {
  constexpr uint32_t kInsn = 0x1000002c;
  uint32_t bad = 0;
  for (uint32_t i = 0; i < iters; ++i) {
    vec128_t a = RandomVec(rng);
    vec128_t b = RandomVec(rng);
    const uint32_t sh = static_cast<uint32_t>(rng()) & 15u;
    const std::vector<uint32_t> ins = {PatchVxa(kInsn, kVd, kVa, kVb, sh),
                                       kPpcInsnBlr};
    vec128_t expect = Ref_vsldoi(a, b, sh);
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

uint32_t RunVsldoi128(cpu::testing::TestGuestPpcBlock& block, uint32_t iters,
                      std::mt19937& rng, const char* name) {
  constexpr uint32_t kInsn = 0x10000010;
  uint32_t bad = 0;
  for (uint32_t i = 0; i < iters; ++i) {
    vec128_t a = RandomVec(rng);
    vec128_t b = RandomVec(rng);
    const uint32_t sh = static_cast<uint32_t>(rng()) & 15u;
    const std::vector<uint32_t> ins = {PatchVx128_5(kInsn, kVd, kVa, kVb, sh),
                                       kPpcInsnBlr};
    vec128_t expect = Ref_vsldoi(a, b, sh);
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

static vec128_t Ref_vrlb(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 16; ++j) {
    uint32_t x = a.u8[j];
    uint32_t s = b.u8[j] & 7u;
    o.u8[j] = static_cast<uint8_t>((x << s) | (x >> (8 - s)));
  }
  return o;
}
static vec128_t Ref_vrlh(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 8; ++j) {
    uint32_t x = a.u16[j];
    uint32_t s = b.u16[j] & 15u;
    o.u16[j] = static_cast<uint16_t>((x << s) | (x >> (16 - s)));
  }
  return o;
}
static vec128_t Ref_vrlw(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 4; ++j) {
    uint32_t x = a.u32[j];
    uint32_t s = b.u32[j] & 31u;
    o.u32[j] = (x << s) | (x >> (32 - s));
  }
  return o;
}

static vec128_t Ref_vslb(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 16; ++j) {
    o.u8[j] = static_cast<uint8_t>(uint32_t(a.u8[j]) << (b.u8[j] & 7u));
  }
  return o;
}
static vec128_t Ref_vslh(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 8; ++j) {
    o.u16[j] = static_cast<uint16_t>(uint32_t(a.u16[j]) << (b.u16[j] & 15u));
  }
  return o;
}
static vec128_t Ref_vslw(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 4; ++j) {
    o.u32[j] = a.u32[j] << (b.u32[j] & 31u);
  }
  return o;
}

static vec128_t Ref_vsrb(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 16; ++j) {
    o.u8[j] = static_cast<uint8_t>(uint32_t(a.u8[j]) >> (b.u8[j] & 7u));
  }
  return o;
}
static vec128_t Ref_vsrh(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 8; ++j) {
    o.u16[j] = static_cast<uint16_t>(uint32_t(a.u16[j]) >> (b.u16[j] & 15u));
  }
  return o;
}
static vec128_t Ref_vsrw(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 4; ++j) {
    o.u32[j] = a.u32[j] >> (b.u32[j] & 31u);
  }
  return o;
}

static vec128_t Ref_vsrab(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 16; ++j) {
    o.i8[j] = static_cast<int8_t>(int8_t(a.u8[j]) >> int(b.u8[j] & 7u));
  }
  return o;
}
static vec128_t Ref_vsrah(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 8; ++j) {
    o.i16[j] = static_cast<int16_t>(int16_t(a.u16[j]) >> int(b.u16[j] & 15u));
  }
  return o;
}
static vec128_t Ref_vsraw(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int j = 0; j < 4; ++j) {
    o.i32[j] = int32_t(a.u32[j]) >> int(b.u32[j] & 31u);
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

RUN(vrlb, 0x10000004, Ref_vrlb)
RUN(vrlh, 0x10000044, Ref_vrlh)
RUN(vrlw, 0x10000084, Ref_vrlw)
RUN(vslb, 0x10000104, Ref_vslb)
RUN(vslh, 0x10000144, Ref_vslh)
RUN(vslw, 0x10000184, Ref_vslw)
RUN(vsrb, 0x10000204, Ref_vsrb)
RUN(vsrh, 0x10000244, Ref_vsrh)
RUN(vsrw, 0x10000284, Ref_vsrw)
RUN(vsrab, 0x10000304, Ref_vsrab)
RUN(vsrah, 0x10000344, Ref_vsrah)
RUN(vsraw, 0x10000384, Ref_vsraw)
RUN(vsl, 0x100001c4, Ref_vsl)
RUN(vsr, 0x100002c4, Ref_vsr)
RUN(vslo, 0x1000040c, Ref_vslo)
RUN(vsro, 0x1000044c, Ref_vsro)

RUN128(vrlw128, 0x18000050, Ref_vrlw)
RUN128(vslw128, 0x180000d0, Ref_vslw)
RUN128(vsrw128, 0x180001d0, Ref_vsrw)
RUN128(vsraw128, 0x18000150, Ref_vsraw)
RUN128(vslo128, 0x14000390, Ref_vslo)
RUN128(vsro128, 0x140003d0, Ref_vsro)

#undef RUN
#undef RUN128

static uint32_t Run_vsldoi(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                           std::mt19937& rng) noexcept {
  return RunVsldoi(b, it, rng, "vsldoi");
}
static uint32_t Run_vsldoi128(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                              std::mt19937& rng) noexcept {
  return RunVsldoi128(b, it, rng, "vsldoi128");
}

}  // namespace

void RegisterShiftsRotatesWhole() {
  auto& v = FuzzCases();
  v.push_back({"vrlb", Run_vrlb});
  v.push_back({"vrlh", Run_vrlh});
  v.push_back({"vrlw", Run_vrlw});
  v.push_back({"vrlw128", Run_vrlw128});
  v.push_back({"vslo", Run_vslo});
  v.push_back({"vslo128", Run_vslo128});
  v.push_back({"vsl", Run_vsl});
  v.push_back({"vslb", Run_vslb});
  v.push_back({"vslh", Run_vslh});
  v.push_back({"vslw", Run_vslw});
  v.push_back({"vslw128", Run_vslw128});
  v.push_back({"vsldoi", Run_vsldoi});
  v.push_back({"vsldoi128", Run_vsldoi128});
  v.push_back({"vsr", Run_vsr});
  v.push_back({"vsrb", Run_vsrb});
  v.push_back({"vsrh", Run_vsrh});
  v.push_back({"vsro", Run_vsro});
  v.push_back({"vsro128", Run_vsro128});
  v.push_back({"vsraw", Run_vsraw});
  v.push_back({"vsraw128", Run_vsraw128});
  v.push_back({"vsrah", Run_vsrah});
  v.push_back({"vsrab", Run_vsrab});
  v.push_back({"vsrw", Run_vsrw});
  v.push_back({"vsrw128", Run_vsrw128});
}

}  // namespace xe::vmx128_fuzz
