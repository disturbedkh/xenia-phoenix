/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2021 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <mutex>

#include "xenia/base/console.h"
#include "xenia/base/cvar.h"
#include "xenia/base/logging.h"
#include "xenia/base/main_win.h"
#include "xenia/base/platform_win.h"
#include "xenia/base/threading.h"
#include "xenia/kernel/kernel_state.cc"
#include "xenia/ui/windowed_app.h"
#include "xenia/ui/windowed_app_context_win.h"

#include "version.h"

#include <DbgHelp.h>
#include <Psapi.h>
#pragma comment(lib, "dbghelp.lib")

DEFINE_bool(enable_console, false, "Open a console window with the main window",
            "Logging");

static uintptr_t g_xenia_exe_base = 0;
static size_t g_xenia_exe_size = 0;

static HMODULE probe_for_module(void* addr) {
  // get 65k aligned addr downwards to probe for MZ
  uintptr_t base = reinterpret_cast<uintptr_t>(addr) & ~0xFFFFULL;

  constexpr unsigned max_search_iters =
      (64 * (1024 * 1024)) /
      65536;  // search down at most 64 mb (we do it in
              // batches of 64k so its pretty quick). i think its reasonable to
              // expect no module will be > 64mb
  // break if access violation thrown, we're definitely not a PE in that case
  __try {
    for (unsigned i = 0; i < max_search_iters; ++i) {
      if (*reinterpret_cast<unsigned short*>(base) == 'ZM') {
        return reinterpret_cast<HMODULE>(base);
      } else {
        base -= 65536;
      }
    }
    return nullptr;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return nullptr;
  }
}

static constexpr auto XENIA_ERROR_LANGUAGE =
    MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT);

struct HostExceptionReport {
  _EXCEPTION_POINTERS* const ExceptionInfo;
  size_t Report_Scratchpos;

  const DWORD last_win32_error;
  const NTSTATUS last_ntstatus;

  const int errno_value;
  // Increased from 2 KiB to make room for a symbolized backtrace appended by
  // stack_trace_handle. AddString clamps writes to this buffer.
  char Report_Scratchbuffer[16384];

  unsigned int address_format_ring_index;

  char formatted_addresses[16][128];

  void AddString(const char* s);
  static char* ChompNewlines(char* s);

  HostExceptionReport(_EXCEPTION_POINTERS* _ExceptionInfo)
      : ExceptionInfo(_ExceptionInfo),
        Report_Scratchpos(0u),
        last_win32_error(GetLastError()),
#if XE_ARCH_AMD64
        last_ntstatus(__readgsdword(0x1250)),
#elif XE_ARCH_ARM64
        // TEB.LastStatusValue at offset 0x1250 (same as x64).
        // ARM64 uses x18 register for TEB base instead of GS segment.
        last_ntstatus(__readx18dword(0x1250)),
#endif
        errno_value(errno),
        address_format_ring_index(0)

  {
    memset(Report_Scratchbuffer, 0, sizeof(Report_Scratchbuffer));
  }

  void DisplayExceptionMessage() {
    MessageBoxA(nullptr, Report_Scratchbuffer, "Unhandled Exception in Xenia",
                MB_ICONERROR);
  }

  const char* GetFormattedAddress(uintptr_t address);

  const char* GetFormattedAddress(PVOID address) {
    return GetFormattedAddress(reinterpret_cast<uintptr_t>(address));
  }
};
char* HostExceptionReport::ChompNewlines(char* s) {
  if (!s) {
    return nullptr;
  }
  unsigned read_pos = 0;
  unsigned write_pos = 0;

  while (true) {
    char current = s[read_pos++];
    if (current == '\n') {
      continue;
    }
    s[write_pos++] = current;
    if (!current) {
      break;
    }
  }
  return s;
}
void HostExceptionReport::AddString(const char* s) {
  size_t ln = strlen(s);
  // Always reserve one byte for the terminating NUL so callers that print the
  // scratch buffer via %s won't read past the end.
  const size_t kCap = sizeof(Report_Scratchbuffer) - 1;
  if (Report_Scratchpos >= kCap) {
    return;
  }
  const size_t copy = std::min(ln, kCap - Report_Scratchpos);
  for (size_t i = 0; i < copy; ++i) {
    Report_Scratchbuffer[i + Report_Scratchpos] = s[i];
  }
  Report_Scratchpos += copy;
  Report_Scratchbuffer[Report_Scratchpos] = '\0';
}

