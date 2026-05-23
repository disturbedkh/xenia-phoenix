/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2020 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/base/filesystem.h"
#include "xenia/base/agent_debug_log.h"

namespace xe {
namespace filesystem {

bool CreateParentFolder(const std::filesystem::path& path) {
  if (path.has_parent_path()) {
    auto parent_path = path.parent_path();
    const bool parent_exists = std::filesystem::exists(parent_path);
    // #region agent log
    xe::agent_debug::Log("filesystem.cc:CreateParentFolder", "entry", "H1",
                         "pre-fix",
                         R"({{"path":"{}","parent":"{}","parent_exists":{}}})",
                         xe::path_to_utf8(path), xe::path_to_utf8(parent_path),
                         parent_exists ? "true" : "false");
    // #endregion
    if (!parent_exists) {
      try {
        const bool created = std::filesystem::create_directories(parent_path);
        // #region agent log
        xe::agent_debug::Log(
            "filesystem.cc:CreateParentFolder", "created", "H1", "pre-fix",
            R"({{"parent":"{}","created":{}}})", xe::path_to_utf8(parent_path),
            created ? "true" : "false");
        // #endregion
        return created;
      } catch (const std::filesystem::filesystem_error& ex) {
        // #region agent log
        xe::agent_debug::Log(
            "filesystem.cc:CreateParentFolder", "filesystem_error", "H1",
            "pre-fix", R"({{"parent":"{}","code":{},"what":"{}"}})",
            xe::path_to_utf8(parent_path), ex.code().value(), ex.what());
        // #endregion
        throw;
      }
    }
  }
  return true;
}

std::error_code CreateFolder(const std::filesystem::path& path) {
  if (std::filesystem::exists(path)) {
    return {};
  }

  std::error_code ec;
  if (std::filesystem::create_directories(path, ec)) {
    return {};
  }

  return ec;
}

std::vector<FileInfo> ListDirectories(const std::filesystem::path& path) {
  std::vector<FileInfo> files = ListFiles(path);
  std::vector<FileInfo> directories = {};

  std::copy_if(files.cbegin(), files.cend(), std::back_inserter(directories),
               [](const FileInfo& file) {
                 return file.type == FileInfo::Type::kDirectory;
               });

  return std::move(directories);
}

std::vector<FileInfo> FilterByName(const std::vector<FileInfo>& files,
                                   const std::regex pattern) {
  std::vector<FileInfo> filtered_entries = {};

  std::copy_if(
      files.cbegin(), files.cend(), std::back_inserter(filtered_entries),
      [pattern](const FileInfo& file) {
        return std::regex_match(file.name.filename().string(), pattern);
      });
  return std::move(filtered_entries);
}

}  // namespace filesystem
}  // namespace xe
