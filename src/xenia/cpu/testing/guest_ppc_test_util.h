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
}  // namespace cpu
}  // namespace xe

namespace xe {
namespace cpu {
namespace testing {

// Encode Altivec `vaddubm vd,va,vb` (5-bit classic VR indices 0-31).
inline uint32_t EncodeVaddubm(uint32_t vd, uint32_t va, uint32_t vb) {
  return (4u << 26) | ((vd & 31u) << 21) | ((va & 31u) << 16) |
         ((vb & 31u) << 11);
}

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
  std::unique_ptr<Memory> memory_;
  std::unique_ptr<Processor> processor_;
};

}  // namespace testing
}  // namespace cpu
}  // namespace xe

#endif  // XENIA_CPU_TESTING_GUEST_PPC_TEST_UTIL_H_
