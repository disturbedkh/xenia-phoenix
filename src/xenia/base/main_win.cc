/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2022 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include <csignal>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <exception>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "xenia/base/cvar.h"
#include "xenia/base/filesystem.h"
#include "xenia/base/logging.h"
#include "xenia/base/main_win.h"
#include "xenia/base/platform_win.h"
#include "xenia/base/string.h"

#include "version.h"

// For RequestWin32MMCSS.
#include <crtdbg.h>
#include <dwmapi.h>
// For RequestWin32HighResolutionTimer.
#include <winternl.h>

DEFINE_bool(win32_high_resolution_timer, true,
            "Requests high-resolution timer from the NT kernel", "Win32");
DEFINE_bool(
    win32_mmcss, true,
    "Opt in the Multimedia Class Scheduler Service (MMCSS) scheduling for "
    "prioritized access to CPU resources",
    "Win32");

DEFINE_path(
    storage_root, "",
    "Root path for persistent internal data storage (config, etc.), or empty "
    "to use the path preferred for the OS, such as the documents folder, or "
    "the emulator executable directory if portable.txt is present in it.",
    "Storage");
#ifndef XE_PLATFORM_WIN32
DEFINE_transient_bool(portable, false,
                      "Specifies if Xenia should run in portable mode.",
                      "General");
#else
DEFINE_transient_bool(portable, true,
                      "Specifies if Xenia should run in portable mode.",
                      "General");
#endif

