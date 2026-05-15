/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 */

#include "xenia/cpu/testing/guest_ppc_test_util.h"

#include "xenia/base/vec128.h"
#include "third_party/catch/include/catch.hpp"

using namespace xe;
using namespace xe::cpu::testing;
using xe::cpu::ppc::PPCContext;

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
        ctx->v[3] = vec128b(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
                            15);
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
