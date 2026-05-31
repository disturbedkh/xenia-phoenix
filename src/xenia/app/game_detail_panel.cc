/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/app/game_detail_panel.h"

#include "third_party/fmt/include/fmt/format.h"
#include "third_party/imgui/imgui.h"
#include "xenia/app/emulator_window.h"

namespace xe {
namespace app {

GameDetailPanel::GameDetailPanel(EmulatorWindow& window,
                                 library::GameLibrary& library,
                                 GameMetadataService& metadata,
                                 CoverArtFetcher& covers, PatchCatalog& patches,
                                 ContentManager& content)
    : window_(window),
      library_(library),
      metadata_(metadata),
      covers_(covers),
      patches_(patches),
      content_(content) {}

void GameDetailPanel::SetEntry(const library::LibraryEntry* entry) {
  entry_ = entry;
  if (entry_ && entry_->title_id) {
    metadata_.PrefetchAsync(entry_->title_id);
    if (!covers_.HasCachedCover(entry_->title_id)) {
      covers_.FetchCoverAsync(entry_->title_id, entry_->title_name, nullptr);
    }
  }
}

void GameDetailPanel::SetOnLaunch(
    std::function<void(const std::filesystem::path&)> cb) {
  on_launch_ = std::move(cb);
}

void GameDetailPanel::SetOnConfigure(std::function<void(uint32_t)> cb) {
  on_configure_ = std::move(cb);
}

void GameDetailPanel::Draw(float width) {
  ImGui::BeginChild("game_detail", ImVec2(width, 0), true);
  if (!entry_) {
    ImGui::TextWrapped("Select a game to see details.");
    ImGui::EndChild();
    return;
  }

  ImGui::TextWrapped("%s", entry_->title_name.c_str());
  ImGui::TextDisabled("Title ID: %s", entry_->title_id_hex.c_str());
  if (!entry_->media_type.empty()) {
    ImGui::TextDisabled("Type: %s", entry_->media_type.c_str());
  }
  if (entry_->play_seconds > 0) {
    ImGui::Text("Play time: %llu min",
                static_cast<unsigned long long>(entry_->play_seconds / 60));
  }

  if (auto meta = metadata_.Lookup(entry_->title_id)) {
    if (!meta->developer.empty()) {
      ImGui::TextDisabled("Developer: %s", meta->developer.c_str());
    }
    if (!meta->publisher.empty()) {
      ImGui::TextDisabled("Publisher: %s", meta->publisher.c_str());
    }
    if (!meta->genre.empty()) {
      ImGui::TextDisabled("Genre: %s", meta->genre.c_str());
    }
  }

  ImGui::Separator();

  const auto patch_list = patches_.PatchesForTitle(entry_->title_id);
  ImGui::Text("Patches: %zu", patch_list.size());
  for (const auto& patch : patch_list) {
    ImGui::BulletText("%s %s", patch.enabled ? "[on]" : "[off]",
                      patch.name.c_str());
  }

  const auto packages = content_.PackagesForTitle(entry_->title_id);
  ImGui::Text("Content packages: %zu", packages.size());

  ImGui::Separator();
  if (ImGui::Button("Play", ImVec2(-1, 0)) && on_launch_) {
    on_launch_(entry_->path);
  }
  if (ImGui::Button("Configure", ImVec2(-1, 0)) && on_configure_ &&
      entry_->title_id) {
    on_configure_(entry_->title_id);
  }
  ImGui::EndChild();
}

}  // namespace app
}  // namespace xe
