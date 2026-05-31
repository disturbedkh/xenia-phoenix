/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/app/launcher_dashboard.h"

#include "xenia/app/emulator_window.h"

namespace xe {
namespace app {

LauncherDashboardDialog::LauncherDashboardDialog(
    ui::ImGuiDrawer* imgui_drawer, EmulatorWindow& emulator_window,
    library::GameLibrary& game_library, std::filesystem::path patches_root)
    : ui::ImGuiDialog(imgui_drawer),
      emulator_window_(emulator_window),
      game_library_(game_library),
      nxe_(std::make_unique<nxe::NxeDashboard>(emulator_window, game_library,
                                               std::move(patches_root))) {
  WireCallbacks();
}

void LauncherDashboardDialog::WireCallbacks() {
  nxe::NxeContext ctx;
  ctx.on_launch = [this](const std::filesystem::path& path) {
    emulator_window_.RequestLaunchTitle(path);
  };
  ctx.on_configure = on_configure_title_;
  ctx.on_library_settings = on_library_settings_;
  ctx.on_preferences = on_preferences_;
  ctx.on_netplay = on_netplay_;
  ctx.on_friends = on_friends_;
  ctx.on_guide = on_guide_;
  ctx.on_manage_profiles = on_manage_profiles_;
  ctx.on_profile_switch = [this]() {
    if (on_friends_) {
      on_friends_();
    }
  };
  nxe_->SetCallbacks(ctx);
}

void LauncherDashboardDialog::SetOnConfigureTitle(
    std::function<void(uint32_t title_id)> on_configure_title) {
  on_configure_title_ = std::move(on_configure_title);
  WireCallbacks();
}

void LauncherDashboardDialog::SetOnOpenLibrarySettings(
    std::function<void()> on_library_settings) {
  on_library_settings_ = std::move(on_library_settings);
  WireCallbacks();
}

void LauncherDashboardDialog::SetOnOpenPreferences(
    std::function<void()> on_preferences) {
  on_preferences_ = std::move(on_preferences);
  WireCallbacks();
}

void LauncherDashboardDialog::SetOnOpenNetplaySettings(
    std::function<void()> on_netplay) {
  on_netplay_ = std::move(on_netplay);
  WireCallbacks();
}

void LauncherDashboardDialog::SetOnOpenFriends(
    std::function<void()> on_friends) {
  on_friends_ = std::move(on_friends);
  WireCallbacks();
}

void LauncherDashboardDialog::SetOnOpenGuide(std::function<void()> on_guide) {
  on_guide_ = std::move(on_guide);
  WireCallbacks();
}

void LauncherDashboardDialog::SetOnManageProfiles(
    std::function<void()> on_manage_profiles) {
  on_manage_profiles_ = std::move(on_manage_profiles);
  WireCallbacks();
}

void LauncherDashboardDialog::OnDraw(ImGuiIO& io) {
  if (nxe_) {
    nxe_->Draw(io);
  }
}

}  // namespace app
}  // namespace xe
