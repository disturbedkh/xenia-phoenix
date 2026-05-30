/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#define IMGUI_DEFINE_MATH_OPERATORS

#include "xenia/app/guide_overlay.h"

#include "third_party/imgui/imgui.h"
#include "xenia/app/emulator_window.h"
#include "xenia/app/nxe/nxe_draw.h"
#include "xenia/app/theme_manager.h"
#include "xenia/emulator.h"

namespace xe {
namespace app {

GuideOverlayDialog::GuideOverlayDialog(ui::ImGuiDrawer* imgui_drawer,
                                       EmulatorWindow& window)
    : ui::ImGuiDialog(imgui_drawer), window_(window) {}

void GuideOverlayDialog::OnDraw(ImGuiIO& io) {
  if (!user_open_) {
    return;
  }

  if (ImGui::IsKeyPressed(ImGuiKey_Tab) ||
      (ImGui::IsKeyPressed(ImGuiKey_GamepadStart) &&
       ImGui::IsKeyDown(ImGuiKey_GamepadL1))) {
    user_open_ = !user_open_;
  }

  const ImGuiViewport* vp = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(vp->WorkPos);
  ImGui::SetNextWindowSize(vp->WorkSize);
  ImGui::SetNextWindowBgAlpha(0.85f);

  bool open = user_open_;
  ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                           ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoSavedSettings;
  if (!ImGui::Begin("XENIA GUIDE", &open, flags)) {
    ImGui::End();
    user_open_ = open;
    return;
  }

  const nxe::NxePalette palette =
      nxe::PaletteFromTheme(ThemeManager::Instance().config());
  ImDrawList* dl = ImGui::GetWindowDrawList();
  const ImVec2 wmin = ImGui::GetWindowPos();
  const ImVec2 wmax = wmin + ImGui::GetWindowSize();
  nxe::DrawVerticalGradient(dl, wmin, wmax, palette.bg_top, palette.bg_bottom);

  ImGui::SetCursorPos(ImVec2(48.f, 48.f));
  ImFont* display = imgui_drawer()->GetDisplayFont();
  if (display) {
    ImGui::PushFont(display);
  }
  ImGui::TextUnformatted("XENIA GUIDE");
  if (display) {
    ImGui::PopFont();
  }

  if (ImGui::BeginTabBar("guide_blades")) {
    if (ImGui::BeginTabItem("Settings")) {
      blade_index_ = 0;
      if (ImGui::Button("Preferences", ImVec2(220, 36))) {
        window_.TogglePreferencesDialog();
      }
      if (ImGui::Button("Netplay", ImVec2(220, 36))) {
        window_.ToggleNetplaySettingsDialog();
      }
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Home")) {
      blade_index_ = 1;
      ImGui::TextWrapped("Quick access while playing.");
      if (ImGui::Button("Return to dashboard", ImVec2(220, 36))) {
        window_.ToggleLauncher();
        user_open_ = false;
      }
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Party")) {
      blade_index_ = 2;
      if (ImGui::Button("Friends & sessions", ImVec2(220, 36))) {
        window_.ToggleFriendsDialog();
      }
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }

  nxe::DrawButtonHint(dl, ImGui::GetFont(),
                      wmin + ImVec2(48.f, wmax.y - wmin.y - 48.f),
                      palette.btn_a, "A", "Select");
  ImGui::TextDisabled("Tab or LB+Start to close guide");
  user_open_ = open;
  ImGui::End();
}

}  // namespace app
}  // namespace xe
