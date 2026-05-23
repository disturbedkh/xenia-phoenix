/**
 ******************************************************************************
 * Guest debug ring buffer drain (homebrew observability).
 ******************************************************************************
 */

#include "xenia/base/obs/obs.h"

#include <atomic>

namespace xe {
namespace obs {
namespace {

std::atomic<uint32_t> g_guest_ring_ptr{0};

}  // namespace

void RegisterGuestDebugRing(uint32_t guest_ptr) {
  g_guest_ring_ptr.store(guest_ptr, std::memory_order_relaxed);
}

void TryDrainGuestDebugRing() {
  // Drain requires guest memory mapping from the CPU backend; homebrew titles
  // call RegisterGuestDebugRing once at init. Per-line Guest.Print traps remain
  // the primary path until a memory callback is wired here.
  (void)g_guest_ring_ptr.load(std::memory_order_relaxed);
}

}  // namespace obs
}  // namespace xe
