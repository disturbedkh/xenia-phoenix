/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_APP_IMGUI_GAMEPAD_NAV_H_
#define XENIA_APP_IMGUI_GAMEPAD_NAV_H_

#include <cstdint>

struct ImGuiIO;

namespace xe {
namespace app {

// Xbox-style gamepad navigation for ImGui launcher panels.
struct GamepadNavState {
  int focused_index = 0;
  int column_count = 1;
  int item_count = 0;
  bool guide_pressed = false;
  bool confirm_pressed = false;
  bool back_pressed = false;
  bool configure_pressed = false;
  bool manage_pressed = false;
  bool tab_prev = false;
  bool tab_next = false;
};

class ImGuiGamepadNav {
 public:
  // Poll SDL/XInput via ImGui IO nav inputs when available.
  static void BeginFrame(ImGuiIO& io, GamepadNavState& state);
  static void MoveFocus(GamepadNavState& state, int delta_rows, int delta_cols);
  static bool IsItemFocused(int index, const GamepadNavState& state);
  static void DrawFocusRing(int index, const GamepadNavState& state);
};

}  // namespace app
}  // namespace xe

#endif  // XENIA_APP_IMGUI_GAMEPAD_NAV_H_
