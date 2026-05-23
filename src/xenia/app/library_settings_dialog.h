/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_APP_LIBRARY_SETTINGS_DIALOG_H_
#define XENIA_APP_LIBRARY_SETTINGS_DIALOG_H_

#include "xenia/app/library/game_library.h"
#include "xenia/ui/imgui_dialog.h"

namespace xe {
namespace app {

class EmulatorWindow;

class LibrarySettingsDialog final : public ui::ImGuiDialog {
 public:
  LibrarySettingsDialog(ui::ImGuiDrawer* imgui_drawer,
                        EmulatorWindow& emulator_window,
                        library::GameLibrary& game_library);

 protected:
  void OnDraw(ImGuiIO& io) override;

 private:
  EmulatorWindow& emulator_window_;
  library::GameLibrary& game_library_;
};

}  // namespace app
}  // namespace xe

#endif  // XENIA_APP_LIBRARY_SETTINGS_DIALOG_H_
