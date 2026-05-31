/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_APP_GUIDE_OVERLAY_H_
#define XENIA_APP_GUIDE_OVERLAY_H_

#include "xenia/ui/imgui_dialog.h"

namespace xe {
namespace app {

class EmulatorWindow;

class GuideOverlayDialog final : public ui::ImGuiDialog {
 public:
  GuideOverlayDialog(ui::ImGuiDrawer* imgui_drawer, EmulatorWindow& window);

  void SetOpen(bool open) { user_open_ = open; }
  bool IsUserOpen() const { return user_open_; }

 protected:
  void OnDraw(ImGuiIO& io) override;

 private:
  EmulatorWindow& window_;
  bool user_open_ = false;
  int blade_index_ = 1;  // 0=settings, 1=home, 2=party
};

}  // namespace app
}  // namespace xe

#endif  // XENIA_APP_GUIDE_OVERLAY_H_
