/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_APP_PREFERENCES_DIALOG_H_
#define XENIA_APP_PREFERENCES_DIALOG_H_

#include "xenia/app/cvar_settings_ui.h"
#include "xenia/app/graphics_settings_panel.h"
#include "xenia/app/settings_ui_registry.h"
#include "xenia/ui/imgui_dialog.h"

namespace xe {
namespace app {

class EmulatorWindow;

class PreferencesDialog final : public ui::ImGuiDialog {
 public:
  PreferencesDialog(ui::ImGuiDrawer* imgui_drawer,
                    EmulatorWindow& emulator_window,
                    PreferencesTab initial_tab = PreferencesTab::kGraphics);

  void SetInitialTab(PreferencesTab tab);
  void SetPerGameMode(bool enabled);

 protected:
  void OnDraw(ImGuiIO& io) override;

 private:
  void DrawHeader();
  void DrawActiveTab();

  EmulatorWindow& emulator_window_;
  PreferencesTab initial_tab_ = PreferencesTab::kGraphics;
  PreferencesTab active_tab_ = PreferencesTab::kGraphics;
  int pending_tab_select_frames_ = 2;
  bool per_game_mode_ = false;
  bool wants_open_ = true;
  CvarSettingsUiState cvar_ui_state_{};
  GraphicsSettingsPanelState graphics_panel_state_{};
  std::string window_title_;
};

}  // namespace app
}  // namespace xe

#endif  // XENIA_APP_PREFERENCES_DIALOG_H_
