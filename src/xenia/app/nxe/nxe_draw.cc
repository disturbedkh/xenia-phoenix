/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/app/nxe/nxe_draw.h"

#include <algorithm>
#include <cmath>

namespace xe {
namespace app {
namespace nxe {

namespace {

ImU32 Vec4ToU32(const ImVec4& c, float alpha_mul = 1.f) {
  return IM_COL32(static_cast<int>(c.x * 255.f), static_cast<int>(c.y * 255.f),
                  static_cast<int>(c.z * 255.f),
                  static_cast<int>(c.w * 255.f * alpha_mul));
}

}  // namespace

NxePalette PaletteFromTheme(const ThemeConfig& config) {
  NxePalette p;
  p.brand_primary = Vec4ToU32(config.accent_color);
  p.brand_light = Vec4ToU32(config.brand_light);
  p.brand_dark = Vec4ToU32(config.brand_dark);
  p.bg_top = Vec4ToU32(config.bg_top);
  p.bg_bottom = Vec4ToU32(config.bg_bottom);
  p.text_primary = Vec4ToU32(config.text_primary);
  p.text_secondary = Vec4ToU32(config.text_secondary);
  p.btn_a = config.btn_a;
  p.btn_b = config.btn_b;
  p.btn_x = config.btn_x;
  p.btn_y = config.btn_y;
  return p;
}

void DrawVerticalGradient(ImDrawList* dl, ImVec2 min, ImVec2 max, ImU32 top,
                          ImU32 bottom) {
  dl->AddRectFilledMultiColor(min, max, top, top, bottom, bottom);
}

void DrawRadialBrandGlow(ImDrawList* dl, ImVec2 center, float radius,
                         ImU32 brand_color) {
  const ImU32 inner = (brand_color & 0x00FFFFFF) | 0x40000000;
  const ImU32 outer = (brand_color & 0x00FFFFFF) | 0x00000000;
  dl->AddCircleFilled(center, radius, outer, 64);
  dl->AddCircleFilled(center, radius * 0.5f, inner, 48);
}

void DrawGlossyPanel(ImDrawList* dl, ImVec2 min, ImVec2 max, ImU32 fill,
                     float rounding) {
  dl->AddRectFilled(min, max, fill, rounding);
  const ImVec2 gloss_max(max.x, min.y + (max.y - min.y) * 0.45f);
  const ImU32 gloss_top = IM_COL32(255, 255, 255, 40);
  const ImU32 gloss_bot = IM_COL32(255, 255, 255, 0);
  dl->AddRectFilledMultiColor(min, gloss_max, gloss_top, gloss_top, gloss_bot,
                              gloss_bot);
}

void DrawBrandGlowRect(ImDrawList* dl, ImVec2 min, ImVec2 max,
                       ImU32 brand_color, float rounding) {
  const ImU32 glow = (brand_color & 0x00FFFFFF) | 0x60000000;
  dl->AddRect(min - ImVec2(6, 6), max + ImVec2(6, 6), glow, rounding + 4.f, 0,
              8.f);
  dl->AddRect(min, max, brand_color, rounding, 0, 2.f);
}

void DrawMirrorReflection(ImDrawList* dl, ui::ImmediateTexture* tex,
                          ImVec2 blade_min, ImVec2 blade_max, float alpha) {
  if (!tex) {
    return;
  }
  const float h = blade_max.y - blade_min.y;
  ImVec2 r0(blade_min.x, blade_max.y);
  ImVec2 r1(blade_max.x, blade_max.y + h * 0.35f);
  const ImU32 tint = IM_COL32(255, 255, 255, static_cast<int>(alpha * 80.f));
  dl->AddImage(reinterpret_cast<ImTextureID>(tex), r0, r1, ImVec2(0, 1),
               ImVec2(1, 0), tint);
}

void DrawImageRounded(ImDrawList* dl, ui::ImmediateTexture* tex, ImVec2 min,
                      ImVec2 max, float rounding, ImU32 tint) {
  if (!tex) {
    return;
  }
  dl->AddImageRounded(reinterpret_cast<ImTextureID>(tex), min, max,
                      ImVec2(0, 0), ImVec2(1, 1), tint, rounding);
}

void DrawImageQuad(ImDrawList* dl, ui::ImmediateTexture* tex, ImVec2 p0,
                   ImVec2 p1, ImVec2 p2, ImVec2 p3, ImU32 tint) {
  if (!tex) {
    return;
  }
  dl->AddImageQuad(reinterpret_cast<ImTextureID>(tex), p0, p1, p2, p3,
                   ImVec2(0, 0), ImVec2(1, 0), ImVec2(1, 1), ImVec2(0, 1),
                   tint);
}

void DrawButtonHint(ImDrawList* dl, ImFont* font, ImVec2 pos, ImU32 btn_color,
                    const char* label, const char* action) {
  const float r = 12.f;
  dl->AddCircleFilled(pos + ImVec2(r, r), r, btn_color, 24);
  if (font) {
    ImGui::PushFont(font);
  }
  const ImVec2 ts = ImGui::CalcTextSize(label);
  dl->AddText(pos + ImVec2(r - ts.x * 0.5f, r - ts.y * 0.5f),
              IM_COL32(0, 0, 0, 255), label);
  dl->AddText(pos + ImVec2(r * 2.f + 6.f, r - ts.y * 0.5f),
              IM_COL32(255, 255, 255, 255), action);
  if (font) {
    ImGui::PopFont();
  }
}

void DrawProfileCard(ImDrawList* dl, ImFont* font, ImVec2 pos, float width,
                     const char* gamertag, uint32_t gamerscore,
                     ui::ImmediateTexture* avatar) {
  const float avatar_size = 48.f;
  const ImVec2 card_min = pos;
  const ImVec2 card_max(pos.x + width, pos.y + 56.f);
  DrawGlossyPanel(dl, card_min, card_max, IM_COL32(0, 0, 0, 120), 4.f);
  if (avatar) {
    DrawImageRounded(dl, avatar, pos + ImVec2(width - avatar_size - 8.f, 4.f),
                     pos + ImVec2(width - 8.f, 4.f + avatar_size), 4.f);
  }
  if (font) {
    ImGui::PushFont(font);
  }
  dl->AddText(pos + ImVec2(12.f, 8.f), IM_COL32(255, 255, 255, 255), gamertag);
  char gs_buf[32];
  snprintf(gs_buf, sizeof(gs_buf), "%u G", gamerscore);
  dl->AddText(pos + ImVec2(12.f, 30.f), IM_COL32(200, 200, 200, 255), gs_buf);
  if (font) {
    ImGui::PopFont();
  }
}

BladeLayout ComputeBladeLayout(int index, float active_index,
                               ImVec2 viewport_size) {
  const float offset = static_cast<float>(index) - active_index;
  const float vw = viewport_size.x;
  const float vh = viewport_size.y;
  BladeLayout layout{};
  layout.center =
      ImVec2(vw * 0.5f + offset * vw * 0.22f - vw * 0.20f, vh * 0.45f);
  layout.scale = std::max(0.55f, 1.f - 0.15f * std::fabs(offset));
  layout.opacity = std::clamp(1.f - std::fabs(offset), 0.f, 1.f);
  layout.z_order = 100 - static_cast<int>(std::round(std::fabs(offset)));
  return layout;
}

CoverflowLayout ComputeCoverflowLayout(int index, float active_index,
                                       ImVec2 viewport_size) {
  const float offset = static_cast<float>(index) - active_index;
  const float abs_off = std::fabs(offset);
  CoverflowLayout layout{};
  layout.center =
      ImVec2(viewport_size.x * 0.5f + offset * 140.f, viewport_size.y * 0.42f);
  const float scale = std::max(0.6f, 1.f - 0.1f * abs_off);
  layout.size = ImVec2(180.f * scale, 240.f * scale);
  layout.rotation_y_deg = offset * -10.f;
  layout.opacity = std::clamp(1.f - abs_off * 0.35f, 0.2f, 1.f);
  layout.z_order = 100 - static_cast<int>(std::round(abs_off));
  return layout;
}

}  // namespace nxe
}  // namespace app
}  // namespace xe
