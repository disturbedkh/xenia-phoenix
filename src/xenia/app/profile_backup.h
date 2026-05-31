/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_APP_PROFILE_BACKUP_H_
#define XENIA_APP_PROFILE_BACKUP_H_

#include <cstdint>
#include <filesystem>
#include <string>

namespace xe {
namespace app {

// Export/import profile save data (XUID-scoped content folders).
class ProfileBackup {
 public:
  ProfileBackup(std::filesystem::path content_root,
                std::filesystem::path backup_root);

  bool ExportProfile(uint64_t xuid, std::string& error) const;
  bool ImportProfile(const std::filesystem::path& archive_path,
                     std::string& error) const;

 private:
  std::filesystem::path content_root_;
  std::filesystem::path backup_root_;
};

}  // namespace app
}  // namespace xe

#endif  // XENIA_APP_PROFILE_BACKUP_H_
