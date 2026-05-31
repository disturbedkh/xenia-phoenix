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
#include <memory>
#include <string>

#include "xenia/app/nxe/nxe_dashboard.h"
#include "xenia/ui/imgui_dialog.h"

namespace xe {
namespace app {

class EmulatorWindow;

class LauncherDashboardDialog final : public ui::ImGuiDialog {
 public:
  LauncherDashboardDialog(ui::ImGuiDrawer* imgui_drawer,
                          EmulatorWindow& emulator_window,
                          library::GameLibrary& game_library,
                          std::filesystem::path patches_root);

  void SetOnConfigureTitle(
      std::function<void(uint32_t title_id)> on_configure_title);
  void SetOnOpenLibrarySettings(std::function<void()> on_library_settings);
  void SetOnOpenPreferences(std::function<void()> on_preferences);
  void SetOnOpenNetplaySettings(std::function<void()> on_netplay);
  void SetOnOpenFriends(std::function<void()> on_friends);
  void SetOnOpenGuide(std::function<void()> on_guide);
  void SetOnManageProfiles(std::function<void()> on_manage_profiles);

 protected:
  void OnDraw(ImGuiIO& io) override;

 private:
  void WireCallbacks();

  EmulatorWindow& emulator_window_;
  library::GameLibrary& game_library_;
  std::unique_ptr<nxe::NxeDashboard> nxe_;

  std::function<void(uint32_t title_id)> on_configure_title_;
  std::function<void()> on_library_settings_;
  std::function<void()> on_preferences_;
  std::function<void()> on_netplay_;
  std::function<void()> on_friends_;
  std::function<void()> on_guide_;
  std::function<void()> on_manage_profiles_;
};

}  // namespace app
}  // namespace xe

#endif  // XENIA_APP_LAUNCHER_DASHBOARD_H_
