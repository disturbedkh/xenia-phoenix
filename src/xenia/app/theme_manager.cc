/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/app/theme_manager.h"

#include <fstream>

#include "third_party/imgui/imgui.h"
#include "third_party/tomlplusplus/toml.hpp"
#include "xenia/ui/imgui_phoenix_theme.h"

namespace xe {
namespace app {
namespace {

constexpr const char* kConfigFile = "cache/dashboard_theme.toml";

ImVec4 PresetAccent(DashboardThemePreset preset) {
  switch (preset) {
    case DashboardThemePreset::kXbox360Nxe:
      return ImVec4(0.565f, 0.765f, 0.114f, 1.f);
    case DashboardThemePreset::kXbox360Blades:
      return ImVec4(0.55f, 0.65f, 0.75f, 1.f);
    case DashboardThemePreset::kPhoenixDark:
    default:
      return ImVec4(0.92f, 0.38f, 0.12f, 1.f);
  }
}

}  // namespace

ThemeManager& ThemeManager::Instance() {
  static ThemeManager instance;
  return instance;
}

void ThemeManager::ApplyNxeDefaults() {
  config_.accent_color = ImVec4(0.565f, 0.765f, 0.114f, 1.f);
  config_.brand_light = ImVec4(0.816f, 0.894f, 0.631f, 1.f);
  config_.brand_dark = ImVec4(0.380f, 0.573f, 0.047f, 1.f);
  config_.bg_top = ImVec4(0.373f, 0.373f, 0.373f, 1.f);
  config_.bg_bottom = ImVec4(0.227f, 0.227f, 0.227f, 1.f);
  config_.btn_a = IM_COL32(89, 200, 83, 255);
  config_.btn_b = IM_COL32(229, 68, 58, 255);
  config_.btn_x = IM_COL32(58, 130, 229, 255);
  config_.btn_y = IM_COL32(242, 196, 14, 255);
}

void ThemeManager::Load(const std::filesystem::path& storage_root) {
  config_path_ = storage_root / kConfigFile;
  std::error_code ec;
  std::filesystem::create_directories(config_path_.parent_path(), ec);
  config_.preset = DashboardThemePreset::kXbox360Nxe;
  ApplyNxeDefaults();
  if (!std::filesystem::exists(config_path_, ec)) {
    return;
  }
  try {
    auto table = toml::parse_file(config_path_.string());
    const int preset = table["preset"].value_or(1);
    config_.preset = static_cast<DashboardThemePreset>(preset);
    config_.accent_color = PresetAccent(config_.preset);
    if (config_.preset == DashboardThemePreset::kXbox360Nxe) {
      ApplyNxeDefaults();
    }
    if (auto accent = table["accent"].as_array()) {
      if (accent->size() >= 3) {
        config_.accent_color =
            ImVec4(static_cast<float>((*accent)[0].value_or(0.565)),
                   static_cast<float>((*accent)[1].value_or(0.765)),
                   static_cast<float>((*accent)[2].value_or(0.114)), 1.f);
      }
    }
    if (auto wp = table["wallpaper"].value<std::string>()) {
      config_.wallpaper_path = *wp;
    }
    if (auto stage = table["stage"].value<std::string>()) {
      config_.stage_path = *stage;
    }
    config_.ui_sounds_enabled = table["ui_sounds"].value_or(true);
    config_.sound_volume =
        static_cast<float>(table["sound_volume"].value_or(1.0));
    config_.bgm_volume = static_cast<float>(table["bgm_volume"].value_or(0.5));
    if (auto key = table["steamgriddb_key"].value<std::string>()) {
      config_.steamgriddb_api_key = *key;
    }
  } catch (...) {
    config_.preset = DashboardThemePreset::kXbox360Nxe;
    ApplyNxeDefaults();
  }
}

void ThemeManager::Save() const {
  if (config_path_.empty()) {
    return;
  }
  toml::table root;
  root.insert("preset", static_cast<int>(config_.preset));
  root.insert("accent", toml::array{
                            config_.accent_color.x,
                            config_.accent_color.y,
                            config_.accent_color.z,
                        });
  if (!config_.wallpaper_path.empty()) {
    root.insert("wallpaper", config_.wallpaper_path.string());
  }
  if (!config_.stage_path.empty()) {
    root.insert("stage", config_.stage_path.string());
  }
  root.insert("ui_sounds", config_.ui_sounds_enabled);
  root.insert("sound_volume", config_.sound_volume);
  root.insert("bgm_volume", config_.bgm_volume);
  if (!config_.steamgriddb_api_key.empty()) {
    root.insert("steamgriddb_key", config_.steamgriddb_api_key);
  }
  std::ofstream out(config_path_);
  if (out) {
    out << root;
  }
}

void ThemeManager::SetPreset(DashboardThemePreset preset) {
  config_.preset = preset;
  config_.accent_color = PresetAccent(preset);
  if (preset == DashboardThemePreset::kXbox360Nxe) {
    ApplyNxeDefaults();
  }
}

const char* ThemeManager::PresetLabel(DashboardThemePreset preset) const {
  switch (preset) {
    case DashboardThemePreset::kXbox360Nxe:
      return "Xbox 360 NXE";
    case DashboardThemePreset::kXbox360Blades:
      return "Xbox 360 Blades";
    case DashboardThemePreset::kPhoenixDark:
    default:
      return "Phoenix Dark";
  }
}

void ThemeManager::ApplyToImGui(ImGuiStyle* style, float dpi_scale) const {
  ui::ApplyPhoenixTheme(style, dpi_scale);
  if (!style) {
    style = &ImGui::GetStyle();
  }
  const ImVec4 accent = config_.accent_color;
  const ImVec4 accent_hov(accent.x + 0.08f, accent.y + 0.08f, accent.z + 0.08f,
                          1.f);
  const ImVec4 accent_act(accent.x - 0.08f, accent.y - 0.08f, accent.z - 0.08f,
                          1.f);
  style->Colors[ImGuiCol_Header] = ImVec4(accent.x, accent.y, accent.z, 0.35f);
  style->Colors[ImGuiCol_HeaderHovered] = accent_hov;
  style->Colors[ImGuiCol_HeaderActive] = accent_act;
  style->Colors[ImGuiCol_Button] = ImVec4(accent.x, accent.y, accent.z, 0.55f);
  style->Colors[ImGuiCol_ButtonHovered] = accent_hov;
  style->Colors[ImGuiCol_ButtonActive] = accent_act;
  style->Colors[ImGuiCol_CheckMark] = accent;
  style->Colors[ImGuiCol_SliderGrab] = accent;
  style->Colors[ImGuiCol_SliderGrabActive] = accent_act;
  style->Colors[ImGuiCol_TabHovered] = accent_hov;
  style->Colors[ImGuiCol_TabSelected] = accent;
  if (config_.preset == DashboardThemePreset::kXbox360Nxe) {
    style->Colors[ImGuiCol_WindowBg] = ImVec4(0.04f, 0.06f, 0.04f, 0.96f);
  } else if (config_.preset == DashboardThemePreset::kXbox360Blades) {
    style->Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.08f, 0.12f, 0.96f);
  }
}

}  // namespace app
}  // namespace xe
