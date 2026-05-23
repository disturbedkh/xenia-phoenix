#include <cstdio>
#include \"xenia/cpu/backend/x64/x64_backend.h\"
int main() {
  struct F { xe::cpu::backend::x64::X64BackendContext backend{}; uint8_t anchor{}; };
  F f{};
  auto* bctx = reinterpret_cast<xe::cpu::backend::x64::X64BackendContext*>(
      reinterpret_cast<intptr_t>(&f.anchor) - sizeof(xe::cpu::backend::x64::X64BackendContext));
  printf(\"sizeof backend=%zu anchor=%p backend=%p match=%d\n\",
    sizeof(xe::cpu::backend::x64::X64BackendContext), (void*)&f.anchor, (void*)&f.backend,
    bctx == &f.backend ? 1 : 0);
  return 0;
}
