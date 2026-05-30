/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#define IMGUI_DEFINE_MATH_OPERATORS

#include "xenia/app/nxe/nxe_dashboard.h"

#include <algorithm>

#include "third_party/fmt/include/fmt/format.h"
#include "third_party/imgui/imgui.h"
#include "xenia/app/emulator_window.h"
#include "xenia/app/library/game_library.h"
#include "xenia/app/nxe/nxe_assets.h"
#include "xenia/app/nxe/nxe_audio.h"
#include "xenia/app/nxe/nxe_draw.h"
#include "xenia/app/nxe/nxe_strings.h"
#include "xenia/app/theme_manager.h"
#include "xenia/emulator.h"
#include "xenia/ui/imgui_drawer.h"
#include "xenia/ui/immediate_drawer.h"

namespace xe {
namespace app {
namespace nxe {

namespace {

const char* ScreenName(NxeScreen s) {
  switch (s) {
    case NxeScreen::kDashboard:
      return "dashboard";
    case NxeScreen::kGameLibrary:
      return "library";
    case NxeScreen::kAchievements:
      return "achievements";
    case NxeScreen::kSettingsHub:
      return "settings";
    default:
      return "screen";
  }
}

}  // namespace

NxeDashboard::NxeDashboard(EmulatorWindow& window,
                           library::GameLibrary& library,
                           std::filesystem::path patches_root)
    : window_(window),
      library_(library),
      textures_(window.immediate_drawer()),
      data_(window.emulator()),
      metadata_(library.storage_root()),
      cover_fetcher_(library.storage_root()),
      patch_catalog_(std::move(patches_root)),
      content_manager_(library.storage_root(),
                       window.emulator()->content_root()) {
  NxeAssets::Instance().Initialize(library.storage_root());
  NxeStrings::Instance().Load(library.storage_root());
  NxeAudio::Instance().Initialize(library.storage_root());
  ThemeManager::Instance().Load(library.storage_root());
  if (ThemeManager::Instance().config().preset !=
      DashboardThemePreset::kXbox360Nxe) {
    ThemeManager::Instance().SetPreset(DashboardThemePreset::kXbox360Nxe);
  }
  NxeAudio::Instance().SetEnabled(
      ThemeManager::Instance().config().ui_sounds_enabled);

  ctx_.game_library = &library_;
  ctx_.metadata = &metadata_;
  ctx_.cover_fetcher = &cover_fetcher_;
  ctx_.patch_catalog = &patch_catalog_;
  ctx_.content_manager = &content_manager_;
  ctx_.detail_index_anim.SetImmediate(0.f);
  ctx_.library_focus_anim.SetImmediate(0.f);

  InitBlades();
  content_manager_.ScanPackages();
  RefreshFilteredEntries(ctx_);

  wallpaper_tex_ = textures_.LoadFile(NxeAssets::Instance().Wallpaper());
  stage_tex_ = textures_.LoadFile(NxeAssets::Instance().Stage());
  flourish_tex_ = textures_.LoadFile(NxeAssets::Instance().Flourish());
  guide_orb_tex_ =
      textures_.LoadFile(NxeAssets::Instance().Icon("xbox-25912.png"));
}

void NxeDashboard::SetCallbacks(NxeContext& ctx) {
  ctx_.on_launch = ctx.on_launch;
  ctx_.on_configure = ctx.on_configure;
  ctx_.on_library_settings = ctx.on_library_settings;
  ctx_.on_preferences = ctx.on_preferences;
  ctx_.on_netplay = ctx.on_netplay;
  ctx_.on_friends = ctx.on_friends;
  ctx_.on_guide = ctx.on_guide;
  ctx_.on_manage_profiles = ctx.on_manage_profiles;
  ctx_.on_profile_switch = ctx.on_profile_switch;
}

void NxeDashboard::InitBlades() {
  ctx_.detail_blades.clear();
  auto add = [](const char* id, const char* label, const char* desc,
                const char* icon, NxeScreen target) {
    BladeItem b;
    b.id = id;
    b.label = label;
    b.description = desc;
    b.icon_path = NxeAssets::Instance().Icon(icon);
    b.target = target;
    return b;
  };
  ctx_.detail_blades.push_back(add("tray", tr("open_tray"),
                                   "Launch a game from disc or file.",
                                   "start.png", NxeScreen::kGameLibrary));
  ctx_.detail_blades.push_back(
      add("achievements", tr("achievements"), "View unlocked achievements.",
          "achievements.png", NxeScreen::kAchievements));
  ctx_.detail_blades.push_back(add("library", tr("game_library"),
                                   "Browse your game collection.", "games.png",
                                   NxeScreen::kGameLibrary));
  ctx_.detail_blades.push_back(add("settings", tr("system_settings"),
                                   "Configure Xenia Phoenix.", "Setting.png",
                                   NxeScreen::kSettingsHub));
}

void NxeDashboard::RefreshFilteredEntries(NxeContext& ctx) {
  ctx.filtered_entries.clear();
  const auto& entries = library_.entries();
  std::string query = ctx.search_buffer;
  std::transform(query.begin(), query.end(), query.begin(), ::tolower);
  for (const auto& e : entries) {
    if (!query.empty()) {
      std::string name = e.title_name;
      std::transform(name.begin(), name.end(), name.begin(), ::tolower);
      if (name.find(query) == std::string::npos) {
        continue;
      }
    }
    ctx.filtered_entries.push_back(&e);
  }
  std::sort(
      ctx.filtered_entries.begin(), ctx.filtered_entries.end(),
      [&](const library::LibraryEntry* a, const library::LibraryEntry* b) {
        if (ctx.sort_mode == 1) {
          return a->last_played > b->last_played;
        }
        return a->title_name < b->title_name;
      });
}

ui::ImmediateTexture* NxeDashboard::CoverForEntry(
    const library::LibraryEntry& entry) {
  std::filesystem::path icon_path = library::EffectiveIconPath(entry);
  if (entry.title_id && cover_fetcher_.HasCachedCover(entry.title_id)) {
    icon_path = cover_fetcher_.CoverPathForTitle(entry.title_id);
  }
  if (icon_path.empty()) {
    return textures_.LoadFile(
        NxeAssets::Instance().Image("default-game-cover.jpg"));
  }
  return textures_.LoadFile(icon_path);
}

void NxeDashboard::NavigateTo(NxeContext& ctx, NxeScreen screen) {
  if (ctx.current_screen == screen) {
    return;
  }
  ctx.previous_screen = ctx.current_screen;
  ctx.current_screen = screen;
  ctx.transition_tween.Start(0.f, 1.f, 0.35f);
  NxeAudio::Instance().Play(NxeSound::kPanelUnfold);
}

void NxeDashboard::HandleNavigation(NxeContext& ctx, float dt) {
  auto& nav = nav_.state();
  nav.item_count = static_cast<int>(ctx.detail_blades.size());

  if (nav.back) {
    if (ctx.current_screen == NxeScreen::kProfileSwitch) {
      NavigateTo(ctx, NxeScreen::kDashboard);
      NxeAudio::Instance().Play(NxeSound::kBack);
    } else if (ctx.current_screen != NxeScreen::kDashboard) {
      NavigateTo(ctx, NxeScreen::kDashboard);
      NxeAudio::Instance().Play(NxeSound::kBack);
    }
    return;
  }
  if (nav.guide && ctx.on_guide) {
    ctx.on_guide();
    return;
  }
  if (nav.manage && ctx.current_screen == NxeScreen::kDashboard) {
    NavigateTo(ctx, NxeScreen::kProfileSwitch);
    NxeAudio::Instance().Play(NxeSound::kSelect);
    return;
  }

  if (ctx.current_screen == NxeScreen::kProfileSwitch) {
    if (nav.confirm && data_.ListProfiles().empty() && ctx.on_manage_profiles) {
      ctx.on_manage_profiles();
      NxeAudio::Instance().Play(NxeSound::kSelect);
    }
    return;
  }

  if (ctx.current_screen == NxeScreen::kDashboard) {
    if (nav.move_left && nav.focused_list == FocusList::kDetail) {
      nav_.MoveDetail(-1, NavAxis::kHorizontal);
      NxeAudio::Instance().Play(NxeSound::kFocus);
    }
    if (nav.move_right && nav.focused_list == FocusList::kDetail) {
      nav_.MoveDetail(1, NavAxis::kHorizontal);
      NxeAudio::Instance().Play(NxeSound::kFocus);
    }
    if (nav.move_up && nav.focused_list == FocusList::kDetail) {
      nav.focused_list = FocusList::kMaster;
      NxeAudio::Instance().Play(NxeSound::kFocus);
    }
    if (nav.move_down && nav.focused_list == FocusList::kMaster) {
      nav.focused_list = FocusList::kDetail;
      NxeAudio::Instance().Play(NxeSound::kFocus);
    }
    if (nav.move_left && nav.focused_list == FocusList::kMaster) {
      nav.master_index = 0;
      NxeAudio::Instance().Play(NxeSound::kFocus);
    }
    if (nav.move_right && nav.focused_list == FocusList::kMaster) {
      if (nav.master_index == 0) {
        nav.master_index = 1;
        NxeAudio::Instance().Play(NxeSound::kFocus);
      } else {
        nav.focused_list = FocusList::kDetail;
        NxeAudio::Instance().Play(NxeSound::kFocus);
      }
    }
    ctx.detail_index = nav.detail_index;
    ctx.detail_index_anim.SetTarget(static_cast<float>(ctx.detail_index));
    ctx.detail_index_anim.Update(dt);

    if (nav.confirm && nav.focused_list == FocusList::kDetail) {
      const auto& blade = ctx.detail_blades[ctx.detail_index];
      NavigateTo(ctx, blade.target);
      NxeAudio::Instance().Play(NxeSound::kSelect);
    }
    if (nav.confirm && nav.focused_list == FocusList::kMaster) {
      if (ctx.master_index == 0) {
        NavigateTo(ctx, NxeScreen::kAboutHub);
      } else if (ctx.master_index == 1) {
        NavigateTo(ctx, NxeScreen::kFriends);
      }
      NxeAudio::Instance().Play(NxeSound::kSelect);
    }
    if (nav.confirm && data_.ListProfiles().empty()) {
      if (ctx.on_manage_profiles) {
        ctx.on_manage_profiles();
      } else {
        NavigateTo(ctx, NxeScreen::kProfileSwitch);
      }
      NxeAudio::Instance().Play(NxeSound::kSelect);
    }
  } else if (ctx.current_screen == NxeScreen::kGameLibrary) {
    nav.item_count = static_cast<int>(ctx.filtered_entries.size());
    nav.column_count =
        std::max(1, static_cast<int>(ctx.filtered_entries.size()));
    if (nav.move_left) {
      nav_.MoveContent(-1, NavAxis::kHorizontal);
      ctx.library_focus_index = nav.content_index;
      ctx.library_focus_anim.SetTarget(
          static_cast<float>(ctx.library_focus_index));
      NxeAudio::Instance().Play(NxeSound::kFocus);
    }
    if (nav.move_right) {
      nav_.MoveContent(1, NavAxis::kHorizontal);
      ctx.library_focus_index = nav.content_index;
      ctx.library_focus_anim.SetTarget(
          static_cast<float>(ctx.library_focus_index));
      NxeAudio::Instance().Play(NxeSound::kFocus);
    }
    ctx.library_focus_anim.Update(dt);
    if (nav.confirm && !ctx.filtered_entries.empty()) {
      const auto* entry = ctx.filtered_entries[ctx.library_focus_index];
      if (ctx.on_launch) {
        ctx.on_launch(entry->path);
      }
      NxeAudio::Instance().Play(NxeSound::kSelect);
    }
  }
}

void NxeDashboard::DrawBackground(NxeContext& ctx, ImGuiIO& io) {
  const ImGuiViewport* vp = ImGui::GetMainViewport();
  ImDrawList* bg = ImGui::GetBackgroundDrawList();
  const NxePalette palette =
      PaletteFromTheme(ThemeManager::Instance().config());

  DrawVerticalGradient(bg, vp->Pos, vp->Pos + vp->Size, palette.bg_top,
                       palette.bg_bottom);
  if (stage_tex_) {
    bg->AddImage(reinterpret_cast<ImTextureID>(stage_tex_), vp->Pos,
                 vp->Pos + vp->Size, ImVec2(0, 0), ImVec2(1, 1),
                 IM_COL32(255, 255, 255, 180));
  }
  auto wp = ThemeManager::Instance().config().wallpaper_path;
  if (wp.empty()) {
    wp = NxeAssets::Instance().Wallpaper();
  }
  if (auto* wt = textures_.LoadFile(wp)) {
    bg->AddImage(reinterpret_cast<ImTextureID>(wt), vp->Pos, vp->Pos + vp->Size,
                 ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 100));
  } else if (wallpaper_tex_) {
    bg->AddImage(reinterpret_cast<ImTextureID>(wallpaper_tex_), vp->Pos,
                 vp->Pos + vp->Size, ImVec2(0, 0), ImVec2(1, 1),
                 IM_COL32(255, 255, 255, 100));
  }
  DrawRadialBrandGlow(bg,
                      vp->Pos + ImVec2(vp->Size.x * 0.5f, vp->Size.y * 0.3f),
                      vp->Size.x * 0.45f, palette.brand_primary);
  (void)ctx;
  (void)io;
}

void NxeDashboard::DrawChrome(NxeContext& ctx, ImGuiIO& io) {
  const ImGuiViewport* vp = ImGui::GetMainViewport();
  ImDrawList* dl = ImGui::GetForegroundDrawList();
  const NxePalette palette =
      PaletteFromTheme(ThemeManager::Instance().config());
  ImFont* display = window_.imgui_drawer()->GetDisplayFont();
  ImFont* body = ImGui::GetFont();

  const char* master_labels[] = {tr("about"), tr("my_xenia")};
  for (int i = 0; i < 2; ++i) {
    const float y = 40.f + i * 36.f;
    const ImU32 col = (ctx.master_index == i) ? palette.text_primary
                                              : IM_COL32(180, 180, 180, 120);
    if (display) {
      ImGui::PushFont(display);
    }
    dl->AddText(ImVec2(48.f, y), col, master_labels[i]);
    if (display) {
      ImGui::PopFont();
    }
  }

  auto profile = data_.ActiveProfile();
  ui::ImmediateTexture* avatar = nullptr;
  if (!profile.avatar_rgba.empty()) {
    avatar = textures_.LoadMemory(fmt::format("avatar_{}", profile.xuid),
                                  profile.avatar_rgba, profile.avatar_w,
                                  profile.avatar_h);
  }
  DrawProfileCard(dl, body, ImVec2(vp->Size.x - 280.f, 24.f), 260.f,
                  profile.gamertag.c_str(), profile.gamerscore, avatar);

  DrawButtonHint(dl, body, ImVec2(48.f, vp->Size.y - 48.f), palette.btn_a, "A",
                 tr("select"));
  DrawButtonHint(dl, body, ImVec2(180.f, vp->Size.y - 48.f), palette.btn_y, "Y",
                 tr("change_profile"));

  if (guide_orb_tex_) {
    const ImVec2 orb_min(vp->Size.x - 96.f, vp->Size.y - 96.f);
    const ImVec2 orb_max(vp->Size.x - 24.f, vp->Size.y - 24.f);
    dl->AddImage(reinterpret_cast<ImTextureID>(guide_orb_tex_), orb_min,
                 orb_max);
    if (ImGui::IsMouseHoveringRect(orb_min, orb_max) &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ctx.on_guide) {
      ctx.on_guide();
    }
  }
}

void NxeDashboard::DrawProfileCardButton(NxeContext& ctx, ImGuiIO& io) {
  const ImGuiViewport* vp = ImGui::GetMainViewport();
  const ImVec2 card_pos(vp->Size.x - 280.f, 24.f);
  const ImVec2 card_size(260.f, 56.f);
  ImGui::SetCursorScreenPos(vp->WorkPos + card_pos);
  if (ImGui::InvisibleButton("##profile_card", card_size)) {
    NavigateTo(ctx, NxeScreen::kProfileSwitch);
    NxeAudio::Instance().Play(NxeSound::kSelect);
  }
  (void)io;
}

void NxeDashboard::DrawDashboardHome(NxeContext& ctx, ImGuiIO& io) {
  const ImGuiViewport* vp = ImGui::GetMainViewport();
  ImDrawList* dl = ImGui::GetWindowDrawList();
  const NxePalette palette =
      PaletteFromTheme(ThemeManager::Instance().config());
  ImFont* display = window_.imgui_drawer()->GetDisplayFont();
  const ImVec2 win_pos = ImGui::GetWindowPos();

  const float active = ctx.detail_index_anim.current();
  const int blade_count = static_cast<int>(ctx.detail_blades.size());

  std::vector<std::pair<int, BladeLayout>> order;
  for (int i = 0; i < blade_count; ++i) {
    order.emplace_back(i, ComputeBladeLayout(i, active, vp->Size));
  }
  std::sort(order.begin(), order.end(), [](const auto& a, const auto& b) {
    return a.second.z_order < b.second.z_order;
  });

  for (const auto& [index, layout] : order) {
    const auto& blade = ctx.detail_blades[index];
    const float w = 320.f * layout.scale;
    const float h = 420.f * layout.scale;
    const ImVec2 bmin(layout.center.x - w * 0.5f, layout.center.y - h * 0.5f);
    const ImVec2 bmax(layout.center.x + w * 0.5f, layout.center.y + h * 0.5f);
    const ImVec2 screen_bmin = win_pos + bmin;
    const ImVec2 screen_bmax = win_pos + bmax;

    if (ImGui::IsMouseHoveringRect(screen_bmin, screen_bmax)) {
      if (ctx.detail_index != index) {
        nav_.state().detail_index = index;
        ctx.detail_index = index;
        ctx.detail_index_anim.SetTarget(static_cast<float>(index));
        NxeAudio::Instance().Play(NxeSound::kFocus);
      }
      if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        NavigateTo(ctx, blade.target);
        NxeAudio::Instance().Play(NxeSound::kSelect);
      }
    }

    const ImU32 fill =
        IM_COL32(20, 40, 20, static_cast<int>(220 * layout.opacity));
    DrawGlossyPanel(dl, bmin, bmax, fill, 10.f);
    if (std::fabs(static_cast<float>(index) - active) < 0.5f) {
      DrawBrandGlowRect(dl, bmin, bmax, palette.brand_primary, 10.f);
    }
    auto* hero = flourish_tex_;
    if (hero) {
      DrawImageRounded(
          dl, hero, bmin + ImVec2(8, 8), ImVec2(bmax.x - 8, bmin.y + h * 0.55f),
          8.f, IM_COL32(255, 255, 255, static_cast<int>(255 * layout.opacity)));
    }
    if (display) {
      ImGui::PushFont(display);
    }
    dl->AddText(ImVec2(bmin.x + 16.f, bmax.y - 72.f), palette.text_primary,
                blade.label.c_str());
    if (display) {
      ImGui::PopFont();
    }
    dl->AddText(ImVec2(bmin.x + 16.f, bmax.y - 40.f), palette.text_secondary,
                blade.description.c_str());
    DrawMirrorReflection(dl, hero, bmin, bmax, layout.opacity * 0.35f);
  }
  (void)io;
}

void NxeDashboard::DrawGameLibrary(NxeContext& ctx, ImGuiIO& io) {
  const ImGuiViewport* vp = ImGui::GetMainViewport();
  ImDrawList* dl = ImGui::GetWindowDrawList();
  const NxePalette palette =
      PaletteFromTheme(ThemeManager::Instance().config());
  ImFont* display = window_.imgui_drawer()->GetDisplayFont();
  const ImVec2 win_pos = ImGui::GetWindowPos();

  if (display) {
    ImGui::PushFont(display);
  }
  dl->AddText(ImVec2(48.f, 48.f), palette.text_primary, tr("game_library"));
  if (display) {
    ImGui::PopFont();
  }

  RefreshFilteredEntries(ctx);
  const int count = static_cast<int>(ctx.filtered_entries.size());
  if (count == 0) {
    dl->AddText(ImVec2(48.f, 120.f), palette.text_secondary, tr("no_games"));
    return;
  }

  const float focus_active = ctx.library_focus_anim.current();
  std::vector<std::pair<int, CoverflowLayout>> order;
  for (int i = 0; i < count; ++i) {
    order.emplace_back(i, ComputeCoverflowLayout(i, focus_active, vp->Size));
  }
  std::sort(order.begin(), order.end(), [](const auto& a, const auto& b) {
    return a.second.z_order < b.second.z_order;
  });

  for (const auto& [index, layout] : order) {
    const auto* entry = ctx.filtered_entries[index];
    auto* cover = CoverForEntry(*entry);
    const ImVec2 half(layout.size.x * 0.5f, layout.size.y * 0.5f);
    const ImVec2 c = layout.center;
    const float skew = layout.rotation_y_deg * 0.01f;
    ImVec2 p0(c.x - half.x + skew * 40.f, c.y - half.y);
    ImVec2 p1(c.x + half.x + skew * 40.f, c.y - half.y);
    ImVec2 p2(c.x + half.x - skew * 40.f, c.y + half.y);
    ImVec2 p3(c.x - half.x - skew * 40.f, c.y + half.y);

    const ImVec2 bb_min(std::min({p0.x, p1.x, p2.x, p3.x}),
                        std::min({p0.y, p1.y, p2.y, p3.y}));
    const ImVec2 bb_max(std::max({p0.x, p1.x, p2.x, p3.x}),
                        std::max({p0.y, p1.y, p2.y, p3.y}));
    const ImVec2 screen_bb_min = win_pos + bb_min;
    const ImVec2 screen_bb_max = win_pos + bb_max;
    if (ImGui::IsMouseHoveringRect(screen_bb_min, screen_bb_max)) {
      if (ctx.library_focus_index != index) {
        nav_.state().content_index = index;
        ctx.library_focus_index = index;
        ctx.library_focus_anim.SetTarget(static_cast<float>(index));
        NxeAudio::Instance().Play(NxeSound::kFocus);
      }
      if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ctx.on_launch) {
        ctx.on_launch(entry->path);
        NxeAudio::Instance().Play(NxeSound::kSelect);
      }
    }

    const ImU32 tint =
        IM_COL32(255, 255, 255, static_cast<int>(255 * layout.opacity));
    if (cover) {
      DrawImageQuad(dl, cover, p0, p1, p2, p3, tint);
    }
    if (std::fabs(static_cast<float>(index) - focus_active) < 0.5f) {
      DrawBrandGlowRect(dl, p0, p2, palette.brand_primary, 4.f);
      dl->AddText(ImVec2(c.x - 80.f, c.y + half.y + 8.f), palette.text_primary,
                  entry->title_name.c_str());
    }
  }

