/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_APP_LAUNCHER_DASHBOARD_H_
#define XENIA_APP_LAUNCHER_DASHBOARD_H_

#include <functional>
#include <map>
#include <memory>
#include <string>

#include "xenia/app/library/game_library.h"
#include "xenia/ui/imgui_dialog.h"

namespace xe {
namespace ui {
class ImmediateTexture;
}
namespace app {

class EmulatorWindow;

class LauncherDashboardDialog final : public ui::ImGuiDialog {
 public:
  LauncherDashboardDialog(ui::ImGuiDrawer* imgui_drawer,
                          EmulatorWindow& emulator_window,
                          library::GameLibrary& game_library);

  void SetOnConfigureTitle(
      std::function<void(uint32_t title_id)> on_configure_title);
  void SetOnOpenLibrarySettings(std::function<void()> on_library_settings);

 protected:
  void OnDraw(ImGuiIO& io) override;

 private:
  void DrawSidebar();
  void DrawGrid();
  void DrawStatusBar();
  ui::ImmediateTexture* GetCoverTexture(const library::LibraryEntry& entry);
  std::vector<const library::LibraryEntry*> FilteredEntries() const;

  EmulatorWindow& emulator_window_;
  library::GameLibrary& game_library_;

  int sidebar_tab_ = 0;
  char search_buffer_[128] = {};
  int sort_mode_ = 0;
  int context_entry_index_ = -1;

  std::map<std::string, std::unique_ptr<ui::ImmediateTexture>> cover_textures_;

  std::function<void(uint32_t title_id)> on_configure_title_;
  std::function<void()> on_library_settings_;
};

}  // namespace app
}  // namespace xe

#endif  // XENIA_APP_LAUNCHER_DASHBOARD_H_