namespace xe {

static void RequestWin32HighResolutionTimer() {
  HMODULE ntdll_module = GetModuleHandleW(L"ntdll.dll");
  if (!ntdll_module) {
    return;
  }

  // clang-format off
  NTSTATUS (NTAPI* nt_query_timer_resolution)(OUT PULONG MinimumResolution,
                                              OUT PULONG MaximumResolution,
                                              OUT PULONG CurrentResolution);
  NTSTATUS (NTAPI* nt_set_timer_resolution)(IN ULONG DesiredResolution,
                                            IN BOOLEAN SetResolution,
                                            OUT PULONG CurrentResolution);
  // clang-format on
  nt_query_timer_resolution =
      reinterpret_cast<decltype(nt_query_timer_resolution)>(
          GetProcAddress(ntdll_module, "NtQueryTimerResolution"));
  nt_set_timer_resolution = reinterpret_cast<decltype(nt_set_timer_resolution)>(
      GetProcAddress(ntdll_module, "NtSetTimerResolution"));
  if (!nt_query_timer_resolution || !nt_set_timer_resolution) {
    return;
  }

  ULONG minimum_resolution, maximum_resolution, current_resolution;
  nt_query_timer_resolution(&minimum_resolution, &maximum_resolution,
                            &current_resolution);
  nt_set_timer_resolution(maximum_resolution, true, &current_resolution);
}

static void RequestWin32MMCSS() {
  HMODULE dwmapi_module = LoadLibraryW(L"dwmapi.dll");
  if (!dwmapi_module) {
    return;
  }
  // clang-format off
  HRESULT (STDAPICALLTYPE* dwm_enable_mmcss)(BOOL fEnableMMCSS);
  // clang-format on
  dwm_enable_mmcss = reinterpret_cast<decltype(dwm_enable_mmcss)>(
      GetProcAddress(dwmapi_module, "DwmEnableMMCSS"));
  if (dwm_enable_mmcss) {
    dwm_enable_mmcss(true);
  }
  FreeLibrary(dwmapi_module);
}

namespace {

bool IsLaunchOptionToken(std::string_view arg) {
  // Treat lone "-" as positional; "--foo" and "-f" are options.
  return arg.size() >= 2 && arg[0] == '-';
}

// Start-Process / some shells split paths at spaces (e.g. D:\Xbox
// 360\game.iso). Merge the leading positional run and --target values before
// cxxopts parsing.
void CoalesceSplitPathArgv(int& argc, char**& argv) {
  if (argc <= 1) {
    return;
  }

  static std::vector<std::string> storage;
  static std::vector<char*> ptrs;
  storage.clear();
  ptrs.clear();
  storage.emplace_back(argv[0]);

  auto merge_range = [&](int begin, int end) {
    std::string merged = argv[begin];
    for (int i = begin + 1; i <= end; ++i) {
      merged.push_back(' ');
      merged += argv[i];
    }
    storage.push_back(std::move(merged));
  };

  int i = 1;
  bool merged_leading_positional = false;
  while (i < argc) {
    std::string_view arg(argv[i]);
    if (!merged_leading_positional && !IsLaunchOptionToken(arg)) {
      int begin = i;
      while (i < argc && !IsLaunchOptionToken(argv[i])) {
        ++i;
      }
      merge_range(begin, i - 1);
      merged_leading_positional = true;
      continue;
    }

    if (arg == "--target" && i + 1 < argc &&
        !IsLaunchOptionToken(argv[i + 1])) {
      int begin = i + 1;
      int j = begin + 1;
      while (j < argc && !IsLaunchOptionToken(argv[j])) {
        ++j;
      }
      storage.emplace_back("--target");
      merge_range(begin, j - 1);
      i = j;
      continue;
    }

    storage.emplace_back(argv[i]);
    ++i;
  }

  if (storage.size() == static_cast<size_t>(argc)) {
    return;
  }

  ptrs.reserve(storage.size());
  for (auto& s : storage) {
    ptrs.push_back(s.data());
  }
  argc = static_cast<int>(ptrs.size());
  argv = ptrs.data();
}

static std::string WideToUtf8(const wchar_t* wide) {
  if (!wide) {
    return "<null>";
  }
  return xe::to_utf8(std::u16string_view(
      reinterpret_cast<const char16_t*>(wide), std::wcslen(wide)));
}

static void LogWin32StartupContext() {
  XELOGI("Command line: {}", WideToUtf8(GetCommandLineW()));

  wchar_t working_dir_w[MAX_PATH + 1] = {};
  if (GetCurrentDirectoryW(xe::countof(working_dir_w), working_dir_w)) {
    XELOGI("Working directory: {}", WideToUtf8(working_dir_w));
  }

  XELOGI("Process ID: {}", GetCurrentProcessId());
  XELOGI("Debugger attached: {}", IsDebuggerPresent() ? "yes" : "no");

  const auto storage_root =
      xe::filesystem::ResolveStorageRoot(cvars::storage_root, cvars::portable);
  XELOGI("Storage root: {}", storage_root);
  XELOGI("Log file: {}", xe::GetActiveLogFilePath());

  using RtlGetVersionFn = LONG(WINAPI*)(PRTL_OSVERSIONINFOW);
  HMODULE ntdll_module = GetModuleHandleW(L"ntdll.dll");
  if (ntdll_module) {
    auto rtl_get_version = reinterpret_cast<RtlGetVersionFn>(
        GetProcAddress(ntdll_module, "RtlGetVersion"));
    if (rtl_get_version) {
      RTL_OSVERSIONINFOW version_info = {};
      version_info.dwOSVersionInfoSize = sizeof(version_info);
      if (rtl_get_version(&version_info) >= 0) {
        XELOGI("Host OS: {}.{}.{}", version_info.dwMajorVersion,
               version_info.dwMinorVersion, version_info.dwBuildNumber);
      }
    }
  }
}

#ifdef _DEBUG
static int XeniaCrtReportHook(int report_type, char* message,
                              int* return_value) {
  (void)return_value;
  XELOGE("[CRT report type={}] {}", report_type, message ? message : "<null>");
  xe::logging::WriteCrashSidecar("crt", message ? message : "<null>");
  xe::FlushLog();
  return FALSE;
}
#endif

static void XeniaInvalidParameterHandler(const wchar_t* expression,
                                         const wchar_t* function,
                                         const wchar_t* file, unsigned int line,
                                         uintptr_t reserved) {
  (void)reserved;
  XELOGE("[CRT invalid parameter] expr={} func={} file={} line={}",
         expression ? WideToUtf8(expression) : "<null>",
         function ? WideToUtf8(function) : "<null>",
         file ? WideToUtf8(file) : "<null>", line);
  xe::FlushLog();
}

[[noreturn]] static void XeniaTerminateHandler() {
  XELOGE("std::terminate invoked");
  xe::FlushLog();
  std::abort();
}

[[noreturn]] static void XeniaPurecallHandler() {
  XELOGE("pure-virtual call");
  xe::FlushLog();
  std::abort();
}

static void XeniaSigabrtHandler(int signal) {
  (void)signal;
  XELOGE("SIGABRT");
  xe::FlushLog();
}

static void InstallSoftwareLevelCrashHooks() {
#ifdef _DEBUG
  _CrtSetReportHook2(_CRT_RPTHOOK_INSTALL, &XeniaCrtReportHook);
  _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_WNDW);
  _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_WNDW);
  _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_WNDW);