  ImGui::SetCursorPos(ImVec2(vp->Size.x - 280.f, 100.f));
  ImGui::BeginChild("lib_detail", ImVec2(240.f, 400.f), true);
  if (ctx.library_focus_index >= 0 &&
      ctx.library_focus_index < static_cast<int>(ctx.filtered_entries.size())) {
    const auto* entry = ctx.filtered_entries[ctx.library_focus_index];
    ImGui::TextWrapped("%s", entry->title_name.c_str());
    ImGui::TextDisabled("%s", entry->title_id_hex.c_str());
    if (entry->title_id && ctx.metadata) {
      if (auto meta = ctx.metadata->Lookup(entry->title_id)) {
        if (!meta->developer.empty()) {
          ImGui::Text("Dev: %s", meta->developer.c_str());
        }
        if (!meta->publisher.empty()) {
          ImGui::Text("Pub: %s", meta->publisher.c_str());
        }
      }
    }
    if (ImGui::Button("Play", ImVec2(-1, 0)) && ctx.on_launch) {
      ctx.on_launch(entry->path);
    }
    if (ImGui::Button("Configure", ImVec2(-1, 0)) && ctx.on_configure &&
        entry->title_id) {
      ctx.on_configure(entry->title_id);
    }
  }
  ImGui::EndChild();
}