const char* HostExceptionReport::GetFormattedAddress(uintptr_t address) {
  char(&current_buffer)[128] =
      formatted_addresses[address_format_ring_index++ % 16];

  /* if (address >= g_xenia_exe_base &&
       address - g_xenia_exe_base < g_xenia_exe_size) {
     uintptr_t offset = address - g_xenia_exe_base;

     sprintf_s(current_buffer, "xenia_canary.exe+%llX", offset);
   } else */
  {
    HMODULE hmod_for = probe_for_module((void*)address);

    if (hmod_for) {
      // get the module filename, then chomp off all but the actual file name
      // (full path is obtained)
      char tmp_module_name[MAX_PATH + 1];
      GetModuleFileNameA(hmod_for, tmp_module_name, sizeof(tmp_module_name));

      size_t search_back = strlen(tmp_module_name);
      // hunt backwards for the last sep
      while (tmp_module_name[--search_back] != '\\');

      // MessageBoxA(nullptr, tmp_module_name, "ffds", MB_OK);
      sprintf_s(current_buffer, "%s+%llX", tmp_module_name + search_back + 1,
                address - reinterpret_cast<uintptr_t>(hmod_for));

    } else {
      sprintf_s(current_buffer, "0x%llX", address);
    }
  }
  return current_buffer;
}
using ExceptionInfoCategoryHandler = bool (*)(HostExceptionReport* report);
static char* Ntstatus_msg(NTSTATUS status) {
  char* statusmsg = nullptr;
  FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE |
                     FORMAT_MESSAGE_IGNORE_INSERTS,
                 GetModuleHandleA("ntdll.dll"), status, XENIA_ERROR_LANGUAGE,
                 (LPSTR)&statusmsg, 0, NULL);
  return statusmsg;
}
static bool exception_pointers_handler(HostExceptionReport* report) {
  PVOID exception_addr =
      report->ExceptionInfo->ExceptionRecord->ExceptionAddress;

#if XE_ARCH_AMD64
  DWORD64 last_stackpointer = report->ExceptionInfo->ContextRecord->Rsp;
  DWORD64 last_rip = report->ExceptionInfo->ContextRecord->Rip;
#elif XE_ARCH_ARM64
  DWORD64 last_stackpointer = report->ExceptionInfo->ContextRecord->Sp;
  DWORD64 last_rip = report->ExceptionInfo->ContextRecord->Pc;
#endif
  DWORD except_code = report->ExceptionInfo->ExceptionRecord->ExceptionCode;

  std::string build = (
#ifdef XE_BUILD_IS_PR
      "PR#" XE_BUILD_PR_NUMBER " - "
#endif
      XE_BUILD_BRANCH "@" XE_BUILD_COMMIT_SHORT " on " XE_BUILD_DATE);

  std::string title_info = "Title not started yet.";

  if (xe::kernel::kernel_state()) {
    if (xe::kernel::kernel_state()->emulator()->is_title_open()) {
      const uint32_t title_id = xe::kernel::kernel_state()->title_id();
      const std::string title_name =
          xe::kernel::kernel_state()->emulator()->title_name();
      const std::string title_version =
          xe::kernel::kernel_state()->emulator()->title_version();

      title_info =
          fmt::format("{} ({:08X}) - {}", title_name, title_id, title_version);
    }
  }

  const std::string except_message = fmt::format(
      "Exception encountered!\nTitle Info: {}\nBuild: {}\nException address: "
      "{}\nStackpointer: {}\nInstruction pointer: {}\nExceptionCode: 0x{} "
      "({})\n",
      title_info, build.c_str(), report->GetFormattedAddress(exception_addr),
      report->GetFormattedAddress(last_stackpointer),
      report->GetFormattedAddress(last_rip), except_code,
      HostExceptionReport::ChompNewlines(Ntstatus_msg(except_code)));

  report->AddString(except_message.c_str());

  return true;
}

static bool exception_win32_error_handle(HostExceptionReport* report) {
  if (!report->last_win32_error) {
    return false;  // no error, nothing to do
  }
  // todo: formatmessage
  char win32_error_buf[512];
  // its ok if we dont free statusmsg, we're exiting anyway
  char* statusmsg = nullptr;
  FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                     FORMAT_MESSAGE_IGNORE_INSERTS,
                 nullptr, report->last_win32_error, XENIA_ERROR_LANGUAGE,
                 (LPSTR)&statusmsg, 0, nullptr);
  sprintf_s(win32_error_buf, "Last Win32 Error: 0x%X (%s)\n",
            report->last_win32_error,
            HostExceptionReport::ChompNewlines(statusmsg));
  report->AddString(win32_error_buf);
  return true;
}
static bool exception_ntstatus_error_handle(HostExceptionReport* report) {
  if (!report->last_ntstatus) {
    return false;
  }
  // todo: formatmessage
  char win32_error_buf[512];

  sprintf_s(win32_error_buf, "Last NTSTATUS: 0x%X (%s)\n",
            report->last_ntstatus, Ntstatus_msg(report->last_ntstatus));
  report->AddString(win32_error_buf);
  return true;
}

