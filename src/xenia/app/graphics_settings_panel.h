/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_APP_GRAPHICS_SETTINGS_PANEL_H_
#define XENIA_APP_GRAPHICS_SETTINGS_PANEL_H_

namespace xe {
namespace app {

class EmulatorWindow;

struct GraphicsSettingsPanelState {
  int resolution_scale = 1;
  bool independent_scale_xy = false;
  int scale_x = 1;
  int scale_y = 1;
};

void DrawGraphicsCuratedPanel(EmulatorWindow& emulator_window,
                              bool per_game_mode,
                              GraphicsSettingsPanelState& state);

}  // namespace app
}  // namespace xe

#endif  // XENIA_APP_GRAPHICS_SETTINGS_PANEL_H_
