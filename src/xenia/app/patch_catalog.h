/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_APP_PATCH_CATALOG_H_
#define XENIA_APP_PATCH_CATALOG_H_

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "xenia/patcher/patch_db.h"

namespace xe {
namespace app {

struct PatchDisplayEntry {
  uint32_t patch_id = 0;
  std::string name;
  std::string description;
  std::string author;
  bool enabled = false;
  char category = '?';  // A/B/C/D from Phoenix debt taxonomy when known
  std::filesystem::path source_file;
};

// Loads patches from game-patches directory and supports toggling is_enabled.
class PatchCatalog {
 public:
  explicit PatchCatalog(std::filesystem::path patches_root);

  void Reload();
  std::vector<PatchDisplayEntry> PatchesForTitle(uint32_t title_id);
  bool SetPatchEnabled(uint32_t title_id, uint32_t patch_id, bool enabled,
                       std::string& error);

  const std::filesystem::path& patches_root() const { return patches_root_; }

 private:
  std::filesystem::path patches_root_;
  patcher::PatchDB patch_db_;
};

}  // namespace app
}  // namespace xe

#endif  // XENIA_APP_PATCH_CATALOG_H_