void NxeDashboard::DrawAchievements(NxeContext& ctx, ImGuiIO& io) {
  const NxePalette palette =
      PaletteFromTheme(ThemeManager::Instance().config());
  ImDrawList* dl = ImGui::GetWindowDrawList();
  ImFont* display = window_.imgui_drawer()->GetDisplayFont();
  if (display) {
    ImGui::PushFont(display);
  }
  dl->AddText(ImVec2(48.f, 48.f), palette.text_primary, tr("achievements"));
  if (display) {
    ImGui::PopFont();
  }

  auto profile = data_.ActiveProfile();
  auto titles = data_.ListPlayedTitles(profile.xuid);
  ImGui::SetCursorPos(ImVec2(48.f, 100.f));
  ImGui::BeginChild("ach_titles", ImVec2(260.f, 500.f), true);
  for (size_t i = 0; i < titles.size(); ++i) {
    const auto& t = titles[i];
    char label[256];
    snprintf(label, sizeof(label), "%s (%u/%u)", t.title_name.c_str(),
             t.unlocked, t.total);
    if (ImGui::Selectable(label,
                          ctx.achievement_title_index == static_cast<int>(i))) {
      ctx.achievement_title_index = static_cast<int>(i);
    }
  }
  ImGui::EndChild();
  ImGui::SameLine();
  ImGui::BeginChild("ach_list", ImVec2(600.f, 500.f), true);
  if (ctx.achievement_title_index >= 0 &&
      ctx.achievement_title_index < static_cast<int>(titles.size())) {
    const auto& t = titles[ctx.achievement_title_index];
    auto achievements = data_.AchievementsForTitle(profile.xuid, t.title_id);
    for (const auto& a : achievements) {
      ImGui::Separator();
      ImGui::Text("%s %s", a.unlocked ? "[X]" : "[ ]", a.label.c_str());
      ImGui::TextDisabled("%s", a.unlocked ? a.description.c_str()
                                           : a.unachieved_description.c_str());
      ImGui::Text("%uG", a.gamerscore);
    }
  }
  ImGui::EndChild();
}

