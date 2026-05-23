/**
 ******************************************************************************
 * vec128 layout helpers for vmx128-fuzz refs (match HIR Extract / Permute).
 ******************************************************************************
 */
#ifndef XENIA_TOOLS_VMX128_FUZZ_VEC128_FUZZ_REF_H_
#define XENIA_TOOLS_VMX128_FUZZ_VEC128_FUZZ_REF_H_

#include <cstdint>

#include "xenia/base/vec128.h"

namespace xe::vmx128_fuzz {

// RAII: MXCSR DAZ+FTZ for the duration of host SSE ref ops (matches JIT VMX path).
class VmxMxcsrScope {
 public:
  VmxMxcsrScope();
  ~VmxMxcsrScope();
  VmxMxcsrScope(const VmxMxcsrScope&) = delete;
  VmxMxcsrScope& operator=(const VmxMxcsrScope&) = delete;

 private:
#if XE_ARCH_AMD64
  unsigned int old_mxcsr_ = 0;
#endif
};

// Match x64 VEC128_B / VEC128_W / VEC128_D in cpu/backend/x64/x64_op.h.
inline uint8_t FuzzVec128B(uint8_t logical) {
  return static_cast<uint8_t>(logical ^ 3u);
}
inline uint8_t FuzzVec128W(uint8_t logical) {
  return static_cast<uint8_t>(logical ^ 1u);
}

vec128_t RefVectorDenormFlush(const vec128_t& v);
vec128_t RefVmxFlushDenormsOnly(const vec128_t& v);
// Guest JIT oracle: VMX denorm flush + SSE (matches vaddps/vmaxps under MXCSR Vmx).
vec128_t RefVmxVaddfp(const vec128_t& a, const vec128_t& b);
vec128_t RefVmxVsubfp(const vec128_t& a, const vec128_t& b);
vec128_t RefVmxVmaxfp(const vec128_t& a, const vec128_t& b);
vec128_t RefVmxVminfp(const vec128_t& a, const vec128_t& b);
vec128_t RefVmxVrfip(const vec128_t& vb);
// HIR PPC NaN rules (vector_nan_propagation_test.cc).
vec128_t RefPpcVmxAddfp(const vec128_t& a, const vec128_t& b);
vec128_t RefPpcVmxSubfp(const vec128_t& a, const vec128_t& b);
vec128_t RefPpcVmxMaxfp(const vec128_t& a, const vec128_t& b);
vec128_t RefPpcVmxMinfp(const vec128_t& a, const vec128_t& b);
vec128_t RefPermuteInt16(const vec128_t& control, const vec128_t& va,
                         const vec128_t& vb);

}  // namespace xe::vmx128_fuzz

#endif  // XENIA_TOOLS_VMX128_FUZZ_VEC128_FUZZ_REF_H_
