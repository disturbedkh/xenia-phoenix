/**
 ******************************************************************************
 * vec128 layout helpers for vmx128-fuzz refs.
 ******************************************************************************
 */

#include "vec128_fuzz_ref.h"

#include <cmath>
#include <cstring>

#if XE_ARCH_AMD64
#include <immintrin.h>
#endif

namespace xe::vmx128_fuzz {

VmxMxcsrScope::VmxMxcsrScope() {
#if XE_ARCH_AMD64
  old_mxcsr_ = _mm_getcsr();
  _mm_setcsr(old_mxcsr_ | 0x8040u);  // DAZ | FTZ (VMX flush denorms)
#endif
}

VmxMxcsrScope::~VmxMxcsrScope() {
#if XE_ARCH_AMD64
  _mm_setcsr(old_mxcsr_);
#endif
}

namespace {

constexpr uint32_t kPpcDefaultQNaN = 0xFFC00000u;

bool IsFloatNanU32(uint32_t u) {
  return (u & 0x7F800000u) == 0x7F800000u && (u & 0x7FFFFFu) != 0;
}

bool IsFloatDenormU32(uint32_t u) {
  return (u & 0x7F800000u) == 0 && (u & 0x7FFFFFu) != 0;
}

uint32_t QuietNanU32(uint32_t u) { return u | 0x00400000u; }

uint32_t FlushDenormU32(uint32_t u) {
  if ((u & 0x7F800000u) == 0 && (u & 0x7FFFFFu) != 0) {
    return u & 0x80000000u;
  }
  return u;
}

float U32ToF32(uint32_t u) {
  float f = 0.f;
  std::memcpy(&f, &u, sizeof(f));
  return f;
}

uint32_t F32ToU32(float f) {
  uint32_t u = 0;
  std::memcpy(&u, &f, sizeof(u));
  return u;
}

#if XE_ARCH_AMD64
vec128_t RefVmxVaddfpImpl(const vec128_t& a, const vec128_t& b) {
  const vec128_t fa = RefVmxFlushDenormsOnly(a);
  const vec128_t fb = RefVmxFlushDenormsOnly(b);
  __m128 va = _mm_loadu_ps(fa.f32);
  __m128 vb = _mm_loadu_ps(fb.f32);
  __m128 vr = _mm_add_ps(va, vb);
  vec128_t o = {};
  _mm_storeu_ps(o.f32, vr);
  return o;
}

vec128_t RefVmxVsubfpImpl(const vec128_t& a, const vec128_t& b) {
  const vec128_t fa = RefVmxFlushDenormsOnly(a);
  const vec128_t fb = RefVmxFlushDenormsOnly(b);
  __m128 va = _mm_loadu_ps(fa.f32);
  __m128 vb = _mm_loadu_ps(fb.f32);
  __m128 vr = _mm_sub_ps(va, vb);
  vec128_t o = {};
  _mm_storeu_ps(o.f32, vr);
  return o;
}

vec128_t RefVmxVmaxfpImpl(const vec128_t& a, const vec128_t& b) {
  __m128 va = _mm_loadu_ps(a.f32);
  __m128 vb = _mm_loadu_ps(b.f32);
  __m128 t1 = _mm_max_ps(va, vb);
  __m128 t2 = _mm_max_ps(vb, va);
  __m128 vr = _mm_or_ps(t1, t2);
  vec128_t o = {};
  _mm_storeu_ps(o.f32, vr);
  return o;
}

vec128_t RefVmxVminfpImpl(const vec128_t& a, const vec128_t& b) {
  __m128 va = _mm_loadu_ps(a.f32);
  __m128 vb = _mm_loadu_ps(b.f32);
  __m128 t1 = _mm_min_ps(va, vb);
  __m128 t2 = _mm_min_ps(vb, va);
  __m128 vr = _mm_or_ps(t1, t2);
  vec128_t o = {};
  _mm_storeu_ps(o.f32, vr);
  return o;
}

vec128_t RefVmxVrfipImpl(const vec128_t& vb) {
  __m128 x = _mm_loadu_ps(vb.f32);
  __m128 r = _mm_round_ps(x, _MM_FROUND_TO_POS_INF | _MM_FROUND_NO_EXC);
  vec128_t o = {};
  _mm_storeu_ps(o.f32, r);
  return o;
}
#else
vec128_t RefVmxVaddfpImpl(const vec128_t& a, const vec128_t& b) {
  const vec128_t fa = RefVmxFlushDenormsOnly(a);
  const vec128_t fb = RefVmxFlushDenormsOnly(b);
  vec128_t o = {};
  for (int i = 0; i < 4; ++i) {
    o.f32[i] = fa.f32[i] + fb.f32[i];
  }
  return o;
}

vec128_t RefVmxVsubfpImpl(const vec128_t& a, const vec128_t& b) {
  const vec128_t fa = RefVmxFlushDenormsOnly(a);
  const vec128_t fb = RefVmxFlushDenormsOnly(b);
  vec128_t o = {};
  for (int i = 0; i < 4; ++i) {
    o.f32[i] = fa.f32[i] - fb.f32[i];
  }
  return o;
}

vec128_t RefVmxVmaxfpImpl(const vec128_t& a, const vec128_t& b) {
  const vec128_t fa = RefVmxFlushDenormsOnly(a);
  const vec128_t fb = RefVmxFlushDenormsOnly(b);
  vec128_t o = {};
  for (int i = 0; i < 4; ++i) {
    o.f32[i] = std::fmaxf(fa.f32[i], fb.f32[i]);
  }
  return o;
}

vec128_t RefVmxVminfpImpl(const vec128_t& a, const vec128_t& b) {
  const vec128_t fa = RefVmxFlushDenormsOnly(a);
  const vec128_t fb = RefVmxFlushDenormsOnly(b);
  vec128_t o = {};
  for (int i = 0; i < 4; ++i) {
    o.f32[i] = std::fminf(fa.f32[i], fb.f32[i]);
  }
  return o;
}

vec128_t RefVmxVrfipImpl(const vec128_t& vb) {
  const vec128_t fv = RefVmxFlushDenormsOnly(vb);
  vec128_t o = {};
  for (int i = 0; i < 4; ++i) {
    o.f32[i] = IsFloatNanU32(vb.u32[i]) ? vb.f32[i] : std::ceil(fv.f32[i]);
  }
  return o;
}
#endif

// HIR / PPC NaN propagation (vector_nan_propagation_test.cc).
uint32_t PpcVmxAddF32U32(uint32_t ua, uint32_t ub) {
  if (IsFloatNanU32(ua)) {
    return QuietNanU32(ua);
  }
  if (IsFloatNanU32(ub)) {
    return QuietNanU32(ub);
  }
  const float r = U32ToF32(ua) + U32ToF32(ub);
  uint32_t ur = F32ToU32(r);
  if (IsFloatNanU32(ur)) {
    return kPpcDefaultQNaN;
  }
  return ur;
}

uint32_t PpcVmxSubF32U32(uint32_t ua, uint32_t ub) {
  if (IsFloatNanU32(ua)) {
    return QuietNanU32(ua);
  }
  if (IsFloatNanU32(ub)) {
    return QuietNanU32(ub);
  }
  const float r = U32ToF32(ua) - U32ToF32(ub);
  uint32_t ur = F32ToU32(r);
  if (IsFloatNanU32(ur)) {
    return kPpcDefaultQNaN;
  }
  return ur;
}

uint32_t PpcVmxMaxF32U32(uint32_t ua, uint32_t ub) {
  if (IsFloatNanU32(ua)) {
    return QuietNanU32(ua);
  }
  if (IsFloatNanU32(ub)) {
    return QuietNanU32(ub);
  }
  const float r = std::fmaxf(U32ToF32(ua), U32ToF32(ub));
  uint32_t ur = F32ToU32(r);
  if (IsFloatNanU32(ur)) {
    return kPpcDefaultQNaN;
  }
  return ur;
}

uint32_t PpcVmxMinF32U32(uint32_t ua, uint32_t ub) {
  if (IsFloatNanU32(ua)) {
    return QuietNanU32(ua);
  }
  if (IsFloatNanU32(ub)) {
    return QuietNanU32(ub);
  }
  const float r = std::fminf(U32ToF32(ua), U32ToF32(ub));
  uint32_t ur = F32ToU32(r);
  if (IsFloatNanU32(ur)) {
    return kPpcDefaultQNaN;
  }
  return ur;
}

using PpcLaneU32Fn = uint32_t (*)(uint32_t, uint32_t);

vec128_t RefPpcLaneBinary(PpcLaneU32Fn lane, const vec128_t& a,
                          const vec128_t& b) {
  vec128_t o = {};
  for (int i = 0; i < 4; ++i) {
    o.u32[i] = lane(a.u32[i], b.u32[i]);
  }
  return o;
}

}  // namespace

vec128_t RefVmxFlushDenormsOnly(const vec128_t& v) {
  vec128_t o = v;
  for (int i = 0; i < 4; ++i) {
    uint32_t u = o.u32[i];
    if ((u & 0x7F800000u) != 0x7F800000u || (u & 0x7FFFFFu) == 0) {
      if ((u & 0x7F800000u) == 0 && (u & 0x7FFFFFu) != 0) {
        o.u32[i] = u & 0x80000000u;
      }
    }
  }
  return o;
}

vec128_t RefVectorDenormFlush(const vec128_t& v) {
  vec128_t o = v;
  for (int i = 0; i < 4; ++i) {
    uint32_t u = o.u32[i];
    if ((u & 0x7F800000u) == 0 && (u & 0x7FFFFFu) != 0) {
      o.u32[i] = u & 0x80000000u;
    }
  }
  return o;
}

vec128_t RefPpcVmxAddfp(const vec128_t& a, const vec128_t& b) {
  return RefPpcLaneBinary(PpcVmxAddF32U32, a, b);
}

vec128_t RefPpcVmxSubfp(const vec128_t& a, const vec128_t& b) {
  return RefPpcLaneBinary(PpcVmxSubF32U32, a, b);
}

vec128_t RefPpcVmxMaxfp(const vec128_t& a, const vec128_t& b) {
  return RefPpcLaneBinary(PpcVmxMaxF32U32, a, b);
}

vec128_t RefPpcVmxMinfp(const vec128_t& a, const vec128_t& b) {
  return RefPpcLaneBinary(PpcVmxMinF32U32, a, b);
}

vec128_t RefVmxVaddfp(const vec128_t& a, const vec128_t& b) {
  return RefVmxVaddfpImpl(a, b);
}

vec128_t RefVmxVsubfp(const vec128_t& a, const vec128_t& b) {
  return RefVmxVsubfpImpl(a, b);
}

vec128_t RefVmxVmaxfp(const vec128_t& a, const vec128_t& b) {
  return RefVmxVmaxfpImpl(a, b);
}

vec128_t RefVmxVminfp(const vec128_t& a, const vec128_t& b) {
  return RefVmxVminfpImpl(a, b);
}

vec128_t RefVmxVrfip(const vec128_t& vb) {
  return RefVmxVrfipImpl(vb);
}

vec128_t RefPermuteInt16(const vec128_t& control, const vec128_t& va,
                         const vec128_t& vb) {
  vec128_t perm = (control & vec128s(0xF)) ^ vec128s(0x1);
  vec128_t perm_ctrl = vec128b(0);
  for (int i = 0; i < 8; ++i) {
    perm_ctrl.i16[i] = perm.i16[i] > 7 ? static_cast<int16_t>(-1) : 0;
    const auto v = static_cast<uint8_t>(perm.u16[i]);
    perm.u8[i * 2] = static_cast<uint8_t>(v * 2);
    perm.u8[i * 2 + 1] = static_cast<uint8_t>(v * 2 + 1);
  }
  auto shuffle_bytes = [](const vec128_t& src, const vec128_t& idx,
                          vec128_t& out) {
    for (int i = 0; i < 16; ++i) {
      const uint8_t sel = idx.u8[i];
      out.u8[i] = (sel & 0x80) ? 0 : src.u8[sel & 0xF];
    }
  };
  vec128_t shuf1{};
  vec128_t shuf2{};
  shuffle_bytes(va, perm, shuf1);
  shuffle_bytes(vb, perm, shuf2);
  uint8_t mask = 0;
  for (int i = 0; i < 8; ++i) {
    if (perm_ctrl.i16[i] == 0) {
      mask |= static_cast<uint8_t>(1 << (7 - i));
    }
  }
  vec128_t o{};
  for (int i = 0; i < 8; ++i) {
    if (mask & (1 << i)) {
      o.u16[i] = shuf1.u16[i];
    } else {
      o.u16[i] = shuf2.u16[i];
    }
  }
  return o;
}

}  // namespace xe::vmx128_fuzz
