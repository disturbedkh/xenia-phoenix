/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/app/social_panel.h"

#include "third_party/imgui/imgui.h"
#include "xenia/app/emulator_window.h"
#include "xenia/emulator.h"
#include "xenia/kernel/kernel_state.h"
#include "xenia/kernel/xam/xam_state.h"

namespace xe {
namespace app {

SocialPanel::SocialPanel(EmulatorWindow& window) : window_(window) {}

void SocialPanel::SetOnOpenNetplaySettings(std::function<void()> cb) {
  on_netplay_settings_ = std::move(cb);
}

void SocialPanel::SetOnOpenFriends(std::function<void()> cb) {
  on_friends_ = std::move(cb);
}

void SocialPanel::Draw() {
  ImGui::TextUnformatted("Social");
  ImGui::Separator();

  auto* emulator = window_.emulator();
  if (!emulator || !emulator->kernel_state()) {
    ImGui::TextWrapped("Emulator not ready.");
    return;
  }

  const auto profile_count = emulator->kernel_state()
                                 ->xam_state()
                                 ->profile_manager()
                                 ->GetAccountCount();
  ImGui::Text("Signed-in profiles: %u", profile_count);

  if (ImGui::Button("Friends & sessions", ImVec2(-1, 0)) && on_friends_) {
    on_friends_();
  }
  if (ImGui::Button("Netplay settings", ImVec2(-1, 0)) &&
      on_netplay_settings_) {
    on_netplay_settings_();
  }
  if (ImGui::Button("Netplay status", ImVec2(-1, 0))) {
    window_.ToggleNetplayStatusDialog();
  }

  ImGui::Separator();
  ImGui::TextDisabled(
      "Network modes: Offline, Systemlink, Xbox Live\n"
      "Configure API server and interface in Netplay settings.");
}

}  // namespace app
}  // namespace xe
