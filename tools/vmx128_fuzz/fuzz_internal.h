/**
 ******************************************************************************
 * vmx128-fuzz: shared registry, divergence logging, and small helpers.
 ******************************************************************************
 */
#ifndef XENIA_TOOLS_VMX128_FUZZ_FUZZ_INTERNAL_H_
#define XENIA_TOOLS_VMX128_FUZZ_FUZZ_INTERNAL_H_

#include <cstdint>
#include <fstream>
#include <functional>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "xenia/base/vec128.h"
#include "xenia/cpu/ppc/ppc_context.h"
#include "xenia/cpu/testing/guest_ppc_test_util.h"

#include "encode_patch.h"
#include "random_vec.h"

#include "xenia/base/cvar.h"

DECLARE_string(vmx128_fuzz_rm_pass);

namespace xe::vmx128_fuzz {

// Guest VR indices (classic 0-31 map to low VR128 range).
inline constexpr uint32_t kVd = 5;
inline constexpr uint32_t kVa = 3;
inline constexpr uint32_t kVb = 4;
inline constexpr uint32_t kVc = 2;

inline constexpr uint32_t kPpcInsnBlr = 0x4E800020;

struct FuzzCase {
  const char* name;
  uint32_t (*run)(cpu::testing::TestGuestPpcBlock& block, uint32_t iters,
                  std::mt19937& rng) noexcept;
};

std::vector<FuzzCase>& FuzzCases();

void AppendVecJson(std::ostringstream& oss, std::string_view key,
                   const vec128_t& v);
void LogDivergence(std::string_view op, const vec128_t& a, const vec128_t& b,
                   const vec128_t& expect, const vec128_t& got);

// CR6 after vector compare with Rc=1 (matches PPCHIRBuilder::UpdateCR6 intent).
inline bool RefCR6AllEqual(const vec128_t& mask) {
  for (int i = 0; i < 16; ++i) {
    if (mask.u8[i] != 0xFF) {
      return false;
    }
  }
  return true;
}
inline bool RefCR6NoneEqual(const vec128_t& mask) {
  for (int i = 0; i < 16; ++i) {
    if (mask.u8[i] != 0) {
      return false;
    }
  }
  return true;
}

inline bool Cr6Matches(const cpu::ppc::PPCContext* ctx,
                       const vec128_t& mask) {
  const bool all_eq = RefCR6AllEqual(mask);
  const bool none_eq = RefCR6NoneEqual(mask);
  return ctx->cr6.cr6_all_equal == (all_eq ? 1u : 0u) &&
         ctx->cr6.cr6_none_equal == (none_eq ? 1u : 0u);
}

// --- Registration (implemented in fuzz_registry.cc) -------------------------

void RegisterIntegerAddSub();
void RegisterLogicalMinmaxAvg();
void RegisterCompareSelect();
void RegisterShiftsRotatesWhole();
void RegisterPermSplatMerge();
void RegisterPackUnpack();
void RegisterMulSum();
void RegisterSums();
void RegisterFloatConverts();
void RegisterFloatBinaryVm128();
void RegisterAllFuzzCases();

}  // namespace xe::vmx128_fuzz

#endif  // XENIA_TOOLS_VMX128_FUZZ_FUZZ_INTERNAL_H_
