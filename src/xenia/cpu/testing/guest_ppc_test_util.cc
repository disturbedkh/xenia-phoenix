/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 */

#include "xenia/cpu/testing/guest_ppc_test_util.h"

#include "xenia/base/assert.h"
#include "xenia/base/byte_order.h"
#include "xenia/base/math.h"
#include "xenia/base/platform.h"
#include "xenia/cpu/processor.h"
#include "xenia/cpu/raw_module.h"
#include "xenia/cpu/thread_state.h"
#include "xenia/memory.h"

#if XE_ARCH_AMD64
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

TestGuestPpcBlock::~TestGuestPpcBlock() {
  processor_.reset();
  memory_.reset();
}

void TestGuestPpcBlock::Run(
    const std::vector<uint32_t>& guest_instructions,
    const std::function<void(ppc::PPCContext*)>& pre_call,
    const std::function<void(ppc::PPCContext*)>& post_call, uint32_t entry_pc) {
  assert_true(!guest_instructions.empty());

  // Tear down any prior guest module / entry for this PC (supports reuse).
  processor_->RemoveModule("guest_ppc_test");
  processor_->RemoveFunctionByAddress(entry_pc);

  const uint32_t code_bytes =
      static_cast<uint32_t>(guest_instructions.size() * sizeof(uint32_t));
  const uint32_t range_size = xe::round_up(code_bytes, static_cast<uint32_t>(4096));
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

  auto* fn = processor_->ResolveFunction(entry_pc);
  assert_not_null(fn);

  uint32_t stack_size = 64 * 1024;
  uint32_t stack_address = memory_->SystemHeapAlloc(stack_size);
  uint32_t stack_base = stack_address + stack_size;
  ThreadState thread_state(processor_.get(), 0x100, stack_base);
  auto* ctx = thread_state.context();
  ctx->lr = 0xBCBCBCBC;
  processor_->backend()->SetGuestRoundingMode(ctx, 0);

  pre_call(ctx);
  fn->Call(&thread_state, uint32_t(ctx->lr));
  post_call(ctx);

  memory_->SystemHeapFree(stack_address);

  processor_->RemoveModule("guest_ppc_test");
  processor_->RemoveFunctionByAddress(entry_pc);
}

}  // namespace testing
}  // namespace cpu
}  // namespace xe
