/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 */

#include <algorithm>
#include <cstring>
#include <limits>

#include "xenia/cpu/testing/guest_ppc_test_util.h"

#include "xenia/base/assert.h"
#include "xenia/base/byte_order.h"
#include "xenia/base/math.h"
#include "xenia/base/platform.h"
#include "xenia/cpu/function.h"
#include "xenia/cpu/processor.h"
#include "xenia/cpu/raw_module.h"
#include "xenia/cpu/thread_state.h"
#include "xenia/memory.h"

#if XE_ARCH_AMD64
#include <immintrin.h>
#include "xenia/cpu/backend/x64/x64_backend.h"
#elif XE_ARCH_ARM64
#include "xenia/cpu/backend/a64/a64_backend.h"
#endif

namespace xe {
namespace cpu {
namespace testing {

TestGuestPpcBlock::TestGuestPpcBlock() {
  memory_ = std::make_unique<Memory>();
  memory_->Initialize();

  std::unique_ptr<backend::Backend> backend;
#if XE_ARCH_AMD64
  backend = std::make_unique<backend::x64::X64Backend>();
#elif XE_ARCH_ARM64
  backend = std::make_unique<backend::a64::A64Backend>();
#endif
  assert_not_null(backend);

  processor_ = std::make_unique<Processor>(memory_.get(), nullptr);
  processor_->Setup(std::move(backend));
}

void TestGuestPpcBlock::EnsureReferenceThreadState() const {
  auto* self = const_cast<TestGuestPpcBlock*>(this);
  if (!self->scratch_stack_address_) {
    self->scratch_stack_address_ = memory_->SystemHeapAlloc(kGuestStackSize);
  }
  const uint32_t stack_base = self->scratch_stack_address_ + kGuestStackSize;
  if (!self->thread_state_) {
    self->thread_state_ =
        std::make_unique<ThreadState>(processor_.get(), 0x100, stack_base);
  }
}

float TestGuestPpcBlock::ReferenceVrsqrtefpScalar(float x) const {
#if XE_ARCH_AMD64
  auto* x64 = static_cast<backend::x64::X64Backend*>(processor_->backend());
  if (!x64 || !x64->vrsqrtefp_scalar_invoke_helper_) {
    return std::numeric_limits<float>::quiet_NaN();
  }
  EnsureReferenceThreadState();
  auto* ctx = thread_state_->context();
  processor_->backend()->SetGuestRoundingMode(ctx, static_cast<int>(ctx->fpscr.bits.rn));
  using Fn = float (*)(void*, float);
  return reinterpret_cast<Fn>(x64->vrsqrtefp_scalar_invoke_helper_)(ctx, x);
#else
  (void)x;
  return 0.f;
#endif
}

vec128_t TestGuestPpcBlock::ReferenceVrsqrtefpVector(const vec128_t& vb) const {
  if (vb.u32[0] == vb.u32[1] && vb.u32[0] == vb.u32[2] && vb.u32[0] == vb.u32[3]) {
    const float s = ReferenceVrsqrtefpScalar(vb.f32[0]);
    return vec128f(s, s, s, s);
  }
  if (vb.u32[0] == 0 && vb.u32[1] == 0 && vb.u32[2] == 0) {
    const float w = ReferenceVrsqrtefpScalar(vb.f32[3]);
    const float inf = std::numeric_limits<float>::infinity();
    return vec128f(inf, inf, inf, w);
  }
  vec128_t o = {};
  for (int i = 0; i < 4; ++i) {
    o.f32[i] = ReferenceVrsqrtefpScalar(vb.f32[i]);
  }
  return o;
}

TestGuestPpcBlock::~TestGuestPpcBlock() {
  TeardownGuestIfPrepared();
  if (scratch_stack_address_) {
    memory_->SystemHeapFree(scratch_stack_address_);
    scratch_stack_address_ = 0;
  }
  processor_.reset();
  memory_.reset();
}

void TestGuestPpcBlock::ReleaseCompiledGuest() { TeardownGuestIfPrepared(); }

void TestGuestPpcBlock::TeardownGuestIfPrepared() {
  if (!guest_prepared_) {
    return;
  }
  thread_state_.reset();
  processor_->RemoveModule("guest_ppc_test");
  processor_->RemoveFunctionByAddress(prepared_entry_pc_);
  prepared_guest_instructions_.clear();
  guest_entry_fn_ = nullptr;
  guest_prepared_ = false;
}

void TestGuestPpcBlock::Run(
    const std::vector<uint32_t>& guest_instructions,
    const std::function<void(ppc::PPCContext*)>& pre_call,
    const std::function<void(ppc::PPCContext*)>& post_call, uint32_t entry_pc,
    int rounding_mode) {
  assert_true(!guest_instructions.empty());

  const bool same_guest =
      guest_prepared_ && entry_pc == prepared_entry_pc_ &&
      guest_instructions.size() == prepared_guest_instructions_.size() &&
      std::equal(guest_instructions.begin(), guest_instructions.end(),
                 prepared_guest_instructions_.begin());

  if (!same_guest) {
    TeardownGuestIfPrepared();

    const uint32_t code_bytes =
        static_cast<uint32_t>(guest_instructions.size() * sizeof(uint32_t));
    const uint32_t range_size =
        xe::round_up(code_bytes, static_cast<uint32_t>(4096));
    const uint32_t range_end = entry_pc + range_size;

    uint8_t* p = memory_->TranslateVirtual(entry_pc);
    for (size_t i = 0; i < guest_instructions.size(); ++i) {
      xe::store_and_swap<uint32_t>(p + i * sizeof(uint32_t), guest_instructions[i]);
    }

    processor_->backend()->CommitExecutableRange(entry_pc, range_end);

    auto raw = std::make_unique<RawModule>(processor_.get());
    raw->set_name("guest_ppc_test");
    raw->set_executable(true);
    raw->SetAddressRange(entry_pc, range_size);
    processor_->AddModule(std::move(raw));

    guest_entry_fn_ = processor_->ResolveFunction(entry_pc);
    assert_not_null(guest_entry_fn_);

    prepared_guest_instructions_ = guest_instructions;
    prepared_entry_pc_ = entry_pc;
    guest_prepared_ = true;
  }

  if (!scratch_stack_address_) {
    scratch_stack_address_ = memory_->SystemHeapAlloc(kGuestStackSize);
  }
  const uint32_t stack_base = scratch_stack_address_ + kGuestStackSize;
  if (!thread_state_) {
    thread_state_ =
        std::make_unique<ThreadState>(processor_.get(), 0x100, stack_base);
  }
  auto* ctx = thread_state_->context();
  ctx->lr = 0xBCBCBCBC;

  pre_call(ctx);
  int rn = rounding_mode;
  if (rn < 0) {
    rn = GuestPpcFuzzRoundingMode();
  }
  if (rn < 0) {
    rn = static_cast<int>(ctx->fpscr.bits.rn);
  }
  ctx->fpscr.bits.rn = static_cast<uint32_t>(rn);
  processor_->backend()->SetGuestRoundingMode(ctx, rn);
  guest_entry_fn_->Call(thread_state_.get(), uint32_t(ctx->lr));
  post_call(ctx);
}

}  // namespace testing
}  // namespace cpu
}  // namespace xe
