/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/app/imgui_gamepad_nav.h"

#include "third_party/imgui/imgui.h"

namespace xe {
namespace app {

void ImGuiGamepadNav::BeginFrame(ImGuiIO& io, GamepadNavState& state) {
  state.confirm_pressed = false;
  state.back_pressed = false;
  state.configure_pressed = false;
  state.manage_pressed = false;
  state.guide_pressed = false;
  state.tab_prev = false;
  state.tab_next = false;

  if (!io.NavActive) {
    return;
  }

  if (ImGui::IsKeyPressed(ImGuiKey_GamepadFaceDown, false)) {
    state.confirm_pressed = true;
  }
  if (ImGui::IsKeyPressed(ImGuiKey_GamepadFaceRight, false)) {
    state.back_pressed = true;
  }
  if (ImGui::IsKeyPressed(ImGuiKey_GamepadFaceLeft, false)) {
    state.configure_pressed = true;
  }
  if (ImGui::IsKeyPressed(ImGuiKey_GamepadFaceUp, false)) {
    state.manage_pressed = true;
  }
  if (ImGui::IsKeyPressed(ImGuiKey_GamepadStart, false) &&
      ImGui::IsKeyDown(ImGuiKey_GamepadL1)) {
    state.guide_pressed = true;
  }
  if (ImGui::IsKeyPressed(ImGuiKey_GamepadL1, false)) {
    state.tab_prev = true;
  }
  if (ImGui::IsKeyPressed(ImGuiKey_GamepadR1, false)) {
    state.tab_next = true;
  }

  if (ImGui::IsKeyPressed(ImGuiKey_GamepadDpadUp, true) ||
      ImGui::IsKeyPressed(ImGuiKey_GamepadLStickUp, true)) {
    MoveFocus(state, -1, 0);
  }
  if (ImGui::IsKeyPressed(ImGuiKey_GamepadDpadDown, true) ||
      ImGui::IsKeyPressed(ImGuiKey_GamepadLStickDown, true)) {
    MoveFocus(state, 1, 0);
  }
  if (ImGui::IsKeyPressed(ImGuiKey_GamepadDpadLeft, true) ||
      ImGui::IsKeyPressed(ImGuiKey_GamepadLStickLeft, true)) {
    MoveFocus(state, 0, -1);
  }
  if (ImGui::IsKeyPressed(ImGuiKey_GamepadDpadRight, true) ||
      ImGui::IsKeyPressed(ImGuiKey_GamepadLStickRight, true)) {
    MoveFocus(state, 0, 1);
  }
}

void ImGuiGamepadNav::MoveFocus(GamepadNavState& state, int delta_rows,
                                int delta_cols) {
  if (state.item_count <= 0 || state.column_count <= 0) {
    return;
  }
  if (delta_cols != 0) {
    state.focused_index += delta_cols;
  }
  if (delta_rows != 0) {
    state.focused_index += delta_rows * state.column_count;
  }
  if (state.focused_index < 0) {
    state.focused_index = 0;
  }
  if (state.focused_index >= state.item_count) {
    state.focused_index = state.item_count - 1;
  }
}

bool ImGuiGamepadNav::IsItemFocused(int index, const GamepadNavState& state) {
  return index == state.focused_index;
}

void ImGuiGamepadNav::DrawFocusRing(int index, const GamepadNavState& state) {
  if (!IsItemFocused(index, state)) {
    return;
  }
  const ImVec2 min = ImGui::GetItemRectMin();
  const ImVec2 max = ImGui::GetItemRectMax();
  ImDrawList* draw = ImGui::GetWindowDrawList();
  draw->AddRect(min, max, IM_COL32(120, 220, 60, 255), 4.f, 0, 3.f);
}

}  // namespace app
}  // namespace xe
