/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/app/title_launch_dispatcher.h"

#include "xenia/app/emulator_window.h"
#include "xenia/base/cvar.h"
#include "xenia/base/logging.h"
#include "xenia/base/platform.h"
#include "xenia/base/system.h"
#include "xenia/emulator.h"

#if XE_PLATFORM_WIN32
#include <windows.h>
#endif

#if !XE_PLATFORM_WIN32 && !XE_PLATFORM_ANDROID
#include <unistd.h>
#endif

DECLARE_string(apu);
DECLARE_string(gpu);
DECLARE_string(config);

namespace xe {
namespace app {

TitleLaunchDispatcher::TitleLaunchDispatcher(Emulator& emulator,
                                             EmulatorWindow& window)
    : emulator_(emulator), window_(window) {}

bool TitleLaunchDispatcher::BackendChanged() const {
  const auto& last_gpu = emulator_.active_gpu_backend();
  const auto& last_apu = emulator_.active_apu_backend();
  return (!last_gpu.empty() && last_gpu != cvars::gpu) ||
         (!last_apu.empty() && last_apu != cvars::apu);
}

X_STATUS TitleLaunchDispatcher::Dispatch(LaunchRequest request) {
  if (request.kind == LaunchRequestKind::kDiscSwap) {
    XELOGW("TitleLaunchDispatcher::Dispatch: disc swap {}",
           request.path.string());
    return emulator_.LaunchPath(request.path);
  }

  auto abs_path = std::filesystem::absolute(request.path);
  XELOGW("TitleLaunchDispatcher::Dispatch: path={} title_open={} kind={}",
         abs_path.string(), emulator_.is_title_open(),
         static_cast<int>(request.kind));
  // Per-game TOML overrides are applied in Emulator::CompleteLaunch; global
  // gpu/apu cvars are used for backend selection here.
  emulator_.SetPendingBackends(cvars::gpu, cvars::apu);

  if (emulator_.is_title_open()) {
    if (BackendChanged()) {
      XELOGW(
          "TitleLaunchDispatcher::Dispatch: backend changed, relaunching in "
          "new process");
      LaunchTitleInNewProcess(abs_path);
      return X_STATUS_SUCCESS;
    }

#if XE_PLATFORM_WIN32
    XELOGW(
        "TitleLaunchDispatcher::Dispatch: posting in-process RelaunchTitle to "
        "lifecycle worker");
    auto host_path = xe::path_to_utf8(abs_path);
    emulator_.PostToLifecycleWorker(
        [em = &emulator_, host_path = std::move(host_path),
         launch_module = request.launch_module,
         launch_flags = request.launch_flags,
         launch_data = std::move(request.launch_data)]() {
          em->RelaunchTitle(host_path, launch_module, launch_flags,
                            std::move(launch_data));
        });
    return X_STATUS_SUCCESS;
#elif XE_PLATFORM_ANDROID
    XELOGW(
        "TitleLaunchDispatcher::Dispatch: Android relaunch via new Activity");
    LaunchTitleInNewProcess(abs_path);
    return X_STATUS_SUCCESS;
#else
    XELOGW(
        "TitleLaunchDispatcher::Dispatch: POSIX relaunch via guest cb / fork");
    auto cb = emulator_.on_launch_new_title();
    if (cb) {
      cb(xe::path_to_utf8(abs_path), request.launch_module,
         request.launch_flags, "");
    }
    return X_STATUS_SUCCESS;
#endif
  }

  // First launch: subsystems already initialized at startup (SetupSubsystems).
  XELOGW(
      "TitleLaunchDispatcher::Dispatch: first launch, calling "
      "Emulator::LaunchPath directly");
  return emulator_.LaunchPath(abs_path);
}

void TitleLaunchDispatcher::LaunchTitleInNewProcess(
    const std::filesystem::path& path_to_file) {
  std::filesystem::path executable_path = xe::filesystem::GetExecutablePath();

  if (!path_to_file.empty() && !std::filesystem::exists(path_to_file)) {
    XELOGE("Cannot launch title - file not found: {}", path_to_file.string());
    return;
  }

#if XE_PLATFORM_WIN32
  auto exe_path_u16 = xe::path_to_utf16(executable_path);
  std::u16string cmd_line = u"\"" + exe_path_u16 + u"\"";
  if (!cvars::config.empty()) {
    cmd_line += u" --config=\"" + xe::to_utf16(cvars::config) + u"\"";
  }
  cmd_line += u" --return_to_ui=true";
  if (!path_to_file.empty()) {
    cmd_line += u" \"" + xe::path_to_utf16(path_to_file) + u"\"";
  }

  STARTUPINFOW si = {};
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi = {};
  if (!CreateProcessW(nullptr,
                      const_cast<wchar_t*>(
                          reinterpret_cast<const wchar_t*>(cmd_line.c_str())),
                      nullptr, nullptr, FALSE, CREATE_NEW_CONSOLE, nullptr,
                      nullptr, &si, &pi)) {
    XELOGE("Failed to launch new process: {}", GetLastError());
    return;
  }
  AllowSetForegroundWindow(pi.dwProcessId);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
#elif !XE_PLATFORM_ANDROID
  std::vector<std::string> arg_storage;
  std::vector<char*> argv;
  arg_storage.push_back(executable_path.string());
  if (!cvars::config.empty()) {
    arg_storage.push_back("--config=" + cvars::config);
  }
  arg_storage.push_back("--return_to_ui=true");
  if (!path_to_file.empty()) {
    arg_storage.push_back(path_to_file.string());
  }
  for (auto& s : arg_storage) {
    argv.push_back(s.data());
  }
  argv.push_back(nullptr);

  pid_t pid = fork();
  if (pid == 0) {
    execv(executable_path.c_str(), argv.data());
    XELOGE("Failed to execute: {}", executable_path.string());
    std::exit(1);
  } else if (pid < 0) {
    XELOGE("Failed to fork process");
    return;
  }
#else
  (void)path_to_file;
  XELOGW("LaunchTitleInNewProcess: Android uses Activity intents");
  return;
#endif

  XELOGI("Launched title in new process: {}", path_to_file.string());
  xe::FlushLog();
  window_.app_context().QuitFromUIThread();
}

}  // namespace app
}  // namespace xe