#endif
  _set_invalid_parameter_handler(&XeniaInvalidParameterHandler);
  std::set_terminate(&XeniaTerminateHandler);
  _set_purecall_handler(&XeniaPurecallHandler);
  signal(SIGABRT, &XeniaSigabrtHandler);
  _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
}

}  // namespace

bool ParseWin32LaunchArguments(
    bool transparent_options, const std::string_view positional_usage,
    const std::vector<std::string>& positional_options,
    std::vector<std::string>* args_out) {
  auto command_line = GetCommandLineW();

  int wargc;
  wchar_t** wargv = CommandLineToArgvW(command_line, &wargc);
  if (!wargv) {
    return false;
  }

  // Convert all args to narrow, as cxxopts doesn't support wchar.
  int argc = wargc;
  char** argv = reinterpret_cast<char**>(alloca(sizeof(char*) * argc));
  for (int n = 0; n < argc; n++) {
    size_t len = std::wcstombs(nullptr, wargv[n], 0);
    argv[n] = reinterpret_cast<char*>(alloca(sizeof(char) * (len + 1)));
    std::wcstombs(argv[n], wargv[n], len + 1);
  }

  LocalFree(wargv);

  CoalesceSplitPathArgv(argc, argv);

  if (!transparent_options) {
    cvar::ParseLaunchArguments(argc, argv, positional_usage,
                               positional_options);
  }

  if (args_out) {
    args_out->clear();
    for (int n = 0; n < argc; n++) {
      args_out->push_back(std::string(argv[n]));
    }
  }

  return true;
}

int InitializeWin32App(const std::string_view app_name) {
  // Initialize logging. Needs parsed FLAGS.
  xe::InitializeLogging(app_name);

  const auto storage_root =
      xe::filesystem::ResolveStorageRoot(cvars::storage_root, cvars::portable);
  xe::AttachFileLogSink(storage_root / "log", app_name);

  InstallSoftwareLevelCrashHooks();

  // Print version info.
  XELOGI(
      "Build: "
#ifdef XE_BUILD_IS_PR
      "PR#" XE_BUILD_PR_NUMBER " - "
#endif
      XE_BUILD_BRANCH "@" XE_BUILD_COMMIT_SHORT " on " XE_BUILD_DATE);
#if XE_ARCH_AMD64 == 1
  XELOGI("Target architecture: x64");
#elif XE_ARCH_ARM64 == 1
  XELOGI("Target architecture: arm64");
#endif

  LogWin32StartupContext();

  // Request high-performance timing and scheduling.
  if (cvars::win32_high_resolution_timer) {
    RequestWin32HighResolutionTimer();
  }
  if (cvars::win32_mmcss) {
    RequestWin32MMCSS();
  }

  return 0;
}

void ShutdownWin32App() { xe::ShutdownLogging(); }

}  // namespace xe
