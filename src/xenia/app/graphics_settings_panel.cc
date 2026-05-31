/**

 ******************************************************************************

 * Xenia : Xbox 360 Emulator Research Project                                 *

 ******************************************************************************

 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *

 * Released under the BSD license - see LICENSE in the root for more details. *

 ******************************************************************************

 */

#include "xenia/app/graphics_settings_panel.h"

#include <algorithm>

#include "third_party/fmt/include/fmt/format.h"

#include "third_party/imgui/imgui.h"

#include "xenia/app/cvar_settings_ui.h"

#include "xenia/app/emulator_window.h"

#include "xenia/config.h"

#include "xenia/emulator.h"

#include "xenia/gpu/draw_resolution_scale_limits.h"

#include "xenia/gpu/graphics_system.h"

#include "xenia/kernel/kernel_state.h"

#include "xenia/kernel/xconfig.h"

DECLARE_int32(draw_resolution_scale_x);

DECLARE_int32(draw_resolution_scale_y);

DECLARE_string(gpu);

DECLARE_bool(store_shaders);

DECLARE_string(readback_resolve);

DECLARE_bool(vsync);

DECLARE_uint64(framerate_limit);

DECLARE_int32(anisotropic_override);

DECLARE_bool(native_2x_msaa);

DECLARE_bool(async_shader_compilation);

DECLARE_bool(draw_resolution_scaled_texture_offsets);

DECLARE_bool(depth_bias_shader_offset);

