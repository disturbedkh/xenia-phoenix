/**
 ******************************************************************************
 * Guest debug print ABI (homebrew / observability v2).
 ******************************************************************************
 * Trap types 20 and 26: debug print.
 *   r3 = pointer to UTF-8 (or guest encoding) buffer
 *   r4 = byte length (not required to be NUL-terminated)
 * Host routes to obs::EmitGuestPrint + Guest.Print channel when preset allows.
 ******************************************************************************
 */
#pragma once

#include <cstdint>

// PPC trap immediates used by X64Emitter::Trap.
#define XENIA_GUEST_DEBUG_PRINT_TRAP_A 20u
#define XENIA_GUEST_DEBUG_PRINT_TRAP_B 26u

namespace xe {
namespace guest_debug {

struct GuestPrintArgs {
  uint32_t buffer_guest_ptr = 0;
  uint32_t length_bytes = 0;
  uint32_t link_register = 0;
};

// 4 KiB guest-host ring (homebrew). Guest writes at write_offset; host drains.
constexpr uint32_t kGuestDebugRingBytes = 4096u;
struct GuestDebugRing {
  uint32_t write_offset = 0;
  uint32_t read_offset = 0;
  char data[kGuestDebugRingBytes - sizeof(uint32_t) * 2] = {};
};

}  // namespace guest_debug
}  // namespace xe
