/**
 ******************************************************************************
 * Permute (vperm / vperm128), splat / splat-imm, merge high/low (+128).
 ******************************************************************************
 */

#include "fuzz_internal.h"
#include "vec128_fuzz_ref.h"

#include <algorithm>

#include "xenia/cpu/hir/opcodes.h"

namespace xe::vmx128_fuzz {
namespace {

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

static vec128_t Ref_vperm(const vec128_t& vc, const vec128_t& va,
                          const vec128_t& vb) {
  return RefPermuteInt8(vc, va, vb);
}

uint32_t RunVxaPerm(cpu::testing::TestGuestPpcBlock& block, uint32_t iters,
                    std::mt19937& rng, const char* name, uint32_t insn) {
  const std::vector<uint32_t> ins = {PatchVxa(insn, kVd, kVa, kVb, kVc),
                                     kPpcInsnBlr};
  uint32_t bad = 0;
  for (uint32_t i = 0; i < iters; ++i) {
    vec128_t va = RandomVec(rng);
    vec128_t vb = RandomVec(rng);
    vec128_t vc = RandomVec(rng);
    vec128_t expect = Ref_vperm(vc, va, vb);
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

uint32_t RunVperm128Insn(cpu::testing::TestGuestPpcBlock& block, uint32_t iters,
                         std::mt19937& rng, const char* name, uint32_t insn) {
  const std::vector<uint32_t> ins = {
      PatchVx128_2(insn, kVd, kVa, kVb, kVc & 7u), kPpcInsnBlr};
  uint32_t bad = 0;
  for (uint32_t i = 0; i < iters; ++i) {
    vec128_t va = RandomVec(rng);
    vec128_t vb = RandomVec(rng);
    vec128_t vc = RandomVec(rng);
    vec128_t expect = Ref_vperm(vc, va, vb);
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

static const vec128_t kCtl_vmrghb =
    vec128b(0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23);
static const vec128_t kCtl_vmrglb =
    vec128b(8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31);

static vec128_t Ref_vmrghb(const vec128_t& a, const vec128_t& b) {
  return RefPermuteInt8(kCtl_vmrghb, a, b);
}
static vec128_t Ref_vmrglb(const vec128_t& a, const vec128_t& b) {
  return RefPermuteInt8(kCtl_vmrglb, a, b);
}
// Per-word high/low halfword merge (instr_vmrghh.s / guest_ppc_block_test).
static vec128_t Ref_vmrghh(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int w = 0; w < 4; ++w) {
    const uint32_t ah = (a.u32[w] >> 16) & 0xFFFFu;
    const uint32_t bh = (b.u32[w] >> 16) & 0xFFFFu;
    o.u32[w] = (ah << 16) | bh;
  }
  return o;
}
static vec128_t Ref_vmrglh(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  for (int w = 0; w < 4; ++w) {
    const uint32_t al = a.u32[w] & 0xFFFFu;
    const uint32_t bl = b.u32[w] & 0xFFFFu;
    o.u32[w] = (al << 16) | bl;
  }
  return o;
}
static vec128_t Ref_vmrghw(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  o.u32[0] = a.u32[0];
  o.u32[1] = b.u32[0];
  o.u32[2] = a.u32[1];
  o.u32[3] = b.u32[1];
  return o;
}
static vec128_t Ref_vmrglw(const vec128_t& a, const vec128_t& b) {
  vec128_t o = {};
  o.u32[0] = a.u32[2];
  o.u32[1] = b.u32[2];
  o.u32[2] = a.u32[3];
  o.u32[3] = b.u32[3];
  return o;
}

static vec128_t Ref_vspltisb_imm(uint32_t uimm5) {
  vec128_t o = {};
  uimm5 &= 31u;
  int8_t simm =
      (uimm5 & 0x10u) ? static_cast<int8_t>(uimm5 | 0xF0u) : static_cast<int8_t>(uimm5);
  for (int j = 0; j < 16; ++j) {
    o.i8[j] = simm;
  }
  return o;
}
static vec128_t Ref_vspltish_imm(uint32_t uimm5) {
  vec128_t o = {};
  uimm5 &= 31u;
  int16_t simm = (uimm5 & 0x10u) ? static_cast<int16_t>(uimm5 | 0xFFF0u)
                                  : static_cast<int16_t>(uimm5);
  for (int j = 0; j < 8; ++j) {
    o.i16[j] = simm;
  }
  return o;
}
static vec128_t Ref_vspltisw_imm(uint32_t uimm5) {
  vec128_t o = {};
  uimm5 &= 31u;
  int32_t simm = (uimm5 & 0x10u) ? static_cast<int32_t>(uimm5 | 0xFFFFFFF0u)
                                 : static_cast<int32_t>(uimm5);
  for (int j = 0; j < 4; ++j) {
    o.i32[j] = simm;
  }
  return o;
}

// Matches x64 PERMUTE_I32 + VPBLENDD (constant control); src2 = first vector arg.
static vec128_t PermuteI32Words(uint32_t control, const vec128_t& src2,
                                const vec128_t& src3) {
  const uint32_t src_control =
      (((control >> 24) & 0x3u) << 6) | (((control >> 16) & 0x3u) << 4) |
      (((control >> 8) & 0x3u) << 2) | (((control >> 0) & 0x3u) << 0);
  auto pshufd = [&](const vec128_t& s) {
    vec128_t o{};
    for (int out = 0; out < 4; ++out) {
      const int sel = (src_control >> (out * 2)) & 3;
      o.u32[out] = s.u32[sel];
    }
    return o;
  };
  const vec128_t shuf2 = pshufd(src2);
  const vec128_t shuf3 = pshufd(src3);
  const uint32_t blend_control =
      (((control >> 26) & 0x1u) << 3) | (((control >> 18) & 0x1u) << 2) |
      (((control >> 10) & 0x1u) << 1) | (((control >> 2) & 0x1u) << 0);
  vec128_t o{};
  for (int i = 0; i < 4; ++i) {
    // Bit 1 -> src3 (vd), bit 0 -> src2 (vb rotated), per PERMUTE_I32 emit.
    o.u32[i] = (blend_control & (1u << i)) ? shuf3.u32[i] : shuf2.u32[i];
  }
  return o;
}

static vec128_t Ref_vrlimi128(const vec128_t& vd0, const vec128_t& vb,
                              uint32_t imm5, uint32_t z) {
  const uint32_t blend_mask_src = imm5 & 31u;
  uint32_t blend_mask = 0;
  blend_mask |= (((blend_mask_src >> 3) & 0x1u) ? 0u : 4u) << 0;
  blend_mask |= (((blend_mask_src >> 2) & 0x1u) ? 1u : 5u) << 8;
  blend_mask |= (((blend_mask_src >> 1) & 0x1u) ? 2u : 6u) << 16;
  blend_mask |= (((blend_mask_src >> 0) & 0x1u) ? 3u : 7u) << 24;
  const uint32_t rotate = z & 3u;
  uint32_t swizzle_mask = xe::cpu::hir::SWIZZLE_XYZW_TO_XYZW;
  switch (rotate) {
    case 1:
      swizzle_mask = xe::cpu::hir::SWIZZLE_XYZW_TO_YZWX;
      break;
    case 2:
      swizzle_mask = xe::cpu::hir::SWIZZLE_XYZW_TO_ZWXY;
      break;
    case 3:
      swizzle_mask = xe::cpu::hir::SWIZZLE_XYZW_TO_WXYZ;
      break;
    default:
      break;
  }
  vec128_t vb_rot = {};
  for (uint32_t i = 0; i < 4; ++i) {
    const uint32_t sel = (swizzle_mask >> (i * 2)) & 3u;
    vb_rot.u32[i] = vb.u32[sel];
  }
  if (blend_mask == xe::cpu::hir::kIdentityPermuteMask) {
    return vb_rot;
  }
  return PermuteI32Words(blend_mask, vb_rot, vd0);
}

static vec128_t Ref_vpermwi128(uint32_t perm_enc, const vec128_t& vb) {
  const uint32_t uimm =
      (perm_enc & 31u) | (((perm_enc >> 5) & 7u) << 5);
  const uint32_t mask = xe::cpu::hir::MakeSwizzleMask(
      uimm >> 6, uimm >> 4, uimm >> 2, uimm >> 0);
  vec128_t o = {};
  for (uint32_t i = 0; i < 4; ++i) {
    const uint32_t sel = (mask >> (i * 2)) & 3u;
    o.u32[i] = vb.u32[sel];
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
  for (uint32_t j = 0; j < iters; ++j) {
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

uint32_t RunVspltImmVx(cpu::testing::TestGuestPpcBlock& block, uint32_t iters,
                       std::mt19937&, const char* name, uint32_t insn,
                       uint32_t uimm5,
                       vec128_t (*ref_imm)(uint32_t)) {
  const std::vector<uint32_t> ins = {PatchVx(insn, kVd, uimm5 & 31u, 0),
                                     kPpcInsnBlr};
  const vec128_t expect = ref_imm(uimm5);
  uint32_t bad = 0;
  for (uint32_t i = 0; i < iters; ++i) {
    block.Run(
        ins, [](cpu::ppc::PPCContext*) {},
        [&](cpu::ppc::PPCContext* ctx) {
          if (ctx->v[kVd] != expect) {
            ++bad;
            if (bad <= 32) {
              vec128_t z = {};
              LogDivergence(name, z, z, expect, ctx->v[kVd]);
            }
          }
        });
  }
  return bad;
}

uint32_t RunVspltisw128(cpu::testing::TestGuestPpcBlock& block, uint32_t iters,
                        std::mt19937&, const char* name, uint32_t uimm5) {
  constexpr uint32_t kInsn = 0x18000770u;
  const std::vector<uint32_t> ins = {PatchVx128_3(kInsn, kVd, 0, uimm5 & 31u),
                                      kPpcInsnBlr};
  const vec128_t expect = Ref_vspltisw_imm(uimm5);
  uint32_t bad = 0;
  for (uint32_t i = 0; i < iters; ++i) {
    block.Run(
        ins, [](cpu::ppc::PPCContext*) {},
        [&](cpu::ppc::PPCContext* ctx) {
          if (ctx->v[kVd] != expect) {
            ++bad;
            if (bad <= 32) {
              vec128_t z = {};
              LogDivergence(name, z, z, expect, ctx->v[kVd]);
            }
          }
        });
  }
  return bad;
}

uint32_t RunVpermwi128(cpu::testing::TestGuestPpcBlock& block, uint32_t iters,
                       std::mt19937& rng, const char* name, uint32_t perm6) {
  constexpr uint32_t kInsn = 0x18000210u;
  const std::vector<uint32_t> ins = {
      PatchVx128_P(kInsn, kVd, kVb, perm6 & 63u), kPpcInsnBlr};
  uint32_t bad = 0;
  for (uint32_t i = 0; i < iters; ++i) {
    vec128_t vb = RandomVec(rng);
    vec128_t expect = Ref_vpermwi128(perm6, vb);
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

static uint32_t Run_vperm(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                          std::mt19937& rng) noexcept {
  return RunVxaPerm(b, it, rng, "vperm", 0x1000002bu);
}
static uint32_t Run_vperm128(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                             std::mt19937& rng) noexcept {
  return RunVperm128Insn(b, it, rng, "vperm128", 0x14000000u);
}

static uint32_t Run_vrlimi128(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                              std::mt19937& rng) noexcept {
  constexpr uint32_t kInsn = 0x18000710u;
  // At most 32*4 unique encodings — cache guest ins words to avoid exhausting
  // the x64 unwind table (one JIT function per distinct insn vector).
  static std::vector<std::vector<uint32_t>> ins_cache;
  if (ins_cache.empty()) {
    ins_cache.resize(128);
    for (uint32_t k = 0; k < 128; ++k) {
      const uint32_t imm5 = k >> 2;
      const uint32_t z = k & 3u;
      ins_cache[k] = {PatchVx128_4(kInsn, kVd, kVb, imm5, z), kPpcInsnBlr};
    }
  }
  uint32_t bad = 0;
  for (uint32_t i = 0; i < it; ++i) {
    const uint32_t key =
        (i < 128) ? i : ((rng() & 31u) | ((rng() & 3u) << 5));
    const uint32_t imm5 = key & 31u;
    const uint32_t z = (key >> 5) & 3u;
    const auto& ins = ins_cache[(imm5 << 2) | z];
    const vec128_t vd0 = RandomVec(rng);
    const vec128_t vb = RandomVec(rng);
    const vec128_t expect = Ref_vrlimi128(vd0, vb, imm5, z);
    b.Run(
        ins,
        [&](cpu::ppc::PPCContext* ctx) {
          ctx->v[kVd] = vd0;
          ctx->v[kVb] = vb;
        },
        [&](cpu::ppc::PPCContext* ctx) {
          if (ctx->v[kVd] != expect) {
            ++bad;
            if (bad <= 32) {
              LogDivergence("vrlimi128", vd0, vb, expect, ctx->v[kVd]);
            }
          }
        });
  }
  return bad;
}

RUN(vmrghb, 0x1000000cu, Ref_vmrghb)
RUN(vmrghh, 0x1000004cu, Ref_vmrghh)
RUN(vmrghw, 0x1000008cu, Ref_vmrghw)
RUN128(vmrghw128, 0x18000300u, Ref_vmrghw)
RUN(vmrglb, 0x1000010cu, Ref_vmrglb)
RUN(vmrglh, 0x1000014cu, Ref_vmrglh)
RUN(vmrglw, 0x1000018cu, Ref_vmrglw)
RUN128(vmrglw128, 0x18000340u, Ref_vmrglw)

static uint32_t Run_vspltisb_7(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                               std::mt19937& rng) noexcept {
  return RunVspltImmVx(b, it, rng, "vspltisb", 0x1000030cu, 7u, Ref_vspltisb_imm);
}
static uint32_t Run_vspltish_7(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                               std::mt19937& rng) noexcept {
  return RunVspltImmVx(b, it, rng, "vspltish", 0x1000034cu, 7u, Ref_vspltish_imm);
}
static uint32_t Run_vspltisw_7(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                               std::mt19937& rng) noexcept {
  return RunVspltImmVx(b, it, rng, "vspltisw", 0x1000038cu, 7u, Ref_vspltisw_imm);
}
static uint32_t Run_vspltisw128_full(cpu::testing::TestGuestPpcBlock& b, uint32_t it,
                                    std::mt19937& rng) noexcept {
  if (it == 0) {
    return 0;
  }
  const uint32_t per_imm = std::max(1u, it / 32u);
  uint32_t bad = 0;
  for (uint32_t imm = 0; imm < 32; ++imm) {
    bad += RunVspltisw128(b, per_imm, rng, "vspltisw128", imm);
  }
  return bad;
}
static uint32_t Run_vpermwi128_xyzw(cpu::testing::TestGuestPpcBlock& b,
                                    uint32_t it, std::mt19937& rng) noexcept {
  // uimm bits for identity-ish swizzle (matches MakeSwizzleMask applied in emit).
  const uint32_t perm6 = 0x1Bu;  // 00 01 10 11 nibbles -> xyzw
  return RunVpermwi128(b, it, rng, "vpermwi128", perm6);
}

#undef RUN
#undef RUN128

}  // namespace

void RegisterPermSplatMerge() {
  auto& v = FuzzCases();
  v.push_back({"vperm", Run_vperm});
  v.push_back({"vperm128", Run_vperm128});
  v.push_back({"vrlimi128", Run_vrlimi128});
  v.push_back({"vmrghb", Run_vmrghb});
  v.push_back({"vmrghh", Run_vmrghh});
  v.push_back({"vmrghw", Run_vmrghw});
  v.push_back({"vmrghw128", Run_vmrghw128});
  v.push_back({"vmrglb", Run_vmrglb});
  v.push_back({"vmrglh", Run_vmrglh});
  v.push_back({"vmrglw", Run_vmrglw});
  v.push_back({"vmrglw128", Run_vmrglw128});
  v.push_back({"vspltisb", Run_vspltisb_7});
  v.push_back({"vspltish", Run_vspltish_7});
  v.push_back({"vspltisw", Run_vspltisw_7});
  // Classic `vspltb`/`vsplth`/`vspltw` are imm-only (no VR-index register form);
  // fuzz covers imm splats only.
  v.push_back({"vspltisw128", Run_vspltisw128_full});
  v.push_back({"vpermwi128", Run_vpermwi128_xyzw});
}

}  // namespace xe::vmx128_fuzz
