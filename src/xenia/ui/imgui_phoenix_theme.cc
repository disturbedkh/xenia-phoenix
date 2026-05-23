/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/ui/imgui_phoenix_theme.h"

#include "third_party/imgui/imgui.h"

namespace xe {
namespace ui {

void ApplyPhoenixTheme(ImGuiStyle* style, float dpi_scale) {
  if (!style) {
    style = &ImGui::GetStyle();
  }
  ImGui::StyleColorsDark(style);

  const float s = dpi_scale > 0.f ? dpi_scale : 1.f;
  style->WindowRounding = 8.f * s;
  style->FrameRounding = 6.f * s;
  style->PopupRounding = 6.f * s;
  style->ScrollbarRounding = 6.f * s;
  style->GrabRounding = 4.f * s;
  style->WindowPadding = ImVec2(14.f * s, 12.f * s);
  style->FramePadding = ImVec2(10.f * s, 6.f * s);
  style->ItemSpacing = ImVec2(10.f * s, 8.f * s);
  style->ItemInnerSpacing = ImVec2(8.f * s, 6.f * s);

  ImVec4 accent(0.92f, 0.38f, 0.12f, 1.f);
  ImVec4 accent_hov(1.f, 0.48f, 0.18f, 1.f);
  ImVec4 accent_act(0.78f, 0.28f, 0.08f, 1.f);
  ImVec4 bg(0.08f, 0.09f, 0.11f, 0.94f);
  ImVec4 panel(0.12f, 0.13f, 0.16f, 1.f);

  style->Colors[ImGuiCol_WindowBg] = bg;
  style->Colors[ImGuiCol_ChildBg] = panel;
  style->Colors[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.11f, 0.14f, 0.98f);
  style->Colors[ImGuiCol_Border] = ImVec4(0.22f, 0.24f, 0.28f, 0.6f);
  style->Colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.17f, 0.21f, 1.f);
  style->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.21f, 0.26f, 1.f);
  style->Colors[ImGuiCol_FrameBgActive] = ImVec4(0.24f, 0.25f, 0.30f, 1.f);
  style->Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.11f, 0.14f, 1.f);
  style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.14f, 0.15f, 0.19f, 1.f);
  style->Colors[ImGuiCol_Header] = ImVec4(accent.x, accent.y, accent.z, 0.35f);
  style->Colors[ImGuiCol_HeaderHovered] = accent_hov;
  style->Colors[ImGuiCol_HeaderActive] = accent_act;
  style->Colors[ImGuiCol_Button] = ImVec4(accent.x, accent.y, accent.z, 0.55f);
  style->Colors[ImGuiCol_ButtonHovered] = accent_hov;
  style->Colors[ImGuiCol_ButtonActive] = accent_act;
  style->Colors[ImGuiCol_CheckMark] = accent;
  style->Colors[ImGuiCol_SliderGrab] = accent;
  style->Colors[ImGuiCol_SliderGrabActive] = accent_act;
  style->Colors[ImGuiCol_Tab] = ImVec4(0.14f, 0.15f, 0.19f, 1.f);
  style->Colors[ImGuiCol_TabHovered] = accent_hov;
  style->Colors[ImGuiCol_TabSelected] = accent;
  style->Colors[ImGuiCol_Text] = ImVec4(0.92f, 0.93f, 0.95f, 1.f);
  style->Colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.52f, 0.56f, 1.f);
}

}  // namespace ui
}  // namespace xe
