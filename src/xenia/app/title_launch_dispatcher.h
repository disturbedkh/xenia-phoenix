/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_APP_TITLE_LAUNCH_DISPATCHER_H_
#define XENIA_APP_TITLE_LAUNCH_DISPATCHER_H_

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "xenia/xbox.h"

namespace xe {
class Emulator;
namespace app {

class EmulatorWindow;

enum class LaunchRequestSource {
  kLibrary,
  kFileOpen,
  kCommandLine,
  kRecent,
  kKernelXam,
  kDragDrop,
  kAndroidIntent,
};

enum class LaunchRequestKind {
  kNewTitle,
  kRelaunch,
  kDiscSwap,
};

struct LaunchRequest {
  std::filesystem::path path;
  std::string launch_module;
  uint32_t launch_flags = 0;
  std::vector<uint8_t> launch_data;
  LaunchRequestSource source = LaunchRequestSource::kLibrary;
  LaunchRequestKind kind = LaunchRequestKind::kNewTitle;
};

// Routes title launches to in-process relaunch, process respawn, or Android.
class TitleLaunchDispatcher {
 public:
  TitleLaunchDispatcher(Emulator& emulator, EmulatorWindow& window);

  X_STATUS Dispatch(LaunchRequest request);

  void LaunchTitleInNewProcess(const std::filesystem::path& path_to_file);

 private:
  bool BackendChanged() const;

  Emulator& emulator_;
  EmulatorWindow& window_;
};

}  // namespace app
}  // namespace xe

#endif  // XENIA_APP_TITLE_LAUNCH_DISPATCHER_H_
