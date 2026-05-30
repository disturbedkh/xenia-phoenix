/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/app/nxe/nxe_assets.h"

#include "xenia/base/filesystem.h"
#include "xenia/base/logging.h"
#include "xenia/base/platform.h"

namespace xe {
namespace app {
namespace nxe {

NxeAssets& NxeAssets::Instance() {
  static NxeAssets instance;
  return instance;
}

std::filesystem::path NxeAssets::ResolveBundledRoot() const {
  const std::filesystem::path candidates[] = {
      std::filesystem::path("src/xenia/app/assets/dashboard"),
      std::filesystem::path("assets/dashboard"),
      std::filesystem::current_path() / "assets" / "dashboard",
  };
  for (const auto& candidate : candidates) {
    std::error_code ec;
    if (std::filesystem::exists(candidate / "fonts" / "X360.ttf", ec)) {
      return std::filesystem::absolute(candidate);
    }
  }
  return candidates[0];
}

void NxeAssets::Initialize(const std::filesystem::path& storage_root) {
  std::error_code ec;
  const auto bundled = storage_root / "assets" / "dashboard";
  if (std::filesystem::exists(bundled / "fonts" / "X360.ttf", ec)) {
    root_ = bundled;
  } else {
    root_ = ResolveBundledRoot();
  }
  initialized_ = true;
  XELOGI("NXE assets root: {}", xe::path_to_utf8(root_));
}

std::filesystem::path NxeAssets::FontX360() const {
  return root_ / "fonts" / "X360.ttf";
}

std::filesystem::path NxeAssets::Audio(const char* name) const {
  return root_ / "audio" / name;
}

std::filesystem::path NxeAssets::Icon(const char* name) const {
  return root_ / "icons" / name;
}

std::filesystem::path NxeAssets::Image(const char* name) const {
  return root_ / "images" / name;
}

std::filesystem::path NxeAssets::Wallpaper(const char* name) const {
  return root_ / "images" / "wallpapers" / name;
}

std::filesystem::path NxeAssets::Stage(const char* name) const {
  return root_ / "images" / "stages" / name;
}

std::filesystem::path NxeAssets::Flourish() const {
  return root_ / "images" / "nxe-flourish" / "nxe-flourish.png";
}

std::filesystem::path NxeAssets::DataFile(const char* name) const {
  return root_ / "data" / name;
}

bool NxeAssets::Exists(const std::filesystem::path& path) const {
  std::error_code ec;
  return std::filesystem::exists(path, ec);
}

}  // namespace nxe
}  // namespace app
}  // namespace xe