void NxeDashboard::DrawSettingsHub(NxeContext& ctx, ImGuiIO& io) {
  const NxePalette palette =
      PaletteFromTheme(ThemeManager::Instance().config());
  ImDrawList* dl = ImGui::GetWindowDrawList();
  ImFont* display = window_.imgui_drawer()->GetDisplayFont();
  if (display) {
    ImGui::PushFont(display);
  }
  dl->AddText(ImVec2(48.f, 48.f), palette.text_primary, tr("settings"));
  if (display) {
    ImGui::PopFont();
  }

  ImGui::SetCursorPos(ImVec2(48.f, 120.f));
  if (ImGui::Button(tr("core"), ImVec2(200, 40))) {
    NavigateTo(ctx, NxeScreen::kSettingsCore);
  }
  if (ImGui::Button(tr("colors"), ImVec2(200, 40))) {
    NavigateTo(ctx, NxeScreen::kSettingsColors);
  }
  if (ImGui::Button(tr("system"), ImVec2(200, 40))) {
    NavigateTo(ctx, NxeScreen::kSettingsSystem);
  }
  if (ImGui::Button(tr("audio"), ImVec2(200, 40))) {
    NavigateTo(ctx, NxeScreen::kSettingsAudio);
  }
  if (ImGui::Button(tr("display"), ImVec2(200, 40))) {
    NavigateTo(ctx, NxeScreen::kSettingsDisplay);
  }
  if (ImGui::Button(tr("config"), ImVec2(200, 40))) {
    NavigateTo(ctx, NxeScreen::kSettingsConfig);
  }
  if (ImGui::Button(tr("patches"), ImVec2(200, 40))) {
    NavigateTo(ctx, NxeScreen::kPatchesManager);
  }
  if (ImGui::Button(tr("content"), ImVec2(200, 40))) {
    NavigateTo(ctx, NxeScreen::kContentManager);
  }
  if (ImGui::Button(tr("friends"), ImVec2(200, 40))) {
    NavigateTo(ctx, NxeScreen::kFriends);
  }
  if (ImGui::Button("Library folders", ImVec2(200, 40)) &&
      ctx.on_library_settings) {
    ctx.on_library_settings();
  }
  if (ImGui::Button("Preferences", ImVec2(200, 40)) && ctx.on_preferences) {
    ctx.on_preferences();
  }
}

