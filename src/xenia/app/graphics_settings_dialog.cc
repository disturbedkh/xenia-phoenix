/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/app/graphics_settings_dialog.h"

#include "third_party/fmt/include/fmt/format.h"
#include "third_party/imgui/imgui.h"
#include "xenia/app/emulator_window.h"
#include "xenia/base/cvar.h"
#include "xenia/config.h"
#include "xenia/emulator.h"
#include "xenia/gpu/draw_resolution_scale_limits.h"
#include "xenia/gpu/gpu_flags.h"
#include "xenia/gpu/graphics_system.h"
#include "xenia/gpu/render_target_cache.h"
#include "xenia/gpu/texture_cache.h"

DECLARE_int32(draw_resolution_scale_x);
DECLARE_int32(draw_resolution_scale_y);
DECLARE_uint32(internal_display_resolution);
DECLARE_uint32(internal_display_resolution_x);
DECLARE_uint32(internal_display_resolution_y);
DECLARE_string(gpu);
DECLARE_bool(store_shaders);
DECLARE_string(readback_resolve);
DECLARE_bool(vsync);
DECLARE_uint64(framerate_limit);
DECLARE_int32(anisotropic_override);
DECLARE_bool(native_2x_msaa);
DECLARE_bool(async_shader_compilation);
DECLARE_bool(draw_resolution_scaled_texture_offsets);

