/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/app/profile_backup.h"

#include "third_party/fmt/include/fmt/format.h"

namespace xe {
namespace app {

ProfileBackup::ProfileBackup(std::filesystem::path content_root,
                             std::filesystem::path backup_root)
    : content_root_(std::move(content_root)),
      backup_root_(std::move(backup_root)) {}

bool ProfileBackup::ExportProfile(uint64_t xuid, std::string& error) const {
  const auto src = content_root_ / fmt::format("{:016X}", xuid);
  std::error_code ec;
  if (!std::filesystem::exists(src, ec)) {
    error = "Profile content folder not found";
    return false;
  }
  const auto dest = backup_root_ / fmt::format("{:016X}", xuid);
  std::filesystem::create_directories(dest.parent_path(), ec);
  std::filesystem::copy(src, dest,
                        std::filesystem::copy_options::recursive |
                            std::filesystem::copy_options::overwrite_existing,
                        ec);
  if (ec) {
    error = ec.message();
    return false;
  }
  return true;
}

bool ProfileBackup::ImportProfile(const std::filesystem::path& archive_path,
                                  std::string& error) const {
  std::error_code ec;
  if (!std::filesystem::exists(archive_path, ec)) {
    error = "Backup path not found";
    return false;
  }
  const auto folder_name = archive_path.filename().string();
  const auto dest = content_root_ / folder_name;
  std::filesystem::copy(archive_path, dest,
                        std::filesystem::copy_options::recursive |
                            std::filesystem::copy_options::overwrite_existing,
                        ec);
  if (ec) {
    error = ec.message();
    return false;
  }
  return true;
}

}  // namespace app
}  // namespace xe