void NxeDashboard::DrawSettingsSub(NxeContext& ctx, ImGuiIO& io,
                                   NxeScreen sub) {
  ImGui::SetCursorPos(ImVec2(48.f, 48.f));
  auto& theme = ThemeManager::Instance().mutable_config();
  if (sub == NxeScreen::kSettingsCore) {
    ImGui::Text("Dashboard preset");
    int preset = static_cast<int>(theme.preset);
    if (ImGui::Combo("Preset", &preset,
                     "Phoenix Dark\0Xbox 360 NXE\0Xbox 360 Blades\0")) {
      ThemeManager::Instance().SetPreset(
          static_cast<DashboardThemePreset>(preset));
    }
    if (ImGui::Button("Netplay settings") && ctx.on_netplay) {
      ctx.on_netplay();
    }
    if (ImGui::Button("Save")) {
      ThemeManager::Instance().Save();
    }
  } else if (sub == NxeScreen::kSettingsColors) {
    ImGui::Text("NXE palette (13 colors)");
    ImGui::ColorEdit3("Brand primary", &theme.accent_color.x);
    ImGui::ColorEdit3("Brand light", &theme.brand_light.x);
    ImGui::ColorEdit3("Brand dark", &theme.brand_dark.x);
    ImGui::ColorEdit3("Background top", &theme.bg_top.x);
    ImGui::ColorEdit3("Background bottom", &theme.bg_bottom.x);
    ImGui::ColorEdit3("Text primary", &theme.text_primary.x);
    ImGui::ColorEdit3("Text secondary", &theme.text_secondary.x);
    ImVec4 btn_a = ImGui::ColorConvertU32ToFloat4(theme.btn_a);
    ImVec4 btn_b = ImGui::ColorConvertU32ToFloat4(theme.btn_b);
    ImVec4 btn_x = ImGui::ColorConvertU32ToFloat4(theme.btn_x);
    ImVec4 btn_y = ImGui::ColorConvertU32ToFloat4(theme.btn_y);
    if (ImGui::ColorEdit3("Button A", &btn_a.x)) {
      theme.btn_a = ImGui::ColorConvertFloat4ToU32(btn_a);
    }
    if (ImGui::ColorEdit3("Button B", &btn_b.x)) {
      theme.btn_b = ImGui::ColorConvertFloat4ToU32(btn_b);
    }
    if (ImGui::ColorEdit3("Button X", &btn_x.x)) {
      theme.btn_x = ImGui::ColorConvertFloat4ToU32(btn_x);
    }
    if (ImGui::ColorEdit3("Button Y", &btn_y.x)) {
      theme.btn_y = ImGui::ColorConvertFloat4ToU32(btn_y);
    }
    if (ImGui::Button("Save theme")) {
      ThemeManager::Instance().Save();
    }
  } else if (sub == NxeScreen::kSettingsSystem) {
    ImGui::Text("System");
    ImGui::Text("Storage: %s", library_.storage_root().string().c_str());
    if (auto* emu = window_.emulator()) {
      ImGui::Text("Content root: %s", emu->content_root().string().c_str());
    }
    ImGui::Text("Games in library: %zu", library_.entries().size());
    if (ImGui::Button("Rescan library")) {
      library_.StartScan();
      RefreshFilteredEntries(ctx);
    }
    if (ImGui::Button("Rescan content packages")) {
      content_manager_.ScanPackages();
    }
  } else if (sub == NxeScreen::kSettingsAudio) {
    ImGui::Checkbox("UI sounds", &theme.ui_sounds_enabled);
    ImGui::SliderFloat("Sound volume", &theme.sound_volume, 0.f, 1.f);
    ImGui::SliderFloat("BGM volume", &theme.bgm_volume, 0.f, 1.f);
    NxeAudio::Instance().SetEnabled(theme.ui_sounds_enabled);
    NxeAudio::Instance().SetSoundVolume(theme.sound_volume);
    NxeAudio::Instance().SetBgmVolume(theme.bgm_volume);
    if (ImGui::Button("Test focus sound")) {
      NxeAudio::Instance().Play(NxeSound::kFocus);
    }
    if (ImGui::Button("Save")) {
      ThemeManager::Instance().Save();
    }
  } else if (sub == NxeScreen::kSettingsDisplay) {
    ImGui::Text("Wallpaper path (empty = bundled default)");
    static char wp_buf[512] = {};
    if (wp_buf[0] == 0 && !theme.wallpaper_path.empty()) {
      strncpy(wp_buf, theme.wallpaper_path.string().c_str(),
              sizeof(wp_buf) - 1);
    }
    if (ImGui::InputText("Wallpaper", wp_buf, sizeof(wp_buf))) {
      theme.wallpaper_path = wp_buf;
    }
    static char sgdb_buf[256] = {};
    if (sgdb_buf[0] == 0 && !theme.steamgriddb_api_key.empty()) {
      strncpy(sgdb_buf, theme.steamgriddb_api_key.c_str(),
              sizeof(sgdb_buf) - 1);
    }
    if (ImGui::InputText("SteamGridDB API key", sgdb_buf, sizeof(sgdb_buf))) {
      theme.steamgriddb_api_key = sgdb_buf;
    }
    if (ImGui::Button("Save")) {
      ThemeManager::Instance().Save();
      wallpaper_tex_ = textures_.LoadFile(theme.wallpaper_path);
    }
  } else if (sub == NxeScreen::kSettingsConfig) {
    ImGui::Text("Emulator configuration");
    if (ImGui::Button("Open preferences") && ctx.on_preferences) {
      ctx.on_preferences();
    }
    if (ImGui::Button("Library folders") && ctx.on_library_settings) {
      ctx.on_library_settings();
    }
    if (ctx.library_focus_index >= 0 &&
        ctx.library_focus_index <
            static_cast<int>(ctx.filtered_entries.size()) &&
        ctx.on_configure) {
      const auto* entry = ctx.filtered_entries[ctx.library_focus_index];
      if (entry->title_id && ImGui::Button("Configure selected title")) {
        ctx.on_configure(entry->title_id);
      }
    }
  }
}