namespace xe {

namespace app {

namespace {

constexpr uint32_t kDefaultGuestWidth = 1280;

constexpr uint32_t kDefaultGuestHeight = 720;

const char* ResolutionTierName(uint32_t width, uint32_t height) {
  const uint64_t pixels = static_cast<uint64_t>(width) * height;

  if (pixels <= 1280ULL * 720) {
    return "720p";
  }

  if (pixels <= 1920ULL * 1080) {
    return "1080p";
  }

  if (pixels <= 2560ULL * 1440) {
    return "1440p / 2K";
  }

  if (pixels <= 3840ULL * 2160) {
    return "4K UHD";
  }

  if (pixels <= 5120ULL * 2880) {
    return "5K";
  }

  if (pixels <= 7680ULL * 4320) {
    return "8K";
  }

  return "above 8K";
}

std::pair<uint32_t, uint32_t> GetGuestBaseResolution(Emulator* emulator) {
  if (emulator && emulator->graphics_system()) {
    const auto res = emulator->graphics_system()->GetResolution();

    if (res.first > 0 && res.second > 0) {
      return res;
    }
  }

  return {kDefaultGuestWidth, kDefaultGuestHeight};
}

std::string FormatScaleComboLabel(int scale_per_axis, uint32_t base_w,

                                  uint32_t base_h) {
  const uint32_t out_w = base_w * static_cast<uint32_t>(scale_per_axis);

  const uint32_t out_h = base_h * static_cast<uint32_t>(scale_per_axis);

  const int pixel_multiplier = scale_per_axis * scale_per_axis;

  if (scale_per_axis == 1) {
    return fmt::format("Native (1x per-axis, {}x{}, {}x pixels)",

                       base_w, base_h, pixel_multiplier);
  }

  return fmt::format(

      "{}x per-axis ({}x{}, {}x total pixels, ~{})", scale_per_axis, out_w,

      out_h, pixel_multiplier, ResolutionTierName(out_w, out_h));
}

void DrawGuestResolutionCombo(Emulator* emulator) {
  ImGui::TextWrapped(

      "Guest resolution tells the emulated Xbox 360 what display size games "

      "may use. Not all titles honor this. Relaunch the game after changing.");

  kernel::KernelState* kernel_state =

      emulator ? emulator->kernel_state() : nullptr;

  if (!kernel_state || !kernel_state->xconfig()) {
    ImGui::BeginDisabled();

    ImGui::TextUnformatted("1280x720 (default — launch a title to edit)");

    ImGui::EndDisabled();

    return;
  }

  kernel::XConfig* xconfig = kernel_state->xconfig();

  kernel::XConfigData xconfig_data = *xconfig->GetXConfig();

  const char* preview = "Unknown";

  for (const auto& opt : kernel::XVGAResolution) {
    if (opt.to_host() == xconfig_data.user.av_pack_hdmi_sz.get()) {
      preview = opt.name_.c_str();

      break;
    }
  }

  if (ImGui::BeginCombo("Guest display resolution", preview)) {
    for (const auto& opt : kernel::XVGAResolution) {
      const bool selected =

          (opt.to_host() == xconfig_data.user.av_pack_hdmi_sz.get());

      if (ImGui::Selectable(opt.name_.c_str(), selected)) {
        xconfig_data.user.av_pack_hdmi_sz = opt.to_host();

        if (opt.is_widescreen()) {
          xconfig_data.user.video_flags =

              xconfig_data.user.video_flags |

              static_cast<uint32_t>(kernel::X_VIDEO_FLAGS::Widescreen);

        } else {
          xconfig_data.user.video_flags =

              xconfig_data.user.video_flags &

              ~static_cast<uint32_t>(kernel::X_VIDEO_FLAGS::Widescreen);
        }

        xconfig->WriteXConfig(&xconfig_data);
      }

      if (selected) {
        ImGui::SetItemDefaultFocus();
      }
    }

    ImGui::EndCombo();
  }
}

void DrawResolutionSummary(uint32_t guest_w, uint32_t guest_h, int scale_x,

                           int scale_y) {
  const uint32_t out_w =

      guest_w * static_cast<uint32_t>(std::max(1, scale_x));

  const uint32_t out_h =

      guest_h * static_cast<uint32_t>(std::max(1, scale_y));

  const int pixel_multiplier = scale_x * scale_y;

  ImGui::Spacing();

  ImGui::TextWrapped(

      "Guest: %ux%u  x  Scale: %dx%d (per-axis)  =  %ux%u  (%dx total "

      "pixels, ~%s)",

      guest_w, guest_h, scale_x, scale_y, out_w, out_h, pixel_multiplier,

      ResolutionTierName(out_w, out_h));
}

}  // namespace

void DrawGraphicsCuratedPanel(EmulatorWindow& emulator_window,

                              bool per_game_mode,

                              GraphicsSettingsPanelState& state) {
  Emulator* emulator = emulator_window.emulator();

  const auto [guest_w, guest_h] = GetGuestBaseResolution(emulator);

  const int scale_x = state.independent_scale_xy ? state.scale_x

                                                 : state.resolution_scale;

  const int scale_y = state.independent_scale_xy ? state.scale_y

                                                 : state.resolution_scale;

  if (ImGui::CollapsingHeader("Resolution",

                              ImGuiTreeNodeFlags_DefaultOpen)) {
    DrawGuestResolutionCombo(emulator);

    ImGui::Spacing();

    ImGui::Separator();

    ImGui::Spacing();

    ImGui::TextWrapped(

        "Internal resolution scale (supersampling) multiplies render target "

        "size per axis. 2x per-axis means 4x total pixels. Use Video & Display "

        "for post-process scaling of the finished frame.");

    gpu::DrawResolutionScaleLimits limits =

        gpu::QueryDrawResolutionScaleLimits(true, 0);

    if (!state.independent_scale_xy) {
      const std::string preview =

          FormatScaleComboLabel(state.resolution_scale, guest_w, guest_h);

      if (ImGui::BeginCombo("draw_resolution_scale_x",

                            preview.c_str())) {
        for (int s = 1; s <= static_cast<int>(limits.hard_max); ++s) {
          const std::string label = FormatScaleComboLabel(s, guest_w, guest_h);

          if (ImGui::Selectable(label.c_str(), state.resolution_scale == s)) {
            state.resolution_scale = s;

            state.scale_x = state.scale_y = s;

            CvarSettingsUi::ApplyCvarChange("GPU", "draw_resolution_scale_x",

                                            static_cast<int32_t>(s), emulator,

                                            per_game_mode);

            CvarSettingsUi::ApplyCvarChange("GPU", "draw_resolution_scale_y",

                                            static_cast<int32_t>(s), emulator,

                                            per_game_mode);
          }
        }

        ImGui::EndCombo();
      }

      CvarSettingsUi::DrawCvarHelpMarker("draw_resolution_scale_x");
    }

    ImGui::Checkbox("Independent X/Y scale", &state.independent_scale_xy);

    if (state.independent_scale_xy) {
      if (ImGui::SliderInt("draw_resolution_scale_x", &state.scale_x, 1,

                           static_cast<int>(limits.hard_max))) {
        CvarSettingsUi::ApplyCvarChange("GPU", "draw_resolution_scale_x",

                                        static_cast<int32_t>(state.scale_x),

                                        emulator, per_game_mode);
      }

      if (ImGui::SliderInt("draw_resolution_scale_y", &state.scale_y, 1,

                           static_cast<int>(limits.hard_max))) {
        CvarSettingsUi::ApplyCvarChange("GPU", "draw_resolution_scale_y",

                                        static_cast<int32_t>(state.scale_y),

                                        emulator, per_game_mode);
      }
    }

    limits = gpu::QueryDrawResolutionScaleLimits(true, 0);

    ImGui::Text("Requested: %ux%u  Effective: %ux%u", limits.requested_x,

                limits.requested_y, limits.effective_x, limits.effective_y);

    ImGui::TextColored(ImVec4(1.f, 0.8f, 0.3f, 1.f),

                       "Restart the emulator to apply internal scale changes.");

    DrawResolutionSummary(guest_w, guest_h, scale_x, scale_y);
  }

  if (ImGui::CollapsingHeader("Backend & display mode",

                              ImGuiTreeNodeFlags_DefaultOpen)) {
    const char* gpus[] = {"d3d12", "vulkan", "null"};

    std::string gpu_value = cvars::gpu;

    CvarSettingsUi::DrawStringComboWithRecommended("gpu", gpu_value, gpus,

                                                   3, "GPU", "gpu", emulator,

                                                   per_game_mode);

    CvarSettingsUi::DrawCvarHelpMarker("gpu");

    bool vsync = cvars::vsync;

    if (ImGui::Checkbox("vsync", &vsync)) {
      CvarSettingsUi::ApplyCvarChange("GPU", "vsync", vsync, emulator,

                                      per_game_mode);
    }

    CvarSettingsUi::DrawCvarHelpMarker("vsync");

    int fps = static_cast<int>(cvars::framerate_limit);

    if (ImGui::InputInt("framerate_limit", &fps)) {
      CvarSettingsUi::ApplyCvarChange("GPU", "framerate_limit",

                                      static_cast<uint64_t>(fps), emulator,

                                      per_game_mode);
    }

    CvarSettingsUi::DrawCvarHelpMarker("framerate_limit");
  }

  if (ImGui::CollapsingHeader("Quality & compatibility",

                              ImGuiTreeNodeFlags_DefaultOpen)) {
    int aniso = cvars::anisotropic_override;

    if (ImGui::SliderInt("anisotropic_override", &aniso, -1, 16)) {
      CvarSettingsUi::ApplyCvarChange("GPU", "anisotropic_override",

                                      static_cast<int32_t>(aniso), emulator,

                                      per_game_mode);
    }

    CvarSettingsUi::DrawCvarHelpMarker("anisotropic_override");

    bool msaa = cvars::native_2x_msaa;

    if (ImGui::Checkbox("native_2x_msaa", &msaa)) {
      CvarSettingsUi::ApplyCvarChange("GPU", "native_2x_msaa", msaa, emulator,

                                      per_game_mode);
    }

    CvarSettingsUi::DrawCvarHelpMarker("native_2x_msaa");

    bool async_shaders = cvars::async_shader_compilation;

    if (ImGui::Checkbox("async_shader_compilation", &async_shaders)) {
      CvarSettingsUi::ApplyCvarChange("GPU", "async_shader_compilation",

                                      async_shaders, emulator, per_game_mode);
    }

    CvarSettingsUi::DrawCvarHelpMarker("async_shader_compilation");

    bool store = cvars::store_shaders;

    if (ImGui::Checkbox("store_shaders", &store)) {
      CvarSettingsUi::ApplyCvarChange("GPU", "store_shaders", store, emulator,

                                      per_game_mode);
    }

    CvarSettingsUi::DrawCvarHelpMarker("store_shaders");

    const char* resolve_modes[] = {"fast", "none", "some", "full"};

    std::string resolve_value = cvars::readback_resolve;

    CvarSettingsUi::DrawStringComboWithRecommended(

        "readback_resolve", resolve_value, resolve_modes, 4, "GPU",

        "readback_resolve", emulator, per_game_mode);

    CvarSettingsUi::DrawCvarHelpMarker("readback_resolve");

    bool tex_offsets = cvars::draw_resolution_scaled_texture_offsets;

    if (ImGui::Checkbox("draw_resolution_scaled_texture_offsets",
                        &tex_offsets)) {
      CvarSettingsUi::ApplyCvarChange(

          "GPU", "draw_resolution_scaled_texture_offsets", tex_offsets,

          emulator, per_game_mode);
    }

    CvarSettingsUi::DrawCvarHelpMarker(
        "draw_resolution_scaled_texture_offsets");

    bool depth_bias_shader = cvars::depth_bias_shader_offset;

    if (ImGui::Checkbox("depth_bias_shader_offset",

                        &depth_bias_shader)) {
      CvarSettingsUi::ApplyCvarChange("GPU", "depth_bias_shader_offset",

                                      depth_bias_shader, emulator,

                                      per_game_mode);
    }

    CvarSettingsUi::DrawCvarHelpMarker("depth_bias_shader_offset");
  }
}

}  // namespace app

}  // namespace xe
