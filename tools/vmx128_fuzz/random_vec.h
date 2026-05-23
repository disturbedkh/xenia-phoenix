/**
 ******************************************************************************
 * Random vector helpers for vmx128-fuzz.
 ******************************************************************************
 */
#ifndef XENIA_TOOLS_VMX128_FUZZ_RANDOM_VEC_H_
#define XENIA_TOOLS_VMX128_FUZZ_RANDOM_VEC_H_

#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <random>

#include "xenia/base/vec128.h"

namespace xe::vmx128_fuzz {

inline vec128_t RandomVec(std::mt19937& rng) {
  vec128_t v = {};
  for (int i = 0; i < 16; ++i) {
    v.u8[i] = static_cast<uint8_t>(rng());
  }
  return v;
}

inline vec128_t RandomFpVec(std::mt19937& rng) {
  vec128_t v = {};
  for (int i = 0; i < 4; ++i) {
    uint32_t u = static_cast<uint32_t>(rng());
    switch (u & 7u) {
      case 0:
        v.f32[i] = 0.f;
        break;
      case 1:
        v.f32[i] = -0.f;
        break;
      case 2:
        v.f32[i] = std::numeric_limits<float>::infinity();
        break;
      case 3:
        v.f32[i] = -std::numeric_limits<float>::infinity();
        break;
      case 4: {
        uint32_t qnan = 0x7fc00000u | (u >> 3);
        std::memcpy(&v.f32[i], &qnan, sizeof(float));
        break;
      }
      case 5: {
        uint32_t snan = 0x7f800001u | (u >> 3);
        std::memcpy(&v.f32[i], &snan, sizeof(float));
        break;
      }
      case 6: {
        uint32_t den = (u & 0x007fffffu) | 0x00000001u;
        std::memcpy(&v.f32[i], &den, sizeof(float));
        break;
      }
      default:
        v.f32[i] = std::ldexp(static_cast<float>(static_cast<int32_t>(u)), -96);
        break;
    }
  }
  return v;
}

}  // namespace xe::vmx128_fuzz

#endif  // XENIA_TOOLS_VMX128_FUZZ_RANDOM_VEC_H_
