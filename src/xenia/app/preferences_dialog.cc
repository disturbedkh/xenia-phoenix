/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/app/preferences_dialog.h"

#include "third_party/fmt/include/fmt/format.h"
#include "third_party/imgui/imgui.h"
#include "xenia/app/audio_settings_panel.h"
#include "xenia/app/cvar_settings_ui.h"
#include "xenia/app/display_settings_panel.h"
#include "xenia/app/emulator_window.h"
#include "xenia/app/general_settings_panel.h"
#include "xenia/app/graphics_settings_panel.h"
#include "xenia/app/input_settings_panel.h"
#include "xenia/app/storage_settings_panel.h"
#include "xenia/base/filesystem.h"
#include "xenia/base/system.h"
#include "xenia/config.h"
#include "xenia/kernel/xconfig.h"

DECLARE_int32(draw_resolution_scale_x);
DECLARE_int32(draw_resolution_scale_y);

namespace xe {
namespace app {

PreferencesDialog::PreferencesDialog(ui::ImGuiDrawer* imgui_drawer,
                                     EmulatorWindow& emulator_window,
                                     PreferencesTab initial_tab)
    : ui::ImGuiDialog(imgui_drawer), emulator_window_(emulator_window) {
  SetLifetimeManagedByOwner(true);
  window_title_ = fmt::format("Preferences###{}", GetWindowId());
  graphics_panel_state_.scale_x = std::max(1, cvars::draw_resolution_scale_x);
  graphics_panel_state_.scale_y = std::max(1, cvars::draw_resolution_scale_y);
  graphics_panel_state_.resolution_scale = graphics_panel_state_.scale_x;
  graphics_panel_state_.independent_scale_xy =
      (graphics_panel_state_.scale_x != graphics_panel_state_.scale_y);
  initial_tab_ = initial_tab;
  active_tab_ = initial_tab;
  pending_tab_select_frames_ = 2;
}

void PreferencesDialog::SetInitialTab(PreferencesTab tab) {
  initial_tab_ = tab;
  active_tab_ = tab;
  pending_tab_select_frames_ = 2;
}

void PreferencesDialog::SetPerGameMode(bool enabled) {
  per_game_mode_ = enabled;
}

void PreferencesDialog::DrawHeader() {
  const std::string config_path_utf8 = xe::path_to_utf8(config::config_path);
  ImGui::TextWrapped("Config: %s", config_path_utf8.c_str());
  if (ImGui::Button("Open config folder")) {
    if (!config::config_path.empty()) {
      const auto folder = config::config_path.parent_path();
      if (std::filesystem::exists(folder)) {
        LaunchFileExplorer(folder);
      }
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Open config file")) {
    if (!config::config_path.empty() &&
        std::filesystem::exists(config::config_path)) {
      LaunchFileExplorer(config::config_path);
    }
  }

  Emulator* emulator = emulator_window_.emulator();
  if (emulator && emulator->is_title_open()) {
    ImGui::Checkbox("Apply to current game only", &per_game_mode_);
    if (per_game_mode_) {
      ImGui::TextDisabled("Overrides save to config/%08X.config.toml",
                          emulator->title_id());
    }
  } else {
    per_game_mode_ = false;
  }
  ImGui::Separator();
}

void PreferencesDialog::DrawActiveTab() {
  Emulator* emulator = emulator_window_.emulator();

  if (ImGui::BeginTabBar("PreferencesTabs")) {
    for (int i = 0; i < static_cast<int>(PreferencesTab::kCount); ++i) {
      const auto tab = static_cast<PreferencesTab>(i);
      ImGuiTabItemFlags flags = 0;
      if (pending_tab_select_frames_ > 0 && active_tab_ == tab) {
        flags |= ImGuiTabItemFlags_SetSelected;
      }
      if (ImGui::BeginTabItem(PreferencesTabLabel(tab), nullptr, flags)) {
        if (pending_tab_select_frames_ <= 0) {
          active_tab_ = tab;
        }
        ImGui::EndTabItem();
      }
    }
    ImGui::EndTabBar();
    if (pending_tab_select_frames_ > 0) {
      --pending_tab_select_frames_;
    }
  }

  ImGui::BeginChild("PreferencesTabContent", ImVec2(0, 0), false);

  switch (active_tab_) {
    case PreferencesTab::kGraphics: {
      DrawGraphicsCuratedPanel(emulator_window_, per_game_mode_,
                               graphics_panel_state_);
      ImGui::Separator();
      CvarSettingsUi::DrawCategories(CategoriesForTab(active_tab_),
                                     cvar_ui_state_, emulator, per_game_mode_);
      break;
    }
    case PreferencesTab::kVideoDisplay: {
      DrawDisplayCuratedPanel(emulator_window_);
      ImGui::Separator();
      CvarSettingsUi::DrawCategories(CategoriesForTab(active_tab_),
                                     cvar_ui_state_, emulator, per_game_mode_);
      break;
    }
    case PreferencesTab::kAudio:
      DrawAudioCuratedPanel(emulator, per_game_mode_);
      ImGui::Separator();
      CvarSettingsUi::DrawCategories(CategoriesForTab(active_tab_),
                                     cvar_ui_state_, emulator, per_game_mode_);
      break;
    case PreferencesTab::kInput:
      DrawInputCuratedPanel(emulator_window_, per_game_mode_);
      ImGui::Separator();
      CvarSettingsUi::DrawCategories(CategoriesForTab(active_tab_),
                                     cvar_ui_state_, emulator, per_game_mode_);
      break;
    case PreferencesTab::kStorage:
      DrawStorageCuratedPanel(emulator, per_game_mode_);
      ImGui::Separator();
      CvarSettingsUi::DrawCategories(CategoriesForTab(active_tab_),
                                     cvar_ui_state_, emulator, per_game_mode_);
      break;
    case PreferencesTab::kGeneral:
      DrawGeneralCuratedPanel(emulator_window_, per_game_mode_);
      ImGui::Separator();
      CvarSettingsUi::DrawCategories(CategoriesForTab(active_tab_),
                                     cvar_ui_state_, emulator, per_game_mode_);
      break;
    case PreferencesTab::kCpuSystem:
    case PreferencesTab::kLogging:
      CvarSettingsUi::DrawCategories(CategoriesForTab(active_tab_),
                                     cvar_ui_state_, emulator, per_game_mode_);
      break;
    case PreferencesTab::kAdvanced: {
      const bool previous_show_advanced = cvar_ui_state_.show_advanced;
      cvar_ui_state_.show_advanced = true;
      CvarSettingsUi::DrawCategories(CategoriesForTab(active_tab_),
                                     cvar_ui_state_, emulator, per_game_mode_,
                                     false, true);
      cvar_ui_state_.show_advanced = previous_show_advanced;
      break;
    }
    case PreferencesTab::kGameOverrides:
      CvarSettingsUi::DrawGameOverridesPanel(emulator);
      break;
    case PreferencesTab::kXboxConsole:
      ImGui::TextWrapped(
          "Guest Xbox 360 console settings (timezone, locale, video mode, "
          "audio). Stored in the console profile, not "
          "xenia-canary.config.toml.");
      if (ImGui::Button("Open Xbox console settings")) {
        emulator_window_.ToggleConsoleSettingsDialog();
      }
      ImGui::TextDisabled(
          "Video resolution for games that read XConfig is also available "
          "under "
          "Graphics > Guest display resolution.");
      break;
    default:
      break;
  }

  ImGui::EndChild();
}

void PreferencesDialog::OnDraw(ImGuiIO& io) {
  if (!wants_open_) {
    emulator_window_.ScheduleClosePreferencesDialog();
    return;
  }

  ImGui::SetNextWindowSize(ImVec2(720, 620), ImGuiCond_FirstUseEver);
  bool open = true;
  const ImGuiWindowFlags window_flags =
      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse;
  if (!ImGui::Begin(window_title_.c_str(), &open, window_flags)) {
    ImGui::End();
    return;
  }

  DrawHeader();
  DrawActiveTab();

  ImGui::End();
  if (!open) {
    wants_open_ = false;
    emulator_window_.ScheduleClosePreferencesDialog();
  }
}

}  // namespace app
}  // namespace xe
