/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2020 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/base/filesystem.h"

#include <fstream>
#include <ios>
#include <system_error>

#include "xenia/base/platform.h"
#include "xenia/base/string_util.h"

namespace xe {
namespace filesystem {

std::vector<uint8_t> ReadAllBytes(const std::filesystem::path& path) {
  std::ifstream f(path, std::ios::binary | std::ios::ate);
  if (!f) {
    return {};
  }
  const std::streamoff size = f.tellg();
  if (size <= 0) {
    return {};
  }
  std::vector<uint8_t> data(static_cast<size_t>(size));
  f.seekg(0);
  if (!f.read(reinterpret_cast<char*>(data.data()), data.size())) {
    return {};
  }
  return data;
}

std::string ReadAllText(const std::filesystem::path& path) {
  std::ifstream f(path, std::ios::binary | std::ios::ate);
  if (!f) {
    return {};
  }
  const std::streamoff size = f.tellg();
  if (size <= 0) {
    return {};
  }
  std::string data(static_cast<size_t>(size), '\0');
  f.seekg(0);
  if (!f.read(data.data(), data.size())) {
    return {};
  }
  return data;
}

std::filesystem::path ResolveStorageRoot(
    const std::filesystem::path& override_path, bool portable) {
  std::filesystem::path storage_root = override_path;
  if (storage_root.empty()) {
    storage_root = GetExecutableFolder();
    if (!portable && !std::filesystem::exists(storage_root / "portable.txt")) {
      storage_root = GetUserFolder();
#if XE_PLATFORM_ANDROID
      std::filesystem::path android_files =
          GetAndroidApplicationFilesDirectory();
      if (!android_files.empty()) {
        storage_root = android_files;
      }
#else
      storage_root = storage_root / "Xenia";
#endif
    }
  }
  return std::filesystem::absolute(storage_root);
}

bool CreateParentFolder(const std::filesystem::path& path) {
  if (!path.has_parent_path()) {
    return true;
  }

  std::error_code ec = CreateFolder(path.parent_path());
  if (!ec || ec == std::errc::file_exists) {
    return true;
  }

  return false;
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

  return directories;
}

std::vector<FileInfo> FilterByName(const std::vector<FileInfo>& files,
                                   const std::regex pattern) {
  std::vector<FileInfo> filtered_entries = {};

  std::copy_if(
      files.cbegin(), files.cend(), std::back_inserter(filtered_entries),
      [pattern](const FileInfo& file) {
        return std::regex_match(file.name.filename().string(), pattern);
      });
  return filtered_entries;
}

std::vector<FileInfo> FindFileWithName(const std::filesystem::path& path,
                                       std::string_view name, bool recursive) {
  if (!std::filesystem::exists(path)) {
    return {};
  }

  if (!std::filesystem::is_directory(path)) {
    return {};
  }

  if (!recursive) {
    return FilterByName(ListFiles(path), std::regex(std::string(name)));
  }

  const std::string file_name = xe::utf8::lower_ascii(name);

  std::vector<FileInfo> filtered_entries = {};
  for (const auto& entry :
       std::filesystem::recursive_directory_iterator(path)) {
    if (entry.is_regular_file() && xe::utf8::lower_ascii(xe::path_to_utf8(
                                       entry.path().filename())) == file_name) {
      auto file_info = GetInfo(entry.path());
      if (file_info) {
        filtered_entries.push_back(std::move(file_info.value()));
      }
    }
  }
  return filtered_entries;
}

}  // namespace filesystem
}  // namespace xe
