/**

 ******************************************************************************

 * Xenia : Xbox 360 Emulator Research Project                                 *

 ******************************************************************************

 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *

 * Released under the BSD license - see LICENSE in the root for more details. *

 ******************************************************************************

 */

#include "xenia/app/nxe/nxe_nav.h"

#include <algorithm>

namespace xe {

namespace app {

namespace nxe {

void NxeNav::ResetEdgeFlags() {
  state_.confirm = false;

  state_.back = false;

  state_.configure = false;

  state_.manage = false;

  state_.guide = false;

  state_.tab_prev = false;

  state_.tab_next = false;

  state_.move_left = false;

  state_.move_right = false;

  state_.move_up = false;

  state_.move_down = false;
}

void NxeNav::PollDirectionRepeat(float delta_time, bool held, bool& edge_out,

                                 float& hold_timer, bool& was_held) {
  if (!held) {
    was_held = false;

    hold_timer = 0.f;

    return;
  }

  if (!was_held) {
    edge_out = true;

    was_held = true;

    hold_timer = 0.f;

    return;
  }

  hold_timer += delta_time;

  if (hold_timer >= kRepeatDelay) {
    hold_timer = kRepeatDelay - kRepeatInterval;

    edge_out = true;
  }
}

void NxeNav::BeginFrame(ImGuiIO& io, float delta_time) {
  ResetEdgeFlags();

  if (ImGui::IsKeyPressed(ImGuiKey_GamepadFaceDown, false)) {
    state_.confirm = true;
  }

  if (ImGui::IsKeyPressed(ImGuiKey_GamepadFaceRight, false)) {
    state_.back = true;
  }

  if (ImGui::IsKeyPressed(ImGuiKey_GamepadFaceLeft, false)) {
    state_.configure = true;
  }

  if (ImGui::IsKeyPressed(ImGuiKey_GamepadFaceUp, false)) {
    state_.manage = true;
  }

  if (ImGui::IsKeyPressed(ImGuiKey_GamepadL1, false)) {
    state_.tab_prev = true;
  }

  if (ImGui::IsKeyPressed(ImGuiKey_GamepadR1, false)) {
    state_.tab_next = true;
  }

  if (ImGui::IsKeyPressed(ImGuiKey_GamepadStart, false) &&

      ImGui::IsKeyDown(ImGuiKey_GamepadL1)) {
    state_.guide = true;
  }

  if (ImGui::IsKeyPressed(ImGuiKey_Enter, false) ||

      ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false) ||

      ImGui::IsKeyPressed(ImGuiKey_Space, false)) {
    state_.confirm = true;
  }

  if (ImGui::IsKeyPressed(ImGuiKey_Escape, false) ||

      ImGui::IsKeyPressed(ImGuiKey_Backspace, false)) {
    state_.back = true;
  }

  if (ImGui::IsKeyPressed(ImGuiKey_PageUp, false) ||

      ImGui::IsKeyPressed(ImGuiKey_Q, false)) {
    state_.tab_prev = true;
  }

  if (ImGui::IsKeyPressed(ImGuiKey_PageDown, false) ||

      ImGui::IsKeyPressed(ImGuiKey_E, false)) {
    state_.tab_next = true;
  }

  if (ImGui::IsKeyPressed(ImGuiKey_F1, false)) {
    state_.guide = true;
  }

  const bool left_held =

      ImGui::IsKeyDown(ImGuiKey_LeftArrow) ||

      ImGui::IsKeyDown(ImGuiKey_GamepadDpadLeft) ||

      ImGui::IsKeyDown(ImGuiKey_GamepadLStickLeft);

  const bool right_held =

      ImGui::IsKeyDown(ImGuiKey_RightArrow) ||

      ImGui::IsKeyDown(ImGuiKey_GamepadDpadRight) ||

      ImGui::IsKeyDown(ImGuiKey_GamepadLStickRight);

  const bool up_held = ImGui::IsKeyDown(ImGuiKey_UpArrow) ||

                       ImGui::IsKeyDown(ImGuiKey_GamepadDpadUp) ||

                       ImGui::IsKeyDown(ImGuiKey_GamepadLStickUp);

  const bool down_held =

      ImGui::IsKeyDown(ImGuiKey_DownArrow) ||

      ImGui::IsKeyDown(ImGuiKey_GamepadDpadDown) ||

      ImGui::IsKeyDown(ImGuiKey_GamepadLStickDown);

  PollDirectionRepeat(delta_time, left_held, state_.move_left, left_timer_,

                      left_was_held_);

  PollDirectionRepeat(delta_time, right_held, state_.move_right, right_timer_,

                      right_was_held_);

  PollDirectionRepeat(delta_time, up_held, state_.move_up, up_timer_,

                      up_was_held_);

  PollDirectionRepeat(delta_time, down_held, state_.move_down, down_timer_,

                      down_was_held_);

  if (io.MouseWheel > 0.f) {
    state_.move_left = true;

  } else if (io.MouseWheel < 0.f) {
    state_.move_right = true;
  }
}

void NxeNav::MoveDetail(int delta, NavAxis axis) {
  (void)axis;

  if (state_.item_count <= 0) {
    return;
  }

  const int next = state_.detail_index + delta;

  state_.detail_index = std::clamp(next, 0, state_.item_count - 1);
}

void NxeNav::MoveContent(int delta, NavAxis axis) {
  if (state_.item_count <= 0) {
    return;
  }

  if (axis == NavAxis::kHorizontal && state_.column_count > 1) {
    int row = state_.content_index / state_.column_count;

    int col = state_.content_index % state_.column_count;

    col = std::clamp(col + delta, 0, state_.column_count - 1);

    state_.content_index = row * state_.column_count + col;

  } else {
    const int next = state_.content_index + delta;

    state_.content_index = std::clamp(next, 0, state_.item_count - 1);
  }
}

}  // namespace nxe

}  // namespace app

}  // namespace xe
