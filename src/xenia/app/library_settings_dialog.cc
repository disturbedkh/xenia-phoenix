/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/app/library_settings_dialog.h"

#include "third_party/fmt/include/fmt/format.h"
#include "third_party/imgui/imgui.h"
#include "xenia/app/emulator_window.h"
#include "xenia/base/filesystem.h"
#include "xenia/ui/file_picker.h"

namespace xe {
namespace app {

LibrarySettingsDialog::LibrarySettingsDialog(ui::ImGuiDrawer* imgui_drawer,
                                             EmulatorWindow& emulator_window,
                                             library::GameLibrary& game_library)
    : ui::ImGuiDialog(imgui_drawer),
      emulator_window_(emulator_window),
      game_library_(game_library) {
  SetLifetimeManagedByOwner(true);
}

void LibrarySettingsDialog::OnDraw(ImGuiIO& io) {
  const std::string window_title =
      fmt::format("Library folders###{}", GetWindowId());
  ImGui::SetNextWindowSize(ImVec2(520, 400), ImGuiCond_FirstUseEver);
  bool open = true;
  const ImGuiWindowFlags window_flags =
      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse;
  if (!ImGui::Begin(window_title.c_str(), &open, window_flags)) {
    ImGui::End();
    return;
  }

  if (ImGui::Button("Add folder...")) {
    auto file_picker = ui::FilePicker::Create();
    file_picker->set_mode(ui::FilePicker::Mode::kOpen);
    file_picker->set_type(ui::FilePicker::Type::kDirectory);
    file_picker->set_title("Select Game Library Folder");
    if (file_picker->Show(emulator_window_.window())) {
      auto selected = file_picker->selected_files();
      if (!selected.empty()) {
        game_library_.AddWatchedDirectory(selected[0]);
        game_library_.StartScan();
      }
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Rescan all")) {
    game_library_.StartScan();
  }

  ImGui::Separator();
  ImGui::Text("Watched directories:");
  int remove_index = -1;
  const auto& dirs = game_library_.watched_directories();
  for (int i = 0; i < static_cast<int>(dirs.size()); ++i) {
    ImGui::BulletText("%s", xe::path_to_utf8(dirs[i]).c_str());
    ImGui::SameLine();
    ImGui::PushID(i);
    if (ImGui::SmallButton("Remove")) {
      remove_index = i;
    }
    ImGui::PopID();
  }
  if (remove_index >= 0) {
    game_library_.RemoveWatchedDirectory(dirs[remove_index]);
  }

  if (game_library_.IsScanning()) {
    auto p = game_library_.ScanProgress();
    ImGui::Text("Scanning: %zu games found...",
                static_cast<size_t>(p.games_found));
  }

  ImGui::End();
  if (!open) {
    emulator_window_.ScheduleCloseLibrarySettingsDialog();
    return;
  }
}

}  // namespace app
}  // namespace xe
