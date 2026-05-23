/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the for more details.    *
 ******************************************************************************
 */

#ifndef XENIA_UI_IMGUI_PHOENIX_THEME_H_
#define XENIA_UI_IMGUI_PHOENIX_THEME_H_

struct ImGuiStyle;

namespace xe {
namespace ui {

void ApplyPhoenixTheme(ImGuiStyle* style, float dpi_scale = 1.0f);

}  // namespace ui
}  // namespace xe

#endif  // XENIA_UI_IMGUI_PHOENIX_THEME_H_
