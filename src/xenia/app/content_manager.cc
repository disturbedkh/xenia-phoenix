/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/app/content_manager.h"

#include "third_party/fmt/include/fmt/format.h"
#include "xenia/base/string.h"
#include "xenia/emulator.h"
#include "xenia/vfs/devices/xcontent_container_device.h"
#include "xenia/xbox.h"

namespace xe {
namespace app {

ContentManager::ContentManager(std::filesystem::path storage_root,
                               std::filesystem::path content_root)
    : storage_root_(std::move(storage_root)),
      content_root_(std::move(content_root)),
      content_scan_folder_(storage_root_ / "content_packages") {}

void ContentManager::SetContentScanFolder(const std::filesystem::path& folder) {
  content_scan_folder_ = folder;
}

void ContentManager::ScanPackages() {
  packages_.clear();
  std::error_code ec;
  if (!std::filesystem::exists(content_scan_folder_, ec)) {
    return;
  }
  for (const auto& entry : std::filesystem::recursive_directory_iterator(
           content_scan_folder_, ec)) {
    if (!entry.is_regular_file()) {
      continue;
    }
    const auto ext = entry.path().extension().string();
    if (ext != ".live" && ext != ".pirs" && ext != ".con") {
      continue;
    }
    const auto header =
        vfs::XContentContainerDevice::ReadContainerHeader(entry.path());
    if (!header || !header->content_header.is_magic_valid()) {
      continue;
    }
    ContentPackageInfo info;
    info.source_path = entry.path();
    info.title_id = header->content_metadata.execution_info.title_id.get();
    info.title_name =
        xe::to_utf8(header->content_metadata.display_name(XLanguage::kEnglish));
    const auto ctype =
        static_cast<XContentType>(header->content_metadata.content_type.get());
    auto type_it = XContentTypeMap.find(ctype);
    info.content_type =
        type_it != XContentTypeMap.end()
            ? type_it->second
            : fmt::format("{:08X}", static_cast<uint32_t>(ctype));
    packages_.push_back(std::move(info));
  }
}

std::vector<ContentPackageInfo> ContentManager::PackagesForTitle(
    uint32_t title_id) const {
  std::vector<ContentPackageInfo> out;
  for (const auto& p : packages_) {
    if (p.title_id == title_id) {
      out.push_back(p);
    }
  }
  return out;
}

bool ContentManager::InstallPackage(Emulator& emulator,
                                    const ContentPackageInfo& package,
                                    std::string& error) {
  Emulator::ContentInstallEntry entry(package.source_path);
  const X_STATUS status =
      emulator.InstallContentPackage(package.source_path, entry);
  if (status != X_STATUS_SUCCESS) {
    error = "InstallContentPackage failed";
    return false;
  }
  return true;
}

}  // namespace app
}  // namespace xe
