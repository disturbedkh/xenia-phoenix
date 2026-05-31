/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_APP_THEME_MANAGER_H_
#define XENIA_APP_THEME_MANAGER_H_

#include <filesystem>
#include <string>

#include "third_party/imgui/imgui.h"

namespace xe {
namespace app {

enum class DashboardThemePreset {
  kPhoenixDark,
  kXbox360Nxe,
  kXbox360Blades,
};

struct ThemeConfig {
  DashboardThemePreset preset = DashboardThemePreset::kPhoenixDark;
  ImVec4 accent_color{0.35f, 0.72f, 0.18f, 1.f};
  ImVec4 brand_light{0.82f, 0.89f, 0.63f, 1.f};
  ImVec4 brand_dark{0.38f, 0.57f, 0.05f, 1.f};
  ImVec4 bg_top{0.37f, 0.37f, 0.37f, 1.f};
  ImVec4 bg_bottom{0.23f, 0.23f, 0.23f, 1.f};
  ImVec4 text_primary{1.f, 1.f, 1.f, 1.f};
  ImVec4 text_secondary{0.8f, 0.8f, 0.8f, 1.f};
  ImU32 btn_a = IM_COL32(89, 200, 83, 255);
  ImU32 btn_b = IM_COL32(229, 68, 58, 255);
  ImU32 btn_x = IM_COL32(58, 130, 229, 255);
  ImU32 btn_y = IM_COL32(242, 196, 14, 255);
  std::filesystem::path wallpaper_path;
  std::filesystem::path stage_path;
  std::filesystem::path flourish_path;
  bool ui_sounds_enabled = true;
  float sound_volume = 1.f;
  float bgm_volume = 0.5f;
  std::string steamgriddb_api_key;
};

class ThemeManager {
 public:
  static ThemeManager& Instance();

  void Load(const std::filesystem::path& storage_root);
  void Save() const;
  void ApplyNxeDefaults();

  const ThemeConfig& config() const { return config_; }
  ThemeConfig& mutable_config() { return config_; }

  void ApplyToImGui(ImGuiStyle* style, float dpi_scale = 1.f) const;
  void SetPreset(DashboardThemePreset preset);
  const char* PresetLabel(DashboardThemePreset preset) const;

 private:
  ThemeManager() = default;

  std::filesystem::path config_path_;
  ThemeConfig config_;
};

}  // namespace app
}  // namespace xe

#endif  // XENIA_APP_THEME_MANAGER_H_