void NxeDashboard::DrawProfilePanel(NxeContext& ctx, ImGuiIO& io) {
  const ImGuiViewport* vp = ImGui::GetMainViewport();
  ImDrawList* fg = ImGui::GetForegroundDrawList();
  const NxePalette palette =
      PaletteFromTheme(ThemeManager::Instance().config());

  fg->AddRectFilled(vp->WorkPos, vp->WorkPos + vp->WorkSize,
                    IM_COL32(0, 0, 0, 140));

  const ImVec2 panel_size(480.f, 320.f);
  const ImVec2 center = vp->GetCenter();
  const ImVec2 pmin(center.x - panel_size.x * 0.5f,
                    center.y - panel_size.y * 0.5f);
  const ImVec2 pmax(center.x + panel_size.x * 0.5f,
                    center.y + panel_size.y * 0.5f);
  DrawGlossyPanel(fg, pmin, pmax, IM_COL32(20, 35, 20, 240), 12.f);

  ImGui::SetNextWindowPos(pmin);
  ImGui::SetNextWindowSize(panel_size);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.f, 24.f));
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 0.f));
  ImGuiWindowFlags panel_flags =
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNavFocus;
  if (ImGui::Begin("##nxe_profile_panel", nullptr, panel_flags)) {
    ImFont* display = window_.imgui_drawer()->GetDisplayFont();
    if (display) {
      ImGui::PushFont(display);
    }
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(palette.text_primary),
                       "%s", tr("change_profile"));
    if (display) {
      ImGui::PopFont();
    }
    ImGui::Spacing();

    auto profiles = data_.ListProfiles();
    if (profiles.empty()) {
      ImGui::TextWrapped(
          "No signed-in profiles. Sign in or create a profile to continue.");
      if (ImGui::Button("Sign in / Create profile", ImVec2(-1, 40)) &&
          ctx.on_manage_profiles) {
        ctx.on_manage_profiles();
      }
      if (ImGui::Button("Back", ImVec2(-1, 0))) {
        NavigateTo(ctx, NxeScreen::kDashboard);
        NxeAudio::Instance().Play(NxeSound::kBack);
      }
    } else {
      for (const auto& p : profiles) {
        const bool active = data_.active_profile_slot() == p.slot;
        char label[256];
        snprintf(label, sizeof(label), "%s%s", active ? "> " : "  ",
                 p.gamertag.c_str());
        if (ImGui::Selectable(label, active, 0, ImVec2(-1, 32))) {
          data_.SetActiveProfileSlot(p.slot);
          NavigateTo(ctx, NxeScreen::kDashboard);
          NxeAudio::Instance().Play(NxeSound::kSelect);
        }
        ImGui::SameLine();
        ImGui::TextDisabled("%u G", p.gamerscore);
      }
      ImGui::Spacing();
      if (ImGui::Button("Back", ImVec2(-1, 0))) {
        NavigateTo(ctx, NxeScreen::kDashboard);
        NxeAudio::Instance().Play(NxeSound::kBack);
      }
    }
  }
  ImGui::End();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar();
  (void)io;
}

