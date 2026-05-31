/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/app/general_settings_panel.h"

#include "third_party/imgui/imgui.h"
#include "xenia/app/cvar_settings_ui.h"
#include "xenia/app/emulator_window.h"
#include "xenia/emulator.h"

DECLARE_bool(fullscreen);
DECLARE_bool(discord);

namespace xe {
namespace app {

void DrawGeneralCuratedPanel(EmulatorWindow& emulator_window,
                             bool per_game_mode) {
  Emulator* emulator = emulator_window.emulator();

  ImGui::TextWrapped(
      "Application-wide options. Window size and other UI settings appear in "
      "the sections below.");

  bool fullscreen = cvars::fullscreen;
  if (ImGui::Checkbox("fullscreen", &fullscreen)) {
    CvarSettingsUi::ApplyCvarChange("General", "fullscreen", fullscreen,
                                    emulator, per_game_mode);
  }
  CvarSettingsUi::DrawCvarHelpMarker("fullscreen");

  bool discord = cvars::discord;
  if (ImGui::Checkbox("discord", &discord)) {
    CvarSettingsUi::ApplyCvarChange("General", "discord", discord, emulator,
                                    per_game_mode);
  }
  CvarSettingsUi::DrawCvarHelpMarker("discord");
  ImGui::SameLine();
  ImGui::TextColored(ImVec4(1.f, 0.75f, 0.35f, 1.f), "(restart)");

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::TextUnformatted("Related configuration windows");
  if (ImGui::Button("Open profile settings")) {
    emulator_window.ToggleProfilesConfigDialog();
  }
  ImGui::SameLine();
  if (ImGui::Button("Open Xbox Media Player (XMP) settings")) {
    emulator_window.ToggleXMPConfigDialog();
  }
}

}  // namespace app
}  // namespace xe
