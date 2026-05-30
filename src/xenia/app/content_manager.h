/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_APP_CONTENT_MANAGER_H_
#define XENIA_APP_CONTENT_MANAGER_H_

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace xe {

class Emulator;

namespace app {

struct ContentPackageInfo {
  std::filesystem::path source_path;
  uint32_t title_id = 0;
  std::string title_name;
  std::string content_type;  // DLC, TU, etc.
  bool installed = false;
};

// Scans STFS packages and installs DLC/TU to content paths.
class ContentManager {
 public:
  ContentManager(std::filesystem::path storage_root,
                 std::filesystem::path content_root);

  void SetContentScanFolder(const std::filesystem::path& folder);
  const std::filesystem::path& content_scan_folder() const {
    return content_scan_folder_;
  }

  void ScanPackages();
  const std::vector<ContentPackageInfo>& packages() const { return packages_; }

  std::vector<ContentPackageInfo> PackagesForTitle(uint32_t title_id) const;
  bool InstallPackage(Emulator& emulator, const ContentPackageInfo& package,
                      std::string& error);

 private:
  std::filesystem::path storage_root_;
  std::filesystem::path content_root_;
  std::filesystem::path content_scan_folder_;
  std::vector<ContentPackageInfo> packages_;
};

}  // namespace app
}  // namespace xe

#endif  // XENIA_APP_CONTENT_MANAGER_H_
