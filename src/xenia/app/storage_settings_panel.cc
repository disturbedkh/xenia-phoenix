/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/app/storage_settings_panel.h"

#include "third_party/imgui/imgui.h"
#include "xenia/app/cvar_settings_ui.h"
#include "xenia/base/utf8.h"
#include "xenia/config.h"

DECLARE_bool(mount_scratch);
DECLARE_bool(mount_cache);
DECLARE_path(storage_root);
DECLARE_path(content_root);
DECLARE_path(cache_root);

namespace xe {
namespace app {

void DrawStorageCuratedPanel(Emulator* emulator, bool per_game_mode) {
  ImGui::TextWrapped(
      "Paths for saves, cache, and content. Edit path overrides in the Storage "
      "section below or in xenia-canary.config.toml.");

  ImGui::TextDisabled("Storage root: %s",
                      xe::path_to_utf8(cvars::storage_root).c_str());
  ImGui::TextDisabled("Content root: %s",
                      xe::path_to_utf8(cvars::content_root).c_str());
  ImGui::TextDisabled("Host cache root: %s",
                      xe::path_to_utf8(cvars::cache_root).c_str());

  bool mount_scratch = cvars::mount_scratch;
  if (ImGui::Checkbox("mount_scratch", &mount_scratch)) {
    CvarSettingsUi::ApplyCvarChange("Storage", "mount_scratch", mount_scratch,
                                    emulator, per_game_mode);
  }
  CvarSettingsUi::DrawCvarHelpMarker("mount_scratch");

  bool mount_cache = cvars::mount_cache;
  if (ImGui::Checkbox("mount_cache", &mount_cache)) {
    CvarSettingsUi::ApplyCvarChange("Storage", "mount_cache", mount_cache,
                                    emulator, per_game_mode);
  }
  CvarSettingsUi::DrawCvarHelpMarker("mount_cache");
}

}  // namespace app
}  // namespace xe
