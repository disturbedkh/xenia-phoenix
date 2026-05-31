/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_APP_NXE_NXE_DRAW_H_
#define XENIA_APP_NXE_NXE_DRAW_H_

#define IMGUI_DEFINE_MATH_OPERATORS
#include "third_party/imgui/imgui.h"
#include "xenia/app/theme_manager.h"
#include "xenia/ui/immediate_drawer.h"

namespace xe {
namespace app {
namespace nxe {

struct NxePalette {
  ImU32 brand_primary = IM_COL32(144, 195, 29, 255);
  ImU32 brand_light = IM_COL32(208, 228, 161, 255);
  ImU32 brand_dark = IM_COL32(97, 146, 12, 255);
  ImU32 bg_top = IM_COL32(95, 95, 95, 255);
  ImU32 bg_bottom = IM_COL32(58, 58, 58, 255);
  ImU32 text_primary = IM_COL32(255, 255, 255, 255);
  ImU32 text_secondary = IM_COL32(204, 204, 204, 255);
  ImU32 btn_a = IM_COL32(89, 200, 83, 255);
  ImU32 btn_b = IM_COL32(229, 68, 58, 255);
  ImU32 btn_x = IM_COL32(58, 130, 229, 255);
  ImU32 btn_y = IM_COL32(242, 196, 14, 255);
};

NxePalette PaletteFromTheme(const ThemeConfig& config);

void DrawVerticalGradient(ImDrawList* dl, ImVec2 min, ImVec2 max, ImU32 top,
                          ImU32 bottom);
void DrawRadialBrandGlow(ImDrawList* dl, ImVec2 center, float radius,
                         ImU32 brand_color);
void DrawGlossyPanel(ImDrawList* dl, ImVec2 min, ImVec2 max, ImU32 fill,
                     float rounding = 8.f);
void DrawBrandGlowRect(ImDrawList* dl, ImVec2 min, ImVec2 max,
                       ImU32 brand_color, float rounding = 8.f);
void DrawMirrorReflection(ImDrawList* dl, ui::ImmediateTexture* tex,
                          ImVec2 blade_min, ImVec2 blade_max, float alpha);
void DrawImageRounded(ImDrawList* dl, ui::ImmediateTexture* tex, ImVec2 min,
                      ImVec2 max, float rounding, ImU32 tint = IM_COL32_WHITE);
void DrawImageQuad(ImDrawList* dl, ui::ImmediateTexture* tex, ImVec2 p0,
                   ImVec2 p1, ImVec2 p2, ImVec2 p3,
                   ImU32 tint = IM_COL32_WHITE);
void DrawButtonHint(ImDrawList* dl, ImFont* font, ImVec2 pos, ImU32 btn_color,
                    const char* label, const char* action);
void DrawProfileCard(ImDrawList* dl, ImFont* font, ImVec2 pos, float width,
                     const char* gamertag, uint32_t gamerscore,
                     ui::ImmediateTexture* avatar);

struct BladeLayout {
  ImVec2 center;
  float scale;
  float opacity;
  int z_order;
};

BladeLayout ComputeBladeLayout(int index, float active_index,
                               ImVec2 viewport_size);

struct CoverflowLayout {
  ImVec2 center;
  ImVec2 size;
  float rotation_y_deg;
  float opacity;
  int z_order;
};

CoverflowLayout ComputeCoverflowLayout(int index, float active_index,
                                       ImVec2 viewport_size);

}  // namespace nxe
}  // namespace app
}  // namespace xe

#endif  // XENIA_APP_NXE_NXE_DRAW_H_
