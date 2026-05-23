/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_APP_GRAPHICS_SETTINGS_DIALOG_H_
#define XENIA_APP_GRAPHICS_SETTINGS_DIALOG_H_

#include "xenia/ui/imgui_dialog.h"

namespace xe {
namespace app {

class EmulatorWindow;

class GraphicsSettingsDialog final : public ui::ImGuiDialog {
 public:
  GraphicsSettingsDialog(ui::ImGuiDrawer* imgui_drawer,
                         EmulatorWindow& emulator_window);

 protected:
  void OnDraw(ImGuiIO& io) override;

 private:
  void DrawResolutionSection();
  void DrawBackendSection();
  void DrawQualitySection();
  void DrawPerGameStar(const char* section, const char* cvar_name,
                       const std::string& value);

  EmulatorWindow& emulator_window_;
  int resolution_scale_ = 1;
  bool independent_scale_xy_ = false;
  int scale_x_ = 1;
  int scale_y_ = 1;
  bool edit_per_game_ = false;
  bool wants_open_ = true;
  std::string window_title_;
};

}  // namespace app
}  // namespace xe

#endif  // XENIA_APP_GRAPHICS_SETTINGS_DIALOG_H_
