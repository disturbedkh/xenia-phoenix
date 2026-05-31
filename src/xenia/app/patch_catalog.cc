/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/app/patch_catalog.h"

#include <fstream>

#include "third_party/fmt/include/fmt/format.h"
#include "third_party/tomlplusplus/toml.hpp"

namespace xe {
namespace app {

PatchCatalog::PatchCatalog(std::filesystem::path patches_root)
    : patches_root_(std::move(patches_root)), patch_db_(patches_root_) {
  Reload();
}

void PatchCatalog::Reload() { patch_db_.LoadPatches(); }

std::vector<PatchDisplayEntry> PatchCatalog::PatchesForTitle(
    uint32_t title_id) {
  std::vector<PatchDisplayEntry> out;
  const auto files = patch_db_.GetTitlePatches(title_id, std::nullopt);
  for (const auto& file : files) {
    for (const auto& patch : file.patch_info) {
      PatchDisplayEntry entry;
      entry.patch_id = patch.id;
      entry.name = patch.patch_name;
      entry.description = patch.patch_desc;
      entry.author = patch.patch_author;
      entry.enabled = patch.is_enabled;
      out.push_back(std::move(entry));
    }
  }
  return out;
}

bool PatchCatalog::SetPatchEnabled(uint32_t title_id, uint32_t patch_id,
                                   bool enabled, std::string& error) {
  std::error_code ec;
  if (!std::filesystem::is_directory(patches_root_, ec)) {
    error = "Patches directory not found";
    return false;
  }
  const std::string prefix = fmt::format("{:08X}", title_id);
  for (const auto& dir_entry :
       std::filesystem::directory_iterator(patches_root_ / "patches", ec)) {
    if (!dir_entry.is_regular_file()) {
      continue;
    }
    const auto filename = dir_entry.path().filename().string();
    if (filename.rfind(prefix, 0) != 0) {
      continue;
    }
    try {
      auto table = toml::parse_file(dir_entry.path().string());
      auto* patch_list = table["patch"].as_array();
      if (!patch_list) {
        continue;
      }
      bool changed = false;
      for (auto& patch_node : *patch_list) {
        auto* patch_table = patch_node.as_table();
        if (!patch_table) {
          continue;
        }
        const auto id = (*patch_table)["id"].value_or(0u);
        if (id != patch_id) {
          continue;
        }
        (*patch_table).insert_or_assign("is_enabled", enabled);
        changed = true;
        break;
      }
      if (!changed) {
        continue;
      }
      std::ofstream out(dir_entry.path());
      out << table;
      Reload();
      return true;
    } catch (const std::exception& ex) {
      error = ex.what();
      return false;
    }
  }
  error = "Patch file not found for title";
  (void)title_id;
  return false;
}

}  // namespace app
}  // namespace xe