void NxeDashboard::DrawNoProfileBanner(NxeContext& ctx, ImGuiIO& io) {
  if (!data_.ListProfiles().empty()) {
    return;
  }
  const ImGuiViewport* vp = ImGui::GetMainViewport();
  ImDrawList* fg = ImGui::GetForegroundDrawList();
  const NxePalette palette =
      PaletteFromTheme(ThemeManager::Instance().config());

  const ImVec2 banner_size(520.f, 56.f);
  const ImVec2 center = vp->GetCenter();
  const ImVec2 bmin(center.x - banner_size.x * 0.5f, 110.f);
  const ImVec2 bmax(bmin.x + banner_size.x, bmin.y + banner_size.y);
  DrawGlossyPanel(fg, bmin, bmax, IM_COL32(30, 50, 30, 220), 8.f);
  fg->AddText(ImVec2(bmin.x + 16.f, bmin.y + 18.f), palette.text_primary,
              "No profile signed in - click here or press Enter to sign in");

  if (ImGui::IsMouseHoveringRect(bmin, bmax) &&
      ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
    if (ctx.on_manage_profiles) {
      ctx.on_manage_profiles();
    } else {
      NavigateTo(ctx, NxeScreen::kProfileSwitch);
    }
    NxeAudio::Instance().Play(NxeSound::kSelect);
  }
  (void)io;
}

void NxeDashboard::DrawProfileSwitch(NxeContext& ctx, ImGuiIO& io) {
  DrawProfilePanel(ctx, io);
}

void NxeDashboard::DrawFriends(NxeContext& ctx, ImGuiIO& io) {
  const NxePalette palette =
      PaletteFromTheme(ThemeManager::Instance().config());
  ImDrawList* dl = ImGui::GetWindowDrawList();
  ImFont* display = window_.imgui_drawer()->GetDisplayFont();
  if (display) {
    ImGui::PushFont(display);
  }
  dl->AddText(ImVec2(48.f, 48.f), palette.text_primary, tr("friends"));
  if (display) {
    ImGui::PopFont();
  }

  auto profile = data_.ActiveProfile();
  auto friends = data_.ListFriends(profile.xuid);
  ImGui::SetCursorPos(ImVec2(48.f, 100.f));
  ImGui::BeginChild("friends_list", ImVec2(640.f, 500.f), true);
  if (friends.empty()) {
    ImGui::TextDisabled("No friends on this profile.");
  }
  for (const auto& f : friends) {
    ImGui::Separator();
    ImGui::Text("%s %s", f.online ? "[Online]" : "[Offline]",
                f.gamertag.c_str());
    ImGui::TextDisabled("%s", f.presence.c_str());
  }
  ImGui::EndChild();
}

