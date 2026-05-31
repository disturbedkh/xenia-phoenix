/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/app/patch_manager_panel.h"

#include "third_party/imgui/imgui.h"

namespace xe {
namespace app {

PatchManagerPanel::PatchManagerPanel(PatchCatalog& catalog)
    : catalog_(catalog) {}

void PatchManagerPanel::SetTitle(uint32_t title_id) { title_id_ = title_id; }

void PatchManagerPanel::Draw() {
  if (!title_id_) {
    ImGui::TextWrapped("Select a game to manage patches.");
    return;
  }
  auto patches = catalog_.PatchesForTitle(title_id_);
  ImGui::Text("Patches for %08X", title_id_);
  if (patches.empty()) {
    ImGui::TextDisabled("No patches found for this title.");
    return;
  }
  for (auto& patch : patches) {
    bool enabled = patch.enabled;
    if (ImGui::Checkbox(patch.name.c_str(), &enabled)) {
      std::string error;
      catalog_.SetPatchEnabled(title_id_, patch.patch_id, enabled, error);
    }
    if (!patch.description.empty()) {
      ImGui::TextDisabled("%s", patch.description.c_str());
    }
  }
}

}  // namespace app
}  // namespace xe
