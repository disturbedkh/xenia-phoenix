/**
 ******************************************************************************
 * Patch VM/VMX128 instruction words with concrete VR indices (matches
 * xenia/cpu/ppc/ppc_instr.h PPCOpcodeBits layout).
 ******************************************************************************
 */
#ifndef XENIA_TOOLS_VMX128_FUZZ_ENCODE_PATCH_H_
#define XENIA_TOOLS_VMX128_FUZZ_ENCODE_PATCH_H_

#include <cstdint>

#include "xenia/cpu/ppc/ppc_instr.h"

namespace xe::vmx128_fuzz {

inline uint32_t PatchVx(uint32_t insn, uint32_t vd, uint32_t va, uint32_t vb) {
  cpu::ppc::PPCOpcodeBits b{};
  b.code = insn;
  b.VX.VD = vd & 31u;
  b.VX.VA = va & 31u;
  b.VX.VB = vb & 31u;
  return b.code;
}

inline uint32_t PatchVxa(uint32_t insn, uint32_t vd, uint32_t va, uint32_t vb,
                         uint32_t vc) {
  cpu::ppc::PPCOpcodeBits b{};
  b.code = insn;
  b.VXA.VD = vd & 31u;
  b.VXA.VA = va & 31u;
  b.VXA.VB = vb & 31u;
  b.VXA.VC = vc & 31u;
  return b.code;
}

inline uint32_t PatchVxr(uint32_t insn, uint32_t vd, uint32_t va, uint32_t vb,
                         bool rc) {
  cpu::ppc::PPCOpcodeBits b{};
  b.code = insn;
  b.VXR.VD = vd & 31u;
  b.VXR.VA = va & 31u;
  b.VXR.VB = vb & 31u;
  b.VXR.Rc = rc ? 1u : 0u;
  return b.code;
}

inline uint32_t PatchVx128(uint32_t insn, uint32_t vd, uint32_t va,
                           uint32_t vb) {
  cpu::ppc::PPCOpcodeBits b{};
  b.code = insn;
  b.VX128.VD128l = vd & 31u;
  b.VX128.VD128h = (vd >> 5) & 3u;
  b.VX128.VA128l = va & 31u;
  b.VX128.VA128h = (va >> 5) & 1u;
  b.VX128.VA128H = (va >> 6) & 1u;
  b.VX128.VB128l = vb & 31u;
  b.VX128.VB128h = (vb >> 5) & 3u;
  return b.code;
}

inline uint32_t PatchVx128_2(uint32_t insn, uint32_t vd, uint32_t va,
                             uint32_t vb, uint32_t vc_field3 = 0) {
  cpu::ppc::PPCOpcodeBits b{};
  b.code = insn;
  b.VX128_2.VD128l = vd & 31u;
  b.VX128_2.VD128h = (vd >> 5) & 3u;
  b.VX128_2.VA128l = va & 31u;
  b.VX128_2.VA128h = (va >> 5) & 1u;
  b.VX128_2.VA128H = (va >> 6) & 1u;
  b.VX128_2.VB128l = vb & 31u;
  b.VX128_2.VB128h = (vb >> 5) & 3u;
  b.VX128_2.VC = vc_field3 & 7u;
  return b.code;
}

inline uint32_t PatchVx128_3(uint32_t insn, uint32_t vd, uint32_t vb,
                             uint32_t imm5) {
  cpu::ppc::PPCOpcodeBits b{};
  b.code = insn;
  b.VX128_3.VD128l = vd & 31u;
  b.VX128_3.VD128h = (vd >> 5) & 3u;
  b.VX128_3.VB128l = vb & 31u;
  b.VX128_3.VB128h = (vb >> 5) & 3u;
  b.VX128_3.IMM = imm5 & 31u;
  return b.code;
}

inline uint32_t PatchVx128_4(uint32_t insn, uint32_t vd, uint32_t vb,
                             uint32_t imm5, uint32_t z) {
  cpu::ppc::PPCOpcodeBits b{};
  b.code = insn;
  b.VX128_4.VD128l = vd & 31u;
  b.VX128_4.VD128h = (vd >> 5) & 3u;
  b.VX128_4.VB128l = vb & 31u;
  b.VX128_4.VB128h = (vb >> 5) & 3u;
  b.VX128_4.IMM = imm5 & 31u;
  b.VX128_4.z = z & 3u;
  return b.code;
}

inline uint32_t PatchVx128_5(uint32_t insn, uint32_t vd, uint32_t va,
                               uint32_t vb, uint32_t sh4) {
  cpu::ppc::PPCOpcodeBits b{};
  b.code = insn;
  b.VX128_5.VD128l = vd & 31u;
  b.VX128_5.VD128h = (vd >> 5) & 3u;
  b.VX128_5.VA128l = va & 31u;
  b.VX128_5.VA128h = (va >> 5) & 1u;
  b.VX128_5.VA128H = (va >> 6) & 1u;
  b.VX128_5.VB128l = vb & 31u;
  b.VX128_5.VB128h = (vb >> 5) & 3u;
  b.VX128_5.SH = sh4 & 15u;
  return b.code;
}

inline uint32_t PatchVx128_P(uint32_t insn, uint32_t vd, uint32_t vb,
                             uint32_t perm6) {
  cpu::ppc::PPCOpcodeBits b{};
  b.code = insn;
  b.VX128_P.VD128l = vd & 31u;
  b.VX128_P.VD128h = (vd >> 5) & 3u;
  b.VX128_P.VB128l = vb & 31u;
  b.VX128_P.VB128h = (vb >> 5) & 3u;
  b.VX128_P.PERMl = perm6 & 31u;
  b.VX128_P.PERMh = (perm6 >> 5) & 7u;
  return b.code;
}

inline uint32_t PatchVx128_R(uint32_t insn, uint32_t vd, uint32_t va,
                             uint32_t vb, bool rc) {
  cpu::ppc::PPCOpcodeBits b{};
  b.code = insn;
  b.VX128_R.VD128l = vd & 31u;
  b.VX128_R.VD128h = (vd >> 5) & 3u;
  b.VX128_R.VA128l = va & 31u;
  b.VX128_R.VA128h = (va >> 5) & 1u;
  b.VX128_R.VA128H = (va >> 6) & 1u;
  b.VX128_R.VB128l = vb & 31u;
  b.VX128_R.VB128h = (vb >> 5) & 3u;
  b.VX128_R.Rc = rc ? 1u : 0u;
  return b.code;
}

}  // namespace xe::vmx128_fuzz

#endif  // XENIA_TOOLS_VMX128_FUZZ_ENCODE_PATCH_H_
