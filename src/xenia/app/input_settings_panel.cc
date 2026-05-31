/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/app/input_settings_panel.h"

#include "third_party/imgui/imgui.h"
#include "xenia/app/cvar_settings_ui.h"
#include "xenia/app/emulator_window.h"
#include "xenia/emulator.h"

DECLARE_string(hid);
DECLARE_bool(controller_hotkeys);

namespace xe {
namespace app {

void DrawInputCuratedPanel(EmulatorWindow& emulator_window,
                           bool per_game_mode) {
  Emulator* emulator = emulator_window.emulator();

  ImGui::TextWrapped(
      "Controller drivers and input behavior. Use the list below for "
      "advanced HID options.");

  const char* drivers[] = {"any", "nop", "sdl", "xinput", "winkey"};
  int index = 0;
  for (int i = 0; i < 5; ++i) {
    if (cvars::hid == drivers[i]) {
      index = i;
      break;
    }
  }
  if (ImGui::Combo("hid", &index, drivers, 5)) {
    CvarSettingsUi::ApplyCvarChange("HID", "hid", std::string(drivers[index]),
                                    emulator, per_game_mode);
  }
  CvarSettingsUi::DrawCvarHelpMarker("hid");

  bool hotkeys = cvars::controller_hotkeys;
  if (ImGui::Checkbox("controller_hotkeys", &hotkeys)) {
    CvarSettingsUi::ApplyCvarChange("HID", "controller_hotkeys", hotkeys,
                                    emulator, per_game_mode);
  }
  CvarSettingsUi::DrawCvarHelpMarker("controller_hotkeys");

  if (ImGui::Button("Show controller hotkey reference")) {
    emulator_window.DisplayHotKeysConfig();
  }
}

}  // namespace app
}  // namespace xe
