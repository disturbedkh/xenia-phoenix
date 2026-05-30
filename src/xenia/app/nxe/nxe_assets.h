/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_APP_NXE_NXE_ASSETS_H_
#define XENIA_APP_NXE_NXE_ASSETS_H_

#include <filesystem>
#include <string>

namespace xe {
namespace app {
namespace nxe {

// Resolves bundled NXE dashboard assets relative to exe / storage_root.
class NxeAssets {
 public:
  static NxeAssets& Instance();

  void Initialize(const std::filesystem::path& storage_root);

  std::filesystem::path Root() const { return root_; }
  std::filesystem::path FontX360() const;
  std::filesystem::path Audio(const char* name) const;
  std::filesystem::path Icon(const char* name) const;
  std::filesystem::path Image(const char* name) const;
  std::filesystem::path Wallpaper(const char* name = "2.png") const;
  std::filesystem::path Stage(const char* name = "0003.png") const;
  std::filesystem::path Flourish() const;
  std::filesystem::path DataFile(const char* name) const;

  bool Exists(const std::filesystem::path& path) const;

 private:
  NxeAssets() = default;
  std::filesystem::path ResolveBundledRoot() const;

  std::filesystem::path root_;
  bool initialized_ = false;
};

}  // namespace nxe
}  // namespace app
}  // namespace xe

#endif  // XENIA_APP_NXE_NXE_ASSETS_H_
