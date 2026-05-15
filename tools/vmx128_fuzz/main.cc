/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Tier 0 (MVP1): random-input smoke for guest `vaddubm` vs a scalar C++
 * reference. This is not a full VMX128 fuzzer but proves the harness + JIT
 * path used by TestGuestPpcBlock.
 ******************************************************************************
 */

#include <cstdint>
#include <iostream>
#include <random>
#include <vector>

#include "xenia/base/console_app_main.h"
#include "xenia/base/cvar.h"
#include "xenia/base/vec128.h"
#include "xenia/cpu/testing/guest_ppc_test_util.h"

using namespace xe;

DEFINE_uint32(vmx128_fuzz_iters, 5000,
              "Number of random vaddubm differential iterations.", "CPU");

namespace {

xe::vec128_t ReferenceVaddubm(const xe::vec128_t& a, const xe::vec128_t& b) {
  xe::vec128_t o = {};
  for (int i = 0; i < 16; ++i) {
    o.u8[i] = static_cast<uint8_t>(a.u8[i] + b.u8[i]);
  }
  return o;
}

xe::vec128_t RandomVec(std::mt19937& rng) {
  xe::vec128_t v = {};
  for (int i = 0; i < 16; ++i) {
    v.u8[i] = static_cast<uint8_t>(rng());
  }
  return v;
}

}  // namespace

int Main(const std::vector<std::string>&) {
  xe::cpu::testing::TestGuestPpcBlock block;
  std::mt19937 rng(0xC0FFEEu);
  const std::vector<uint32_t> ins = {xe::cpu::testing::EncodeVaddubm(5, 3, 4),
                                       xe::cpu::testing::kPpcInsnBlr};

  uint32_t mismatches = 0;
  const uint32_t iters = cvars::vmx128_fuzz_iters;
  for (uint32_t i = 0; i < iters; ++i) {
    xe::vec128_t a = RandomVec(rng);
    xe::vec128_t b = RandomVec(rng);
    xe::vec128_t expect = ReferenceVaddubm(a, b);

    block.Run(
        ins,
        [&](xe::cpu::ppc::PPCContext* ctx) {
          ctx->v[3] = a;
          ctx->v[4] = b;
        },
        [&](xe::cpu::ppc::PPCContext* ctx) {
          if (ctx->v[5] != expect) {
            ++mismatches;
          }
        });
  }

  if (mismatches) {
    std::cerr << "vmx128-fuzz: " << mismatches << " / " << iters
              << " mismatches\n";
    return 1;
  }
  std::cout << "vmx128-fuzz: OK (" << iters << " iterations)\n";
  return 0;
}

XE_DEFINE_CONSOLE_APP("vmx128-fuzz", Main, "", "");