static bool exception_cerror_handle(HostExceptionReport* report) {
  if (!report->errno_value) {
    return false;
  }
  char errno_buffer[512];
  sprintf_s(errno_buffer, "Last errno value: 0x%X (%s)\n", report->errno_value,
            strerror(report->errno_value));

  report->AddString(errno_buffer);
  return true;
}

// Emit a symbolized backtrace of the faulting thread. Frames are resolved via
// DbgHelp using whatever PDBs are reachable from the standard symbol search
// path; if symbols are unavailable we still get module+RVA which is enough to
// pinpoint the call site post-mortem with the matching PDB.
static bool stack_trace_handle(HostExceptionReport* report) {
  static std::atomic<bool> sym_initialized{false};
  static std::mutex sym_mutex;
  HANDLE process = GetCurrentProcess();
  {
    std::lock_guard lock(sym_mutex);
    if (!sym_initialized.load(std::memory_order_acquire)) {
      SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME |
                    SYMOPT_FAIL_CRITICAL_ERRORS);
      if (SymInitialize(process, nullptr, TRUE)) {
        sym_initialized.store(true, std::memory_order_release);
      }
    }
  }

  CONTEXT ctx = *report->ExceptionInfo->ContextRecord;

  STACKFRAME64 frame = {};
  DWORD machine_type = 0;
#if XE_ARCH_AMD64
  machine_type = IMAGE_FILE_MACHINE_AMD64;
  frame.AddrPC.Offset = ctx.Rip;
  frame.AddrPC.Mode = AddrModeFlat;
  frame.AddrFrame.Offset = ctx.Rbp;
  frame.AddrFrame.Mode = AddrModeFlat;
  frame.AddrStack.Offset = ctx.Rsp;
  frame.AddrStack.Mode = AddrModeFlat;
#elif XE_ARCH_ARM64
  machine_type = IMAGE_FILE_MACHINE_ARM64;
  frame.AddrPC.Offset = ctx.Pc;
  frame.AddrPC.Mode = AddrModeFlat;
  frame.AddrFrame.Offset = ctx.Fp;
  frame.AddrFrame.Mode = AddrModeFlat;
  frame.AddrStack.Offset = ctx.Sp;
  frame.AddrStack.Mode = AddrModeFlat;
#else
  return false;
#endif

  report->AddString("\n--- backtrace ---\n");

  HANDLE thread = GetCurrentThread();
  constexpr int kMaxFrames = 48;
  std::lock_guard lock(sym_mutex);  // DbgHelp APIs are not thread-safe.
  for (int i = 0; i < kMaxFrames; ++i) {
    if (!StackWalk64(machine_type, process, thread, &frame, &ctx, nullptr,
                     SymFunctionTableAccess64, SymGetModuleBase64, nullptr)) {
      break;
    }
    if (!frame.AddrPC.Offset) {
      break;
    }

    char line_buf[1024];
    const uintptr_t pc = static_cast<uintptr_t>(frame.AddrPC.Offset);
    const char* formatted_pc = report->GetFormattedAddress(pc);

    alignas(SYMBOL_INFO) char sym_storage[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
    SYMBOL_INFO* sym = reinterpret_cast<SYMBOL_INFO*>(sym_storage);
    memset(sym, 0, sizeof(SYMBOL_INFO));
    sym->SizeOfStruct = sizeof(SYMBOL_INFO);
    sym->MaxNameLen = MAX_SYM_NAME;

    DWORD64 displacement = 0;
    const bool have_sym =
        SymFromAddr(process, frame.AddrPC.Offset, &displacement, sym) != FALSE;

    IMAGEHLP_LINE64 line = {};
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
    DWORD line_displacement = 0;
    const bool have_line =
        SymGetLineFromAddr64(process, frame.AddrPC.Offset, &line_displacement,
                             &line) != FALSE;

    if (have_sym && have_line) {
      sprintf_s(line_buf, "#%02d %s  %s+0x%llX  (%s:%lu)\n", i, formatted_pc,
                sym->Name, static_cast<unsigned long long>(displacement),
                line.FileName, line.LineNumber);
    } else if (have_sym) {
      sprintf_s(line_buf, "#%02d %s  %s+0x%llX\n", i, formatted_pc, sym->Name,
                static_cast<unsigned long long>(displacement));
    } else {
      sprintf_s(line_buf, "#%02d %s\n", i, formatted_pc);
    }
    report->AddString(line_buf);

    if (report->Report_Scratchpos >
        sizeof(report->Report_Scratchbuffer) - 256) {
      report->AddString("<truncated>\n");
      break;
    }
  }
  return true;
}

