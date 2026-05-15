/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Tier 0 (MVP1): execute small guest PPC snippets (e.g. VMX) via the normal
 * PPC frontend + JIT path for differential / fuzz testing.
 ******************************************************************************
 */

#ifndef XENIA_CPU_TESTING_GUEST_PPC_TEST_UTIL_H_
#define XENIA_CPU_TESTING_GUEST_PPC_TEST_UTIL_H_

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include "xenia/cpu/ppc/ppc_context.h"

namespace xe {
class Memory;
namespace cpu {
class Processor;
class Function;
class ThreadState;
}  // namespace cpu
}  // namespace xe

namespace xe {
namespace cpu {
namespace testing {

// VX-form AltiVec (primary opcode 4). `xo_low11` matches the low 11 bits used in
// `ppc_opcode_table_gen.cc` (e.g. vadduhm = 0x040).
inline uint32_t EncodeVxAltivec(uint32_t vd, uint32_t va, uint32_t vb,
                                uint32_t xo_low11) {
  return (4u << 26) | ((vd & 31u) << 21) | ((va & 31u) << 16) |
         ((vb & 31u) << 11) | (xo_low11 & 0x7FFu);
}

// Encode Altivec `vaddubm vd,va,vb` (5-bit classic VR indices 0-31).
inline uint32_t EncodeVaddubm(uint32_t vd, uint32_t va, uint32_t vb) {
  return EncodeVxAltivec(vd, va, vb, 0);
}

// XO constants (low 11 bits) for VMX integer add / subtract family.
constexpr uint32_t kVxXO_vaddubm = 0x000;
constexpr uint32_t kVxXO_vadduhm = 0x040;
constexpr uint32_t kVxXO_vadduwm = 0x080;
constexpr uint32_t kVxXO_vaddubs = 0x200;
constexpr uint32_t kVxXO_vadduhs = 0x240;
constexpr uint32_t kVxXO_vadduws = 0x280;
constexpr uint32_t kVxXO_vaddsbs = 0x300;
constexpr uint32_t kVxXO_vaddshs = 0x340;
constexpr uint32_t kVxXO_vaddsws = 0x380;
constexpr uint32_t kVxXO_vsububm = 0x400;
constexpr uint32_t kVxXO_vsubuhm = 0x440;
constexpr uint32_t kVxXO_vsubuwm = 0x480;
constexpr uint32_t kVxXO_vsububs = 0x600;
constexpr uint32_t kVxXO_vsubuhs = 0x640;
constexpr uint32_t kVxXO_vsubuws = 0x680;
constexpr uint32_t kVxXO_vsubsbs = 0x700;
constexpr uint32_t kVxXO_vsubshs = 0x740;
constexpr uint32_t kVxXO_vsubsws = 0x780;

constexpr uint32_t kPpcInsnBlr = 0x4E800020;

// Runs `guest_instructions` (PPC instruction words as shown in a BE disasm /
// objdump) at `entry_pc`, then `blr`. Typical input:
//   { EncodeVaddubm(5, 3, 4), kPpcInsnBlr }
class TestGuestPpcBlock {
 public:
  TestGuestPpcBlock();
  ~TestGuestPpcBlock();

  void Run(const std::vector<uint32_t>& guest_instructions,
           const std::function<void(ppc::PPCContext*)>& pre_call,
           const std::function<void(ppc::PPCContext*)>& post_call,
           uint32_t entry_pc = 0x80000000);

  Processor* processor() const { return processor_.get(); }
  Memory* memory() const { return memory_.get(); }

 private:
  void TeardownGuestIfPrepared();

  std::unique_ptr<Memory> memory_;
  std::unique_ptr<Processor> processor_;
  std::vector<uint32_t> prepared_guest_instructions_;
  uint32_t prepared_entry_pc_ = 0;
  bool guest_prepared_ = false;
  cpu::Function* guest_entry_fn_ = nullptr;

  // Reused guest stack for repeated Run() (fuzz / differential); avoids 64KiB
  // heap alloc+free per invocation.
  static constexpr uint32_t kGuestStackSize = 64 * 1024;
  uint32_t scratch_stack_address_ = 0;
  std::unique_ptr<ThreadState> thread_state_;
};

}  // namespace testing
}  // namespace cpu
}  // namespace xe

#endif  // XENIA_CPU_TESTING_GUEST_PPC_TEST_UTIL_H_
