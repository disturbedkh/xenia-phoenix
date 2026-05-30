/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/app/display_settings_panel.h"

#include "third_party/fmt/include/fmt/format.h"
#include "third_party/imgui/imgui.h"
#include "xenia/app/cvar_settings_ui.h"
#include "xenia/app/emulator_window.h"
#include "xenia/config.h"
#include "xenia/gpu/command_processor.h"
#include "xenia/gpu/graphics_system.h"
#include "xenia/ui/presenter.h"

DECLARE_string(postprocess_antialiasing);
DECLARE_string(postprocess_scaling_and_sharpening);
DECLARE_double(postprocess_ffx_cas_additional_sharpness);
DECLARE_double(postprocess_ffx_fsr_sharpness_reduction);
DECLARE_bool(postprocess_dither);

namespace xe {
namespace app {
namespace {

const char* GetCvarValueForSwapPostEffect(
    gpu::CommandProcessor::SwapPostEffect effect) {
  switch (effect) {
    case gpu::CommandProcessor::SwapPostEffect::kFxaa:
      return "fxaa";
    case gpu::CommandProcessor::SwapPostEffect::kFxaaExtreme:
      return "fxaa_extreme";
    default:
      return "";
  }
}

gpu::CommandProcessor::SwapPostEffect GetSwapPostEffectForCvarValue(
    const std::string& cvar_value) {
  if (cvar_value == GetCvarValueForSwapPostEffect(
                        gpu::CommandProcessor::SwapPostEffect::kFxaa)) {
    return gpu::CommandProcessor::SwapPostEffect::kFxaa;
  }
  if (cvar_value == GetCvarValueForSwapPostEffect(
                        gpu::CommandProcessor::SwapPostEffect::kFxaaExtreme)) {
    return gpu::CommandProcessor::SwapPostEffect::kFxaaExtreme;
  }
  return gpu::CommandProcessor::SwapPostEffect::kNone;
}

const char* GetCvarValueForGuestOutputPaintEffect(
    ui::Presenter::GuestOutputPaintConfig::Effect effect) {
  switch (effect) {
    case ui::Presenter::GuestOutputPaintConfig::Effect::kCas:
      return "cas";
    case ui::Presenter::GuestOutputPaintConfig::Effect::kFsr:
      return "fsr";
    default:
      return "";
  }
}

ui::Presenter::GuestOutputPaintConfig GetGuestOutputPaintConfigForCvars() {
  ui::Presenter::GuestOutputPaintConfig paint_config;
  paint_config.SetAllowOverscanCutoff(true);
  if (cvars::postprocess_scaling_and_sharpening == "cas") {
    paint_config.SetEffect(ui::Presenter::GuestOutputPaintConfig::Effect::kCas);
  } else if (cvars::postprocess_scaling_and_sharpening == "fsr") {
    paint_config.SetEffect(ui::Presenter::GuestOutputPaintConfig::Effect::kFsr);
  } else {
    paint_config.SetEffect(
        ui::Presenter::GuestOutputPaintConfig::Effect::kBilinear);
  }
  paint_config.SetCasAdditionalSharpness(
      float(cvars::postprocess_ffx_cas_additional_sharpness));
  paint_config.SetFsrSharpnessReduction(
      float(cvars::postprocess_ffx_fsr_sharpness_reduction));
  paint_config.SetDither(cvars::postprocess_dither);
  return paint_config;
}

}  // namespace

void DrawDisplayCuratedPanel(EmulatorWindow& emulator_window) {
  gpu::GraphicsSystem* graphics_system =
      emulator_window.emulator()->graphics_system();
  if (!graphics_system) {
    ImGui::TextWrapped("Graphics system is not available.");
    return;
  }

  ImGui::TextColored(
      ImVec4(1.f, 0.75f, 0.35f, 1.f),
      "Post-process filters run AFTER the game renders. For sharper in-game "
      "detail, use Graphics > Resolution scale (internal).");
  ImGui::Spacing();

  gpu::CommandProcessor* command_processor =
      graphics_system->command_processor();
  if (command_processor) {
    if (ImGui::TreeNodeEx(
            "Anti-aliasing",
            ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen)) {
      gpu::CommandProcessor::SwapPostEffect current_swap_post_effect =
          command_processor->GetDesiredSwapPostEffect();
      int new_swap_post_effect_index = int(current_swap_post_effect);
      ImGui::RadioButton("None", &new_swap_post_effect_index,
                         int(gpu::CommandProcessor::SwapPostEffect::kNone));
      ImGui::RadioButton(
          "NVIDIA Fast Approximate Anti-Aliasing (FXAA) [Normal Quality]",
          &new_swap_post_effect_index,
          int(gpu::CommandProcessor::SwapPostEffect::kFxaa));
      ImGui::RadioButton(
          "NVIDIA Fast Approximate Anti-Aliasing (FXAA) [Extreme Quality]",
          &new_swap_post_effect_index,
          int(gpu::CommandProcessor::SwapPostEffect::kFxaaExtreme));
      gpu::CommandProcessor::SwapPostEffect new_swap_post_effect =
          gpu::CommandProcessor::SwapPostEffect(new_swap_post_effect_index);
      if (current_swap_post_effect != new_swap_post_effect) {
        command_processor->SetDesiredSwapPostEffect(new_swap_post_effect);
      }
      if (GetSwapPostEffectForCvarValue(cvars::postprocess_antialiasing) !=
          new_swap_post_effect) {
        CvarSettingsUi::ApplyCvarChange(
            "Display", "postprocess_antialiasing",
            std::string(GetCvarValueForSwapPostEffect(new_swap_post_effect)),
            emulator_window.emulator(), false);
      }
      ImGui::TreePop();
    }
  }

  ui::Presenter* presenter = graphics_system->presenter();
  if (!presenter) {
    return;
  }

  const ui::Presenter::GuestOutputPaintConfig& current_presenter_config =
      presenter->GetGuestOutputPaintConfigFromUIThread();
  ui::Presenter::GuestOutputPaintConfig new_presenter_config =
      current_presenter_config;

  if (ImGui::TreeNodeEx(
          "Resampling and sharpening",
          ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen)) {
    int new_effect_index = int(new_presenter_config.GetEffect());
    ImGui::RadioButton(
        "None / Bilinear", &new_effect_index,
        int(ui::Presenter::GuestOutputPaintConfig::Effect::kBilinear));
    ImGui::RadioButton(
        "AMD FidelityFX Contrast Adaptive Sharpening (CAS)", &new_effect_index,
        int(ui::Presenter::GuestOutputPaintConfig::Effect::kCas));
    ImGui::RadioButton(
        "AMD FidelityFX Super Resolution 1.0 (FSR)", &new_effect_index,
        int(ui::Presenter::GuestOutputPaintConfig::Effect::kFsr));
    new_presenter_config.SetEffect(
        ui::Presenter::GuestOutputPaintConfig::Effect(new_effect_index));

    if (new_presenter_config.GetEffect() ==
            ui::Presenter::GuestOutputPaintConfig::Effect::kCas ||
        new_presenter_config.GetEffect() ==
            ui::Presenter::GuestOutputPaintConfig::Effect::kFsr) {
      ImGui::TextUnformatted(
          "FXAA is highly recommended when using CAS or FSR.");
      ImGui::Spacing();

      if (new_presenter_config.GetEffect() ==
          ui::Presenter::GuestOutputPaintConfig::Effect::kFsr) {
        float fsr_sharpness_reduction =
            new_presenter_config.GetFsrSharpnessReduction();
        const auto label = fmt::format(
            "{} %%", static_cast<int>(fsr_sharpness_reduction * 100));
        fsr_sharpness_reduction = sqrt(2.f * fsr_sharpness_reduction);
        ImGui::SliderFloat(
            "FSR sharpness reduction", &fsr_sharpness_reduction,
            ui::Presenter::GuestOutputPaintConfig::kFsrSharpnessReductionMin,
            ui::Presenter::GuestOutputPaintConfig::kFsrSharpnessReductionMax,
            label.c_str(), ImGuiSliderFlags_NoInput);
        fsr_sharpness_reduction =
            .5f * fsr_sharpness_reduction * fsr_sharpness_reduction;
        new_presenter_config.SetFsrSharpnessReduction(fsr_sharpness_reduction);
      }

      float cas_additional_sharpness =
          new_presenter_config.GetCasAdditionalSharpness();
      const auto label = fmt::format(
          "{} %%", static_cast<int>(cas_additional_sharpness * 100));
      ImGui::SliderFloat(
          "CAS additional sharpness", &cas_additional_sharpness,
          ui::Presenter::GuestOutputPaintConfig::kCasAdditionalSharpnessMin,
          ui::Presenter::GuestOutputPaintConfig::kCasAdditionalSharpnessMax,
          label.c_str(), ImGuiSliderFlags_NoInput);
      new_presenter_config.SetCasAdditionalSharpness(cas_additional_sharpness);
    }

    ImGui::TreePop();
  }

  if (ImGui::TreeNodeEx("Dithering", ImGuiTreeNodeFlags_Framed |
                                         ImGuiTreeNodeFlags_DefaultOpen)) {
    bool dither = current_presenter_config.GetDither();
    ImGui::Checkbox(
        "Dither the final output to 8bpc to make gradients smoother", &dither);
    new_presenter_config.SetDither(dither);
    ImGui::TreePop();
  }

  presenter->SetGuestOutputPaintConfigFromUIThread(new_presenter_config);

  ui::Presenter::GuestOutputPaintConfig cvars_presenter_config =
      GetGuestOutputPaintConfigForCvars();
  Emulator* emulator = emulator_window.emulator();
  if (cvars_presenter_config.GetEffect() != new_presenter_config.GetEffect()) {
    CvarSettingsUi::ApplyCvarChange(
        "Display", "postprocess_scaling_and_sharpening",
        std::string(GetCvarValueForGuestOutputPaintEffect(
            new_presenter_config.GetEffect())),
        emulator, false);
  }
  if (cvars_presenter_config.GetCasAdditionalSharpness() !=
      new_presenter_config.GetCasAdditionalSharpness()) {
    CvarSettingsUi::ApplyCvarChange(
        "Display", "postprocess_ffx_cas_additional_sharpness",
        static_cast<double>(new_presenter_config.GetCasAdditionalSharpness()),
        emulator, false);
  }
  if (cvars_presenter_config.GetFsrSharpnessReduction() !=
      new_presenter_config.GetFsrSharpnessReduction()) {
    CvarSettingsUi::ApplyCvarChange(
        "Display", "postprocess_ffx_fsr_sharpness_reduction",
        static_cast<double>(new_presenter_config.GetFsrSharpnessReduction()),
        emulator, false);
  }
  if (cvars_presenter_config.GetDither() != new_presenter_config.GetDither()) {
    CvarSettingsUi::ApplyCvarChange("Display", "postprocess_dither",
                                    new_presenter_config.GetDither(), emulator,
                                    false);
  }

  if (ImGui::TreeNodeEx("GPU trace capture", ImGuiTreeNodeFlags_Framed)) {
    ImGui::TextUnformatted(
        "Writes one frame via trace_gpu_prefix (default scratch/gpu/). "
        "Retail corpus: tests/gpu_traces/CORPUS.md.");
    if (ImGui::Button("Capture frame trace")) {
      emulator_window.GpuTraceFrame();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(F4 / GPU Trace Frame menu)");
    ImGui::TreePop();
  }
}

}  // namespace app
}  // namespace xe