static bool thread_name_handle(HostExceptionReport* report) {
  // ll GetThreadDescription(HANDLE hThread, PWSTR *ppszThreadDescription)

  FARPROC description_getter =
      GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetThreadDescription");

  if (!description_getter) {
    return false;
  }
  PWSTR descr = nullptr;

  reinterpret_cast<HRESULT (*)(HANDLE, PWSTR*)>(description_getter)(
      GetCurrentThread(), &descr);

  if (!descr) {
    return false;
  }

  char result_buffer[512];

  sprintf_s(result_buffer, "Faulting thread name: %ws\n", descr);

  report->AddString(result_buffer);
  return true;
}
static ExceptionInfoCategoryHandler host_exception_category_handlers[] = {
    exception_pointers_handler,
    exception_win32_error_handle,
    exception_ntstatus_error_handle,
    exception_cerror_handle,
    thread_name_handle,
    stack_trace_handle};

LONG _UnhandledExceptionFilter(_EXCEPTION_POINTERS* ExceptionInfo) {
  HostExceptionReport report{ExceptionInfo};
  for (auto&& handler : host_exception_category_handlers) {
    __try {
      if (!handler(&report)) {
        continue;
      }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
      report.AddString("<Nested Exception Encountered>\n");
    }
  }

  XELOGE("[UNHANDLED EXCEPTION]\n{}", report.Report_Scratchbuffer);
  xe::FlushLog();
  xe::logging::WriteCrashSidecar("unhandled", report.Report_Scratchbuffer,
                                 ExceptionInfo);
  xe::FlushLog();

  report.DisplayExceptionMessage();

  return EXCEPTION_CONTINUE_SEARCH;
}
int WINAPI wWinMain(HINSTANCE hinstance, HINSTANCE hinstance_prev,
                    LPWSTR command_line, int show_cmd) {
  MODULEINFO modinfo;

  GetModuleInformation(GetCurrentProcess(), (HMODULE)hinstance, &modinfo,
                       sizeof(MODULEINFO));

  g_xenia_exe_base = reinterpret_cast<uintptr_t>(hinstance);
  g_xenia_exe_size = modinfo.SizeOfImage;

  // Name the main/UI thread so an unhandled exception filter can identify it
  // in crash sidecars. Without this the "Faulting thread name:" field is
  // empty, which made several lifecycle crashes hard to triage.
  xe::threading::set_name("MainUI");

  int result;
  SetUnhandledExceptionFilter(_UnhandledExceptionFilter);
  {
    xe::ui::Win32WindowedAppContext app_context(hinstance, show_cmd);
    // TODO(Triang3l): Initialize creates a window. Set DPI awareness via the
    // manifest.
    if (!app_context.Initialize()) {
      return EXIT_FAILURE;
    }

    std::unique_ptr<xe::ui::WindowedApp> app =
        xe::ui::GetWindowedAppCreator()(app_context);

    if (!xe::ParseWin32LaunchArguments(false, app->GetPositionalOptionsUsage(),
                                       app->GetPositionalOptions(), nullptr)) {
      return EXIT_FAILURE;
    }

    // Initialize COM on the UI thread with the apartment-threaded concurrency
    // model, so dialogs can be used.
    if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))) {
      return EXIT_FAILURE;
    }

    xe::InitializeWin32App(app->GetName());

    if (app->OnInitialize()) {
      // TODO(Triang3l): Rework this, need to initialize the console properly,
      // disable has_console_attached_ by default in windowed apps, and attach
      // only if needed.
      if (cvars::enable_console) {
        xe::AttachConsole();
      }
      result = app_context.RunMainMessageLoop();
    } else {
      result = EXIT_FAILURE;
    }

    app->InvokeOnDestroy();
  }

  // Logging may still be needed in the destructors.
  xe::ShutdownWin32App();

  CoUninitialize();

  return result;
}
