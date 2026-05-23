/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2022 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include "xenia/base/cvar.h"
#include "xenia/base/logging.h"
#include "xenia/base/main_win.h"
#include "xenia/base/platform_win.h"
#include "xenia/base/string.h"

#include "version.h"

// For RequestWin32MMCSS.
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

// Start-Process / some shells split paths at spaces (e.g. D:\Xbox 360\game.iso).
// Merge the leading positional run and --target values before cxxopts parsing.
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

  // Print version info.
  XELOGI(
      "Build: "
#ifdef XE_BUILD_IS_PR
      "PR#" XE_BUILD_PR_NUMBER " - "
#endif
      XE_BUILD_BRANCH "@" XE_BUILD_COMMIT_SHORT " on " XE_BUILD_DATE);

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