void NxeDashboard::DrawContentManager(NxeContext& ctx, ImGuiIO& io) {
  const NxePalette palette =
      PaletteFromTheme(ThemeManager::Instance().config());
  ImDrawList* dl = ImGui::GetWindowDrawList();
  ImFont* display = window_.imgui_drawer()->GetDisplayFont();
  if (display) {
    ImGui::PushFont(display);
  }
  dl->AddText(ImVec2(48.f, 48.f), palette.text_primary, tr("content"));
  if (display) {
    ImGui::PopFont();
  }

  ImGui::SetCursorPos(ImVec2(48.f, 100.f));
  if (ImGui::Button("Rescan packages")) {
    content_manager_.ScanPackages();
  }
  ImGui::BeginChild("content_list", ImVec2(720.f, 500.f), true);
  const auto& packages = content_manager_.packages();
  if (packages.empty()) {
    ImGui::TextDisabled("No DLC/TU packages found.");
  }
  for (const auto& pkg : packages) {
    ImGui::Separator();
    ImGui::Text("%s", pkg.title_name.c_str());
    ImGui::TextDisabled("%s | %08X | %s", pkg.content_type.c_str(),
                        pkg.title_id,
                        pkg.source_path.filename().string().c_str());
    ImGui::SameLine();
    if (ImGui::SmallButton("Install")) {
      std::string error;
      content_manager_.InstallPackage(*window_.emulator(), pkg, error);
      if (!error.empty()) {
        ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "%s", error.c_str());
      }
    }
  }
  ImGui::EndChild();
  (void)ctx;
}

void NxeDashboard::DrawAboutHub(NxeContext& ctx, ImGuiIO& io) {
  ImGui::SetCursorPos(ImVec2(48.f, 48.f));
  ImGui::TextWrapped("%s", tr("welcome"));
  ImGui::Text("Xenia Phoenix NXE Dashboard");
}

void NxeDashboard::DrawPatchesManager(NxeContext& ctx, ImGuiIO& io) {
  ImGui::SetCursorPos(ImVec2(48.f, 48.f));
  ImGui::Text("Patches");
  if (!ctx.filtered_entries.empty()) {
    const auto* entry = ctx.filtered_entries[0];
    if (entry->title_id && ctx.patch_catalog) {
      auto patches = ctx.patch_catalog->PatchesForTitle(entry->title_id);
      for (auto& p : patches) {
        bool enabled = p.enabled;
        if (ImGui::Checkbox(p.name.c_str(), &enabled)) {
          std::string error;
          ctx.patch_catalog->SetPatchEnabled(entry->title_id, p.patch_id,
                                             enabled, error);
        }
        ImGui::TextDisabled("%s", p.description.c_str());
      }
    }
  }
}

void NxeDashboard::DrawScreen(NxeContext& ctx, ImGuiIO& io) {
  switch (ctx.current_screen) {
    case NxeScreen::kDashboard:
      DrawDashboardHome(ctx, io);
      break;
    case NxeScreen::kGameLibrary:
      DrawGameLibrary(ctx, io);
      break;
    case NxeScreen::kAchievements:
      DrawAchievements(ctx, io);
      break;
    case NxeScreen::kSettingsHub:
      DrawSettingsHub(ctx, io);
      break;
    case NxeScreen::kSettingsColors:
    case NxeScreen::kSettingsAudio:
    case NxeScreen::kSettingsDisplay:
    case NxeScreen::kSettingsSystem:
    case NxeScreen::kSettingsConfig:
      DrawSettingsSub(ctx, io, ctx.current_screen);
      break;
    case NxeScreen::kPatchesManager:
      DrawPatchesManager(ctx, io);
      break;
    case NxeScreen::kProfileSwitch:
      break;
    case NxeScreen::kFriends:
      DrawFriends(ctx, io);
      break;
    case NxeScreen::kContentManager:
      DrawContentManager(ctx, io);
      break;
    case NxeScreen::kAboutHub:
      DrawAboutHub(ctx, io);
      break;
    default:
      DrawDashboardHome(ctx, io);
      break;
  }
}

void NxeDashboard::Draw(ImGuiIO& io) {
  ctx_.overlay_while_playing = window_.emulator()->is_title_open();
  ThemeManager::Instance().ApplyToImGui(&ImGui::GetStyle());
  nav_.BeginFrame(io, io.DeltaTime);
  HandleNavigation(ctx_, io.DeltaTime);

  const ImGuiViewport* vp = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(vp->WorkPos);
  ImGui::SetNextWindowSize(vp->WorkSize);
  ImGui::SetNextWindowBgAlpha(ctx_.overlay_while_playing ? 0.f : 0.f);

  ImGuiWindowFlags flags =
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground |
      ImGuiWindowFlags_NoBringToFrontOnFocus;

  if (!ImGui::Begin("NXE Dashboard", nullptr, flags)) {
    ImGui::End();
    return;
  }

  DrawBackground(ctx_, io);
  DrawScreen(ctx_, io);
  DrawChrome(ctx_, io);
  DrawProfileCardButton(ctx_, io);

  if (ctx_.current_screen == NxeScreen::kGameLibrary) {
    ImGui::SetCursorPos(ImVec2(48.f, 80.f));
    ImGui::SetNextItemWidth(240.f);
    if (ImGui::InputTextWithHint("##search", "Search...", ctx_.search_buffer,
                                 sizeof(ctx_.search_buffer))) {
      RefreshFilteredEntries(ctx_);
    }
  }

  ImGui::End();

  if (ctx_.current_screen == NxeScreen::kProfileSwitch) {
    DrawProfilePanel(ctx_, io);
  } else if (ctx_.current_screen == NxeScreen::kDashboard) {
    DrawNoProfileBanner(ctx_, io);
  }
}

}  // namespace nxe
}  // namespace app
}  // namespace xe