namespace xe {
namespace app {

namespace {

template <typename T>
void OverrideConfigCvar(const char* name, const T& value) {
  if (!cvar::ConfigVars) {
    return;
  }
  auto it = cvar::ConfigVars->find(name);
  if (it == cvar::ConfigVars->end() || !it->second) {
    return;
  }
  if (auto* cv = dynamic_cast<cvar::ConfigVar<T>*>(it->second)) {
    cv->OverrideConfigValue(value);
  }
}

const char* ScaleLabel(int scale) {
  switch (scale) {
    case 1:
      return "Native (1x)";
    case 2:
      return "2x";
    case 3:
      return "3x (4K from 720p)";
    case 4:
      return "4x";
    case 5:
      return "5x";
    case 6:
      return "6x";
    case 7:
      return "7x";
    default:
      return "Custom";
  }
}

}  // namespace

GraphicsSettingsDialog::GraphicsSettingsDialog(ui::ImGuiDrawer* imgui_drawer,
                                               EmulatorWindow& emulator_window)
    : ui::ImGuiDialog(imgui_drawer), emulator_window_(emulator_window) {
  SetLifetimeManagedByOwner(true);
  window_title_ = fmt::format("Graphics###{}", GetWindowId());
  scale_x_ = std::max(1, cvars::draw_resolution_scale_x);
  scale_y_ = std::max(1, cvars::draw_resolution_scale_y);
  resolution_scale_ = scale_x_;
  independent_scale_xy_ = (scale_x_ != scale_y_);
}

void GraphicsSettingsDialog::DrawPerGameStar(const char* section,
                                             const char* cvar_name,
                                             const std::string& value) {
  if (!edit_per_game_ || !emulator_window_.emulator()->is_title_open()) {
    return;
  }
  ImGui::SameLine();
  const std::string save_for_game_label =
      fmt::format("Save for game##{}", cvar_name);
  if (ImGui::SmallButton(save_for_game_label.c_str())) {
    config::SaveGameConfigSetting(emulator_window_.emulator(), section,
                                  cvar_name, value);
  }
}

void GraphicsSettingsDialog::DrawResolutionSection() {
  if (!ImGui::CollapsingHeader("Resolution (internal rendering)",
                               ImGuiTreeNodeFlags_DefaultOpen)) {
    return;
  }

  ImGui::TextWrapped(
      "Internal resolution scale renders the game at higher pixel density "
      "(true supersampling). This is NOT the same as FSR/CAS in Display & "
      "Output, which only scales the finished frame.");

  gpu::DrawResolutionScaleLimits limits =
      gpu::QueryDrawResolutionScaleLimits(true, 0);

  if (!independent_scale_xy_) {
    if (ImGui::BeginCombo("Resolution scale", ScaleLabel(resolution_scale_))) {
      for (int s = 1; s <= static_cast<int>(limits.hard_max); ++s) {
        if (ImGui::Selectable(ScaleLabel(s), resolution_scale_ == s)) {
          resolution_scale_ = s;
          scale_x_ = scale_y_ = s;
          OverrideConfigCvar("draw_resolution_scale_x",
                             static_cast<int32_t>(s));
          OverrideConfigCvar("draw_resolution_scale_y",
                             static_cast<int32_t>(s));
          config::SaveConfig();
        }
      }
      ImGui::EndCombo();
    }
  }

  ImGui::Checkbox("Independent X/Y scale", &independent_scale_xy_);
  if (independent_scale_xy_) {
    if (ImGui::SliderInt("Scale X", &scale_x_, 1,
                         static_cast<int>(limits.hard_max))) {
      OverrideConfigCvar("draw_resolution_scale_x",
                         static_cast<int32_t>(scale_x_));
      config::SaveConfig();
    }
    if (ImGui::SliderInt("Scale Y", &scale_y_, 1,
                         static_cast<int>(limits.hard_max))) {
      OverrideConfigCvar("draw_resolution_scale_y",
                         static_cast<int32_t>(scale_y_));
      config::SaveConfig();
    }
  }

  limits = gpu::QueryDrawResolutionScaleLimits(true, 0);
  ImGui::Text("Requested: %ux%u  Effective: %ux%u", limits.requested_x,
              limits.requested_y, limits.effective_x, limits.effective_y);
  if (limits.requested_x == 3 && limits.requested_y == 3) {
    ImGui::TextDisabled("Typical 720p title -> ~3840x2160 internal");
  }
  if (!limits.clamp_reason.empty()) {
    ImGui::TextColored(ImVec4(1.f, 0.6f, 0.2f, 1.f), "%s",
                       limits.clamp_reason.c_str());
  }
  ImGui::TextColored(
      ImVec4(1.f, 0.8f, 0.3f, 1.f),
      "Restart the emulator or reload GPU to apply scale changes.");

  DrawPerGameStar("GPU", "draw_resolution_scale_x",
                  std::to_string(cvars::draw_resolution_scale_x));
}

void GraphicsSettingsDialog::DrawBackendSection() {
  if (!ImGui::CollapsingHeader("Backend & display mode",
                               ImGuiTreeNodeFlags_DefaultOpen)) {
    return;
  }

  const char* gpus[] = {"d3d12", "vulkan", "null"};
  int gpu_index = 0;
  if (cvars::gpu == "vulkan") {
    gpu_index = 1;
  } else if (cvars::gpu == "null") {
    gpu_index = 2;
  }
  if (ImGui::Combo("GPU backend", &gpu_index, gpus, 3)) {
    OverrideConfigCvar("gpu", std::string(gpus[gpu_index]));
    config::SaveConfig();
  }

  int res_index = static_cast<int>(cvars::internal_display_resolution);
  if (ImGui::SliderInt("Default guest resolution", &res_index, 0, 17)) {
    OverrideConfigCvar("internal_display_resolution",
                       static_cast<uint32_t>(res_index));
    config::SaveConfig();
  }
  if (res_index == 17) {
    int w = static_cast<int>(cvars::internal_display_resolution_x);
    int h = static_cast<int>(cvars::internal_display_resolution_y);
    if (ImGui::InputInt("Custom width", &w)) {
      OverrideConfigCvar("internal_display_resolution_x",
                         static_cast<uint32_t>(w));
      config::SaveConfig();
    }
    if (ImGui::InputInt("Custom height", &h)) {
      OverrideConfigCvar("internal_display_resolution_y",
                         static_cast<uint32_t>(h));
      config::SaveConfig();
    }
  }

  bool vsync = cvars::vsync;
  if (ImGui::Checkbox("VSync", &vsync)) {
    OverrideConfigCvar("vsync", vsync);
    config::SaveConfig();
  }

  int fps = static_cast<int>(cvars::framerate_limit);
  if (ImGui::InputInt("Framerate limit (0=off)", &fps)) {
    OverrideConfigCvar("framerate_limit", static_cast<uint64_t>(fps));
    config::SaveConfig();
  }
}

void GraphicsSettingsDialog::DrawQualitySection() {
  if (!ImGui::CollapsingHeader("Quality & compatibility")) {
    return;
  }

  int aniso = cvars::anisotropic_override;
  if (ImGui::SliderInt("Anisotropic filter (-1=auto)", &aniso, -1, 16)) {
    OverrideConfigCvar("anisotropic_override", static_cast<int32_t>(aniso));
    config::SaveConfig();
  }

  bool msaa = cvars::native_2x_msaa;
  if (ImGui::Checkbox("Native 2x MSAA emulation", &msaa)) {
    OverrideConfigCvar("native_2x_msaa", msaa);
    config::SaveConfig();
  }

  bool async_shaders = cvars::async_shader_compilation;
  if (ImGui::Checkbox("Async shader compilation", &async_shaders)) {
    OverrideConfigCvar("async_shader_compilation", async_shaders);
    config::SaveConfig();
  }

  bool store = cvars::store_shaders;
  if (ImGui::Checkbox("Store shader cache", &store)) {
    OverrideConfigCvar("store_shaders", store);
    config::SaveConfig();
  }

  const char* resolve_modes[] = {"fast", "none", "some", "full"};
  int resolve_index = 0;
  if (cvars::readback_resolve == "none") {
    resolve_index = 1;
  } else if (cvars::readback_resolve == "some") {
    resolve_index = 2;
  } else if (cvars::readback_resolve == "full") {
    resolve_index = 3;
  }
  if (ImGui::Combo("Readback resolve", &resolve_index, resolve_modes, 4)) {
    OverrideConfigCvar("readback_resolve",
                       std::string(resolve_modes[resolve_index]));
    config::SaveConfig();
  }

  bool tex_offsets = cvars::draw_resolution_scaled_texture_offsets;
  if (ImGui::Checkbox("Scaled texture fetch offsets", &tex_offsets)) {
    OverrideConfigCvar("draw_resolution_scaled_texture_offsets", tex_offsets);
    config::SaveConfig();
  }
}

void GraphicsSettingsDialog::OnDraw(ImGuiIO& io) {
  if (!wants_open_) {
    emulator_window_.ScheduleCloseGraphicsSettingsDialog();
    return;
  }

  ImGui::SetNextWindowSize(ImVec2(520, 560), ImGuiCond_FirstUseEver);
  bool open = true;
  const ImGuiWindowFlags window_flags =
      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse;
  if (!ImGui::Begin(window_title_.c_str(), &open, window_flags)) {
    ImGui::End();
    return;
  }

  if (emulator_window_.emulator()->is_title_open()) {
    ImGui::Checkbox("Save changes for current game only", &edit_per_game_);
    ImGui::Separator();
  }

  DrawResolutionSection();
  DrawBackendSection();
  DrawQualitySection();

  ImGui::End();
  if (!open) {
    wants_open_ = false;
    emulator_window_.ScheduleCloseGraphicsSettingsDialog();
    return;
  }
}

}  // namespace app
}  // namespace xe
