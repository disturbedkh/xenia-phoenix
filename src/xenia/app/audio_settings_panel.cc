/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/app/audio_settings_panel.h"

#include "third_party/imgui/imgui.h"
#include "xenia/app/cvar_settings_ui.h"

DECLARE_string(apu);

namespace xe {
namespace app {

void DrawAudioCuratedPanel(Emulator* emulator, bool per_game_mode) {
  ImGui::TextWrapped(
      "Audio processing backend and related options. Restart may be required "
      "when changing the APU backend.");

  const char* backends[] = {"any", "nop", "sdl", "xaudio2", "alsa"};
  int index = 0;
  for (int i = 0; i < 5; ++i) {
    if (cvars::apu == backends[i]) {
      index = i;
      break;
    }
  }
  if (ImGui::Combo("apu", &index, backends, 5)) {
    CvarSettingsUi::ApplyCvarChange("APU", "apu", std::string(backends[index]),
                                    emulator, per_game_mode);
  }
  CvarSettingsUi::DrawCvarHelpMarker("apu");
  ImGui::SameLine();
  ImGui::TextColored(ImVec4(1.f, 0.75f, 0.35f, 1.f), "(restart)");
}

}  // namespace app
}  // namespace xe
