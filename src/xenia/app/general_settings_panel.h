/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_APP_GENERAL_SETTINGS_PANEL_H_
#define XENIA_APP_GENERAL_SETTINGS_PANEL_H_

namespace xe {
namespace app {

class EmulatorWindow;

void DrawGeneralCuratedPanel(EmulatorWindow& emulator_window,
                             bool per_game_mode);

}  // namespace app
}  // namespace xe

#endif  // XENIA_APP_GENERAL_SETTINGS_PANEL_H_
