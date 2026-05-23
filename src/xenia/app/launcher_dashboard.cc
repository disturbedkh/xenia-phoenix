/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/app/launcher_dashboard.h"

#include <algorithm>
#include <fstream>

#include "third_party/fmt/include/fmt/format.h"
#include "third_party/imgui/imgui.h"
#include "third_party/stb/stb_image.h"
#include "version.h"
#include "xenia/app/emulator_window.h"
#include "xenia/base/logging.h"
#include "xenia/base/string.h"
#include "xenia/emulator.h"
#include "xenia/ui/immediate_drawer.h"
#include "xenia/ui/phoenix_icons.h"

namespace xe {
namespace app {

namespace {

constexpr float kTileWidth = 140.f;
constexpr float kTileHeight = 200.f;
constexpr float kCoverSize = 128.f;

}  // namespace

LauncherDashboardDialog::LauncherDashboardDialog(
    ui::ImGuiDrawer* imgui_drawer, EmulatorWindow& emulator_window,
    library::GameLibrary& game_library)
    : ui::ImGuiDialog(imgui_drawer),
      emulator_window_(emulator_window),
      game_library_(game_library) {}

void LauncherDashboardDialog::SetOnConfigureTitle(
    std::function<void(uint32_t title_id)> on_configure_title) {
  on_configure_title_ = std::move(on_configure_title);
}

void LauncherDashboardDialog::SetOnOpenLibrarySettings(
    std::function<void()> on_library_settings) {
  on_library_settings_ = std::move(on_library_settings);
}

std::vector<const library::LibraryEntry*>
LauncherDashboardDialog::FilteredEntries() const {
  std::vector<const library::LibraryEntry*> out;
  const auto& entries = game_library_.entries();
  std::string query = xe::utf8::lower_ascii(search_buffer_);

  for (const auto& e : entries) {
    if (sidebar_tab_ == 1) {
      if (e.last_played == 0) {
        continue;
      }
    }
    if (!query.empty()) {
      std::string hay = xe::utf8::lower_ascii(e.title_name);
      if (hay.find(query) == std::string::npos) {
        continue;
      }
    }
    out.push_back(&e);
  }

  std::sort(
      out.begin(), out.end(),
      [this](const library::LibraryEntry* a, const library::LibraryEntry* b) {
        switch (sort_mode_) {
          case 1:
            return a->last_played > b->last_played;
          case 2:
            return a->title_name < b->title_name;
          default:
            return a->title_name < b->title_name;
        }
      });
  return out;
}

ui::ImmediateTexture* LauncherDashboardDialog::GetCoverTexture(
    const library::LibraryEntry& entry) {
  std::string key = entry.title_id_hex.empty() ? xe::path_to_utf8(entry.path)
                                               : entry.title_id_hex;
  auto it = cover_textures_.find(key);
  if (it != cover_textures_.end()) {
    return it->second.get();
  }
  auto* immediate = emulator_window_.immediate_drawer();
  if (entry.icon_path.empty() || !immediate) {
    return nullptr;
  }
  std::error_code ec;
  if (!std::filesystem::exists(entry.icon_path, ec)) {
    return nullptr;
  }
  int w = 0, h = 0, ch = 0;
  auto path_utf8 = xe::path_to_utf8(entry.icon_path);
  unsigned char* pixels =
      stbi_load(path_utf8.c_str(), &w, &h, &ch, STBI_rgb_alpha);
  if (!pixels) {
    return nullptr;
  }
  auto tex = immediate->CreateTexture(w, h, ui::ImmediateTextureFilter::kLinear,
                                      true, pixels);
  stbi_image_free(pixels);
  if (!tex) {
    return nullptr;
  }
  cover_textures_[key] = std::move(tex);
  return cover_textures_[key].get();
}

void LauncherDashboardDialog::DrawSidebar() {
  ImGui::BeginChild("sidebar", ImVec2(160, 0), true);
  if (ImGui::Selectable(PHX_ICON_LIBRARY "  Library", sidebar_tab_ == 0)) {
    sidebar_tab_ = 0;
  }
  if (ImGui::Selectable(PHX_ICON_RECENT "  Recent", sidebar_tab_ == 1)) {
    sidebar_tab_ = 1;
  }
  ImGui::Separator();
  if (ImGui::Button(PHX_ICON_FOLDER "  Add folder...", ImVec2(-1, 0))) {
    if (on_library_settings_) {
      on_library_settings_();
    }
  }
  if (ImGui::Button("Rescan library", ImVec2(-1, 0))) {
    game_library_.StartScan();
  }
  if (ImGui::Button(PHX_ICON_SETTINGS "  Settings", ImVec2(-1, 0))) {
    if (on_library_settings_) {
      on_library_settings_();
    }
  }
  ImGui::EndChild();
}

void LauncherDashboardDialog::DrawGrid() {
  ImGui::BeginChild("grid", ImVec2(0, -28), false);
  auto filtered = FilteredEntries();
  if (filtered.empty()) {
    ImGui::TextWrapped(
        "No games in the library yet.\nUse Library > Add folder... to scan "
        "your Xbox 360 game directories.");
    ImGui::EndChild();
    return;
  }

  float panel_w = ImGui::GetContentRegionAvail().x;
  int columns = std::max(
      1, static_cast<int>(panel_w /
                          (kTileWidth + ImGui::GetStyle().ItemSpacing.x)));

  if (ImGui::BeginTable("tiles", columns)) {
    int index = 0;
    for (const auto* entry : filtered) {
      ImGui::TableNextColumn();
      ImGui::PushID(index++);
      ImGui::BeginGroup();
      auto* tex = GetCoverTexture(*entry);
      bool launch = false;
      bool from_button = false;
      if (tex) {
        from_button = ImGui::ImageButton(entry->title_id_hex.c_str(),
                                         reinterpret_cast<ImTextureID>(tex),
                                         ImVec2(kCoverSize, kCoverSize));
        launch = from_button;
      } else {
        from_button = ImGui::Button("No cover", ImVec2(kCoverSize, kCoverSize));
        launch = from_button;
      }
      const bool from_double_click =
          ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0);
      if (from_double_click) {
        launch = true;
      }
      if (launch) {
        emulator_window_.RequestLaunchTitle(entry->path);
      }
      if (ImGui::BeginPopupContextItem("tile_ctx")) {
        if (ImGui::MenuItem(PHX_ICON_PLAY "  Play")) {
          emulator_window_.RequestLaunchTitle(entry->path);
        }
        if (entry->title_id && ImGui::MenuItem("Configure for this game...")) {
          if (on_configure_title_) {
            on_configure_title_(entry->title_id);
          }
        }
        if (ImGui::MenuItem("Remove from library")) {
          game_library_.RemoveEntry(entry->path);
        }
        ImGui::EndPopup();
      }
      ImGui::TextWrapped("%s", entry->title_name.c_str());
      if (!entry->media_type.empty()) {
        ImGui::TextDisabled("%s", entry->media_type.c_str());
      }
      ImGui::EndGroup();
      ImGui::PopID();
    }
    ImGui::EndTable();
  }
  ImGui::EndChild();
}

void LauncherDashboardDialog::DrawStatusBar() {
  auto progress = game_library_.ScanProgress();
  std::string status = fmt::format("{} titles", game_library_.entries().size());
  if (game_library_.IsScanning()) {
    status += fmt::format(" | Scanning... {} games", progress.games_found);
  }
  status += fmt::format(" | {}@{} on {}", XE_BUILD_BRANCH,
                        XE_BUILD_COMMIT_SHORT, XE_BUILD_DATE);
  ImGui::Separator();
  ImGui::TextUnformatted(status.c_str());
}

void LauncherDashboardDialog::OnDraw(ImGuiIO& io) {
  if (emulator_window_.emulator()->is_title_open()) {
    return;
  }

  const ImGuiViewport* vp = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(vp->WorkPos);
  ImGui::SetNextWindowSize(vp->WorkSize);
  ImGui::SetNextWindowBgAlpha(0.98f);

  ImGuiWindowFlags flags =
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

  if (!ImGui::Begin("Phoenix Launcher", nullptr, flags)) {
    ImGui::End();
    return;
  }

  ImGui::TextUnformatted("Xenia Phoenix");
  ImGui::SameLine(ImGui::GetWindowWidth() - 280);
  ImGui::SetNextItemWidth(220);
  ImGui::InputTextWithHint("##search", PHX_ICON_SEARCH "  Search...",
                           search_buffer_, sizeof(search_buffer_));
  ImGui::SameLine();
  ImGui::SetNextItemWidth(100);
  const char* sorts[] = {"Name", "Recent"};
  ImGui::Combo("##sort", &sort_mode_, sorts, 2);

  ImGui::Separator();

  ImGui::BeginChild("body", ImVec2(0, 0), false);
  DrawSidebar();
  ImGui::SameLine();
  ImGui::BeginGroup();
  DrawGrid();
  DrawStatusBar();
  ImGui::EndGroup();
  ImGui::EndChild();

  ImGui::End();
}

}  // namespace app
}  // namespace xe
