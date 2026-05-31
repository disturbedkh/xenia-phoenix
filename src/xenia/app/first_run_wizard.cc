/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/app/first_run_wizard.h"

#include <fstream>

#include "third_party/imgui/imgui.h"
#include "third_party/tomlplusplus/toml.hpp"
#include "xenia/app/emulator_window.h"
#include "xenia/app/library/game_library.h"
#include "xenia/emulator.h"
#include "xenia/kernel/xam/ui/create_profile_ui.h"
#include "xenia/ui/file_picker.h"

namespace xe {
namespace app {
namespace {

constexpr const char* kWizardDoneFile = "cache/first_run_complete.toml";

}  // namespace

FirstRunWizardDialog::FirstRunWizardDialog(ui::ImGuiDrawer* imgui_drawer,
                                           EmulatorWindow& window)
    : ui::ImGuiDialog(imgui_drawer), window_(window) {}

bool FirstRunWizardDialog::IsNeeded(const std::filesystem::path& storage_root) {
  std::error_code ec;
  const auto marker = storage_root / kWizardDoneFile;
  if (std::filesystem::exists(marker, ec)) {
    return false;
  }
  const auto library = storage_root / "library.toml";
  return !std::filesystem::exists(library, ec);
}

void FirstRunWizardDialog::OnDraw(ImGuiIO& io) {
  if (completed_) {
    Close();
    return;
  }

  const ImGuiViewport* vp = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(
      ImVec2(vp->WorkPos.x + vp->WorkSize.x * 0.25f, vp->WorkPos.y + 80.f));
  ImGui::SetNextWindowSize(ImVec2(vp->WorkSize.x * 0.5f, 420.f));

  if (!ImGui::Begin("Welcome to Xenia Phoenix", nullptr,
                    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize)) {
    ImGui::End();
    return;
  }

  switch (step_) {
    case 0:
      ImGui::TextWrapped(
          "Welcome! This wizard helps you set up profiles, game folders, "
          "and updates for a console-like experience.");
      if (ImGui::Button("Next")) {
        ++step_;
      }
      break;
    case 1:
      ImGui::TextWrapped("Step 1: Create a profile (required for saves).");
      if (ImGui::Button("Create profile")) {
        new kernel::xam::ui::CreateProfileUI(window_.imgui_drawer(),
                                             window_.emulator());
      }
      ImGui::SameLine();
      if (ImGui::Button("Skip for now")) {
        ++step_;
      }
      if (ImGui::Button("Next")) {
        ++step_;
      }
      break;
    case 2:
      ImGui::TextWrapped("Step 2: Add a game folder.");
      ImGui::InputText("Folder", game_folder_, sizeof(game_folder_));
      if (ImGui::Button("Browse...")) {
        auto picker = ui::FilePicker::Create();
        picker->set_mode(ui::FilePicker::Mode::kOpen);
        picker->set_type(ui::FilePicker::Type::kDirectory);
        picker->set_title("Select Game Folder");
        if (picker->Show(window_.window()) &&
            !picker->selected_files().empty()) {
          const auto path = picker->selected_files().front();
          const auto utf8 = xe::path_to_utf8(path);
          std::snprintf(game_folder_, sizeof(game_folder_), "%s", utf8.c_str());
        }
      }
      if (ImGui::Button("Back")) {
        --step_;
      }
      ImGui::SameLine();
      if (ImGui::Button("Next")) {
        ++step_;
      }
      break;
    case 3:
      ImGui::TextWrapped("Step 3: Update channel");
      ImGui::Checkbox("Stable releases (recommended)", &stable_updates_);
      window_.SetAutoCheckForUpdates(true);
      if (ImGui::Button("Back")) {
        --step_;
      }
      ImGui::SameLine();
      if (ImGui::Button("Finish")) {
        if (game_folder_[0] != '\0') {
          window_.AddGameLibraryFolder(std::filesystem::path(game_folder_));
        }
        std::error_code ec;
        const auto marker =
            window_.emulator()->storage_root() / kWizardDoneFile;
        std::filesystem::create_directories(marker.parent_path(), ec);
        toml::table root;
        root.insert("completed", true);
        root.insert("stable_updates", stable_updates_);
        if (game_folder_[0] != '\0') {
          root.insert("initial_game_folder", std::string(game_folder_));
        }
        std::ofstream out(marker);
        out << root;
        completed_ = true;
      }
      break;
    default:
      break;
  }

  ImGui::End();
}

}  // namespace app
}  // namespace xe
