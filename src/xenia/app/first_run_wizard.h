/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_APP_FIRST_RUN_WIZARD_H_
#define XENIA_APP_FIRST_RUN_WIZARD_H_

#include "xenia/ui/imgui_dialog.h"

namespace xe {
namespace app {

class EmulatorWindow;

class FirstRunWizardDialog final : public ui::ImGuiDialog {
 public:
  FirstRunWizardDialog(ui::ImGuiDrawer* imgui_drawer, EmulatorWindow& window);

  static bool IsNeeded(const std::filesystem::path& storage_root);

 protected:
  void OnDraw(ImGuiIO& io) override;

 private:
  EmulatorWindow& window_;
  int step_ = 0;
  char game_folder_[512] = {};
  bool stable_updates_ = true;
  bool completed_ = false;
};

}  // namespace app
}  // namespace xe

#endif  // XENIA_APP_FIRST_RUN_WIZARD_H_
