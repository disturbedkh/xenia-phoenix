/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/app/cvar_settings_ui.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <map>

#include "third_party/fmt/include/fmt/format.h"
#include "third_party/imgui/imgui.h"
#include "third_party/tomlplusplus/toml.hpp"
#include "xenia/app/settings_ui_registry.h"
#include "xenia/base/cvar.h"
#include "xenia/base/utf8.h"
#include "xenia/config.h"
#include "xenia/emulator.h"
#include "xenia/recommended_settings.h"
#include "xenia/ui/file_picker.h"

DECLARE_string(gpu);
DECLARE_string(readback_resolve);
DECLARE_string(apu);
DECLARE_string(hid);

namespace xe {
namespace app {
namespace {

template <typename T>
void OverrideConfigCvar(const char* name, const T& value) {
  if (!::cvar::ConfigVars) {
    return;
  }
  auto it = ::cvar::ConfigVars->find(name);
  if (it == ::cvar::ConfigVars->end() || !it->second) {
    return;
  }
  if (auto* cv = dynamic_cast<::cvar::ConfigVar<T>*>(it->second)) {
    cv->OverrideConfigValue(value);
  }
}

bool CategoryInList(std::string_view category,
                    const std::vector<std::string_view>& categories) {
  for (const auto& c : categories) {
    if (c == category) {
      return true;
    }
  }
  return false;
}

}  // namespace

bool CvarSettingsUi::DrawStringComboWithRecommended(
    const char* label, std::string& value, const char* const* options,
    int option_count, const char* category, const char* cvar_name,
    Emulator* emulator, bool per_game_mode) {
  const uint32_t title_id =
      emulator && emulator->is_title_open() ? emulator->title_id() : 0;
  const bool can_recommend = title_id != 0;
  const bool using_recommended =
      can_recommend &&
      config::IsUsingRecommendedMode(title_id, category, cvar_name);
  const std::string resolved =
      can_recommend
          ? config::ResolveRecommendedDisplayValue(title_id, cvar_name, value)
          : value;

  int index = using_recommended ? 0 : (can_recommend ? 1 : 0);
  if (!using_recommended) {
    for (int i = 0; i < option_count; ++i) {
      if (value == options[i]) {
        index = can_recommend ? i + 1 : i;
        break;
      }
    }
  }

  std::string preview = using_recommended
                            ? fmt::format("Recommended ({})", resolved)
                            : (index > 0 || !can_recommend ? value : resolved);

  bool changed = false;
  if (ImGui::BeginCombo(label, preview.c_str())) {
    if (can_recommend) {
      const bool selected = using_recommended;
      if (ImGui::Selectable(fmt::format("Recommended ({})", resolved).c_str(),
                            selected)) {
        index = 0;
        changed = true;
      }
      if (selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    for (int i = 0; i < option_count; ++i) {
      const bool selected = (!using_recommended && value == options[i]);
      if (ImGui::Selectable(options[i], selected)) {
        index = can_recommend ? i + 1 : i;
        value = options[i];
        changed = true;
      }
      if (selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }

  if (changed) {
    if (can_recommend && index == 0) {
      config::ClearGameConfigOverride(title_id, category, cvar_name);
      if (auto recommended = config::LookupRecommendedString(
              fmt::format("{:08X}", title_id), cvar_name)) {
        value = *recommended;
      }
    } else {
      CvarSettingsUi::ApplyCvarChange(category, cvar_name, value, emulator,
                                      per_game_mode);
    }
  }
  return changed;
}

namespace {

void DrawRestartBadge(std::string_view name) {
  if (RequiresRestart(name)) {
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.f, 0.75f, 0.35f, 1.f), "(restart)");
  }
}

void DrawCvarHelpMarkerImpl(const ::cvar::IConfigVar& config_var) {
  const std::string& desc = config_var.description();
  if (desc.empty()) {
    return;
  }
  ImGui::SameLine();
  ImGui::TextDisabled("?");
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
    ImGui::BeginTooltip();
    const float wrap = ImGui::GetFontSize() * 35.0f;
    ImGui::PushTextWrapPos(wrap);
    ImGui::TextUnformatted(desc.c_str());
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }
}

bool TryDrawCustomCvarWidget(::cvar::IConfigVar* config_var, Emulator* emulator,
                             bool per_game_mode) {
  const std::string& name = config_var->name();
  if (name == "gpu") {
    const char* gpus[] = {"d3d12", "vulkan", "null"};
    CvarSettingsUi::DrawStringComboWithRecommended(
        "gpu", cvars::gpu, gpus, 3, "GPU", "gpu", emulator, per_game_mode);
    DrawCvarHelpMarkerImpl(*config_var);
    DrawRestartBadge(config_var->name());
    return true;
  }
  if (name == "readback_resolve") {
    const char* modes[] = {"fast", "none", "some", "full"};
    CvarSettingsUi::DrawStringComboWithRecommended(
        "readback_resolve", cvars::readback_resolve, modes, 4, "GPU",
        "readback_resolve", emulator, per_game_mode);
    DrawCvarHelpMarkerImpl(*config_var);
    DrawRestartBadge(config_var->name());
    return true;
  }
  if (name == "apu") {
    const char* backends[] = {"any", "nop", "sdl", "xaudio2", "alsa"};
    CvarSettingsUi::DrawStringComboWithRecommended(
        "apu", cvars::apu, backends, 5, "APU", "apu", emulator, per_game_mode);
    DrawCvarHelpMarkerImpl(*config_var);
    DrawRestartBadge(config_var->name());
    return true;
  }
  if (name == "hid") {
    const char* drivers[] = {"any", "nop", "sdl", "xinput", "winkey"};
    CvarSettingsUi::DrawStringComboWithRecommended(
        "hid", cvars::hid, drivers, 5, "HID", "hid", emulator, per_game_mode);
    DrawCvarHelpMarkerImpl(*config_var);
    DrawRestartBadge(config_var->name());
    return true;
  }
  return false;
}

bool DrawBoolCvar(::cvar::IConfigVar* config_var, Emulator* emulator,
                  bool per_game_mode) {
  auto* cv = dynamic_cast<::cvar::ConfigVar<bool>*>(config_var);
  if (!cv) {
    return false;
  }
  bool value = *cv->current_value();
  if (ImGui::Checkbox(config_var->name().c_str(), &value)) {
    CvarSettingsUi::ApplyCvarChange(config_var->category().c_str(),
                                    config_var->name().c_str(), value, emulator,
                                    per_game_mode);
  }
  DrawCvarHelpMarkerImpl(*config_var);
  DrawRestartBadge(config_var->name());
  return true;
}

bool DrawInt32Cvar(::cvar::IConfigVar* config_var, Emulator* emulator,
                   bool per_game_mode) {
  auto* cv = dynamic_cast<::cvar::ConfigVar<int32_t>*>(config_var);
  if (!cv) {
    return false;
  }
  int value = *cv->current_value();
  if (ImGui::InputInt(config_var->name().c_str(), &value)) {
    CvarSettingsUi::ApplyCvarChange(
        config_var->category().c_str(), config_var->name().c_str(),
        static_cast<int32_t>(value), emulator, per_game_mode);
  }
  DrawCvarHelpMarkerImpl(*config_var);
  DrawRestartBadge(config_var->name());
  return true;
}

bool DrawUInt32Cvar(::cvar::IConfigVar* config_var, Emulator* emulator,
                    bool per_game_mode) {
  auto* cv = dynamic_cast<::cvar::ConfigVar<uint32_t>*>(config_var);
  if (!cv) {
    return false;
  }
  int value = static_cast<int>(*cv->current_value());
  if (ImGui::InputInt(config_var->name().c_str(), &value, 0, 0)) {
    if (value < 0) {
      value = 0;
    }
    CvarSettingsUi::ApplyCvarChange(
        config_var->category().c_str(), config_var->name().c_str(),
        static_cast<uint32_t>(value), emulator, per_game_mode);
  }
  DrawCvarHelpMarkerImpl(*config_var);
  DrawRestartBadge(config_var->name());
  return true;
}

bool DrawInt64Cvar(::cvar::IConfigVar* config_var, Emulator* emulator,
                   bool per_game_mode) {
  auto* cv = dynamic_cast<::cvar::ConfigVar<int64_t>*>(config_var);
  if (!cv) {
    return false;
  }
  int64_t value = *cv->current_value();
  if (ImGui::InputScalar(config_var->name().c_str(), ImGuiDataType_S64,
                         &value)) {
    CvarSettingsUi::ApplyCvarChange(config_var->category().c_str(),
                                    config_var->name().c_str(), value, emulator,
                                    per_game_mode);
  }
  DrawCvarHelpMarkerImpl(*config_var);
  DrawRestartBadge(config_var->name());
  return true;
}

bool DrawUInt64Cvar(::cvar::IConfigVar* config_var, Emulator* emulator,
                    bool per_game_mode) {
  auto* cv = dynamic_cast<::cvar::ConfigVar<uint64_t>*>(config_var);
  if (!cv) {
    return false;
  }
  uint64_t value = *cv->current_value();
  if (ImGui::InputScalar(config_var->name().c_str(), ImGuiDataType_U64,
                         &value)) {
    CvarSettingsUi::ApplyCvarChange(config_var->category().c_str(),
                                    config_var->name().c_str(), value, emulator,
                                    per_game_mode);
  }
  DrawCvarHelpMarkerImpl(*config_var);
  DrawRestartBadge(config_var->name());
  return true;
}

bool DrawDoubleCvar(::cvar::IConfigVar* config_var, Emulator* emulator,
                    bool per_game_mode) {
  auto* cv = dynamic_cast<::cvar::ConfigVar<double>*>(config_var);
  if (!cv) {
    return false;
  }
  float value = static_cast<float>(*cv->current_value());
  if (ImGui::InputFloat(config_var->name().c_str(), &value)) {
    CvarSettingsUi::ApplyCvarChange(
        config_var->category().c_str(), config_var->name().c_str(),
        static_cast<double>(value), emulator, per_game_mode);
  }
  DrawCvarHelpMarkerImpl(*config_var);
  DrawRestartBadge(config_var->name());
  return true;
}

bool DrawStringCvar(::cvar::IConfigVar* config_var, Emulator* emulator,
                    bool per_game_mode) {
  auto* cv = dynamic_cast<::cvar::ConfigVar<std::string>*>(config_var);
  if (!cv) {
    return false;
  }
  std::string value = *cv->current_value();
  char buffer[512];
  size_t copy_len = std::min(value.size(), sizeof(buffer) - 1);
  memcpy(buffer, value.c_str(), copy_len);
  buffer[copy_len] = '\0';
  if (ImGui::InputText(config_var->name().c_str(), buffer, sizeof(buffer))) {
    CvarSettingsUi::ApplyCvarChange(
        config_var->category().c_str(), config_var->name().c_str(),
        std::string(buffer), emulator, per_game_mode);
  }
  DrawCvarHelpMarkerImpl(*config_var);
  DrawRestartBadge(config_var->name());
  return true;
}

bool DrawPathCvar(::cvar::IConfigVar* config_var, Emulator* emulator,
                  bool per_game_mode) {
  auto* cv =
      dynamic_cast<::cvar::ConfigVar<std::filesystem::path>*>(config_var);
  if (!cv) {
    return false;
  }
  const std::string path_utf8 = xe::path_to_utf8(*cv->current_value());
  ImGui::TextUnformatted(config_var->name().c_str());
  DrawCvarHelpMarkerImpl(*config_var);
  ImGui::SameLine();
  ImGui::TextWrapped("%s", path_utf8.c_str());
  ImGui::SameLine();
  const std::string browse_id = fmt::format("Browse##{}", config_var->name());
  if (ImGui::SmallButton(browse_id.c_str())) {
    auto file_picker = ui::FilePicker::Create();
    file_picker->set_mode(ui::FilePicker::Mode::kOpen);
    if (config_var->name().find("root") != std::string::npos ||
        config_var->name().find("folder") != std::string::npos ||
        config_var->name().find("directory") != std::string::npos) {
      file_picker->set_type(ui::FilePicker::Type::kDirectory);
    } else {
      file_picker->set_type(ui::FilePicker::Type::kFile);
    }
    if (file_picker->Show() && !file_picker->selected_files().empty()) {
      const std::filesystem::path picked =
          file_picker->selected_files().front();
      OverrideConfigCvar<std::filesystem::path>(config_var->name().c_str(),
                                                picked);
      if (per_game_mode && emulator && emulator->is_title_open()) {
        config::SaveGameConfigSetting(emulator, config_var->category().c_str(),
                                      config_var->name().c_str(),
                                      xe::path_to_utf8(picked));
      } else {
        config::SaveConfig();
      }
    }
  }
  DrawRestartBadge(config_var->name());
  return true;
}

bool DrawCvarWidget(::cvar::IConfigVar* config_var, Emulator* emulator,
                    bool per_game_mode) {
  if (TryDrawCustomCvarWidget(config_var, emulator, per_game_mode)) {
    return true;
  }
  if (DrawBoolCvar(config_var, emulator, per_game_mode)) {
    return true;
  }
  if (DrawInt32Cvar(config_var, emulator, per_game_mode)) {
    return true;
  }
  if (DrawUInt32Cvar(config_var, emulator, per_game_mode)) {
    return true;
  }
  if (DrawInt64Cvar(config_var, emulator, per_game_mode)) {
    return true;
  }
  if (DrawUInt64Cvar(config_var, emulator, per_game_mode)) {
    return true;
  }
  if (DrawDoubleCvar(config_var, emulator, per_game_mode)) {
    return true;
  }
  if (DrawStringCvar(config_var, emulator, per_game_mode)) {
    return true;
  }
  if (DrawPathCvar(config_var, emulator, per_game_mode)) {
    return true;
  }
  ImGui::TextDisabled("%s (unsupported type)", config_var->name().c_str());
  return false;
}

}  // namespace

void CvarSettingsUi::DrawCvarHelpMarker(const char* cvar_name) {
  if (!::cvar::ConfigVars || !cvar_name) {
    return;
  }
  auto it = ::cvar::ConfigVars->find(cvar_name);
  if (it == ::cvar::ConfigVars->end() || !it->second) {
    return;
  }
  DrawCvarHelpMarkerImpl(*it->second);
}

void CvarSettingsUi::ApplyCvarChange(const char* category,
                                     const char* cvar_name,
                                     const std::string& value,
                                     Emulator* emulator, bool per_game_mode) {
  OverrideConfigCvar(cvar_name, value);
  if (per_game_mode && emulator && emulator->is_title_open()) {
    config::SaveGameConfigSetting(emulator, category, cvar_name, value);
  } else {
    config::SaveConfig();
  }
}

void CvarSettingsUi::ApplyCvarChange(const char* category,
                                     const char* cvar_name, bool value,
                                     Emulator* emulator, bool per_game_mode) {
  OverrideConfigCvar(cvar_name, value);
  if (per_game_mode && emulator && emulator->is_title_open()) {
    config::SaveGameConfigSetting(emulator, category, cvar_name, value);
  } else {
    config::SaveConfig();
  }
}

void CvarSettingsUi::ApplyCvarChange(const char* category,
                                     const char* cvar_name, int32_t value,
                                     Emulator* emulator, bool per_game_mode) {
  OverrideConfigCvar(cvar_name, value);
  if (per_game_mode && emulator && emulator->is_title_open()) {
    config::SaveGameConfigSetting(emulator, category, cvar_name, value);
  } else {
    config::SaveConfig();
  }
}

void CvarSettingsUi::ApplyCvarChange(const char* category,
                                     const char* cvar_name, uint32_t value,
                                     Emulator* emulator, bool per_game_mode) {
  OverrideConfigCvar(cvar_name, value);
  if (per_game_mode && emulator && emulator->is_title_open()) {
    config::SaveGameConfigSetting(emulator, category, cvar_name, value);
  } else {
    config::SaveConfig();
  }
}

void CvarSettingsUi::ApplyCvarChange(const char* category,
                                     const char* cvar_name, uint64_t value,
                                     Emulator* emulator, bool per_game_mode) {
  OverrideConfigCvar(cvar_name, value);
  if (per_game_mode && emulator && emulator->is_title_open()) {
    config::SaveGameConfigSetting(emulator, category, cvar_name,
                                  std::to_string(value));
  } else {
    config::SaveConfig();
  }
}

void CvarSettingsUi::ApplyCvarChange(const char* category,
                                     const char* cvar_name, int64_t value,
                                     Emulator* emulator, bool per_game_mode) {
  OverrideConfigCvar(cvar_name, value);
  if (per_game_mode && emulator && emulator->is_title_open()) {
    config::SaveGameConfigSetting(emulator, category, cvar_name,
                                  std::to_string(value));
  } else {
    config::SaveConfig();
  }
}

void CvarSettingsUi::ApplyCvarChange(const char* category,
                                     const char* cvar_name, double value,
                                     Emulator* emulator, bool per_game_mode) {
  OverrideConfigCvar(cvar_name, value);
  if (per_game_mode && emulator && emulator->is_title_open()) {
    config::SaveGameConfigSetting(emulator, category, cvar_name, value);
  } else {
    config::SaveConfig();
  }
}

bool CvarSettingsUi::MatchesSearch(const ::cvar::IConfigVar& config_var,
                                   std::string_view filter) {
  if (filter.empty()) {
    return true;
  }
  std::string lower_filter(filter);
  std::transform(lower_filter.begin(), lower_filter.end(), lower_filter.begin(),
                 [](unsigned char c) { return static_cast<char>(tolower(c)); });
  auto contains = [&](std::string_view haystack) {
    std::string lower_hay(haystack);
    std::transform(
        lower_hay.begin(), lower_hay.end(), lower_hay.begin(),
        [](unsigned char c) { return static_cast<char>(tolower(c)); });
    return lower_hay.find(lower_filter) != std::string::npos;
  };
  return contains(config_var.name()) || contains(config_var.description()) ||
         contains(config_var.category());
}

void CvarSettingsUi::DrawCategories(
    const std::vector<std::string_view>& categories, CvarSettingsUiState& state,
    Emulator* emulator, bool per_game_mode, bool include_curated,
    bool gpu_advanced_only) {
  if (!::cvar::ConfigVars) {
    return;
  }

  ImGui::InputTextWithHint("Search", "Filter by name or description",
                           state.search_buffer, sizeof(state.search_buffer));
  ImGui::Checkbox("Show advanced options", &state.show_advanced);
  ImGui::Separator();

  std::vector<::cvar::IConfigVar*> vars;
  vars.reserve(::cvar::ConfigVars->size());
  for (const auto& entry : *::cvar::ConfigVars) {
    vars.push_back(entry.second);
  }
  std::sort(vars.begin(), vars.end(),
            [](::cvar::IConfigVar* a, ::cvar::IConfigVar* b) {
              if (a->category() != b->category()) {
                return a->category() < b->category();
              }
              return a->name() < b->name();
            });

  const std::string_view filter = state.search_buffer;
  std::map<std::string, std::vector<::cvar::IConfigVar*>> by_category;
  for (::cvar::IConfigVar* config_var : vars) {
    if (config_var->is_transient()) {
      continue;
    }
    if (!CategoryInList(config_var->category(), categories)) {
      continue;
    }
    if (gpu_advanced_only && config_var->category() == "GPU" &&
        !IsAdvancedOnlyCvar(config_var->name())) {
      continue;
    }
    if (!include_curated && IsCuratedCvar(config_var->name())) {
      continue;
    }
    if (!state.show_advanced && IsAdvancedOnlyCvar(config_var->name())) {
      continue;
    }
    if (!MatchesSearch(*config_var, filter)) {
      continue;
    }
    by_category[config_var->category()].push_back(config_var);
  }

  for (const auto& [category, category_vars] : by_category) {
    if (ImGui::CollapsingHeader(category.c_str(),
                                ImGuiTreeNodeFlags_DefaultOpen)) {
      for (::cvar::IConfigVar* config_var : category_vars) {
        DrawCvarWidget(config_var, emulator, per_game_mode);
      }
    }
  }
}

void CvarSettingsUi::DrawGameOverridesPanel(Emulator* emulator) {
  if (!emulator || !emulator->is_title_open()) {
    ImGui::TextWrapped(
        "Launch a game to view and edit per-title overrides stored in "
        "config/<TitleID>.config.toml.");
    return;
  }

  const uint32_t title_id = emulator->title_id();
  const std::string title_hex = fmt::format("{:08X}", title_id);
  const auto game_path = config::GetGameConfigPath(title_hex);
  ImGui::TextWrapped("Title ID: %s", title_hex.c_str());
  ImGui::TextWrapped("File: %s", xe::path_to_utf8(game_path).c_str());

  toml::table table = config::LoadGameConfig(title_id);
  if (table.empty()) {
    ImGui::TextWrapped("No per-game overrides saved yet.");
    return;
  }

  for (auto& [section_key, section_node] : table) {
    auto* section_table = section_node.as_table();
    if (!section_table) {
      continue;
    }
    const std::string section_name = std::string(section_key);
    if (ImGui::TreeNode(section_name.c_str())) {
      for (auto& [key, value] : *section_table) {
        std::string value_str;
        if (auto* v = value.as_string()) {
          value_str = v->get();
        } else if (auto* v = value.as_boolean()) {
          value_str = v->get() ? "true" : "false";
        } else if (auto* v = value.as_integer()) {
          value_str = std::to_string(v->get());
        } else if (auto* v = value.as_floating_point()) {
          value_str = std::to_string(v->get());
        } else {
          value_str = value.type() == toml::node_type::none ? "" : "?";
        }
        ImGui::BulletText("%s = %s", std::string(key).c_str(),
                          value_str.c_str());
        ImGui::SameLine();
        const std::string reset_id =
            fmt::format("Reset##{}{}", section_name, std::string(key));
        if (ImGui::SmallButton(reset_id.c_str())) {
          section_table->erase(key);
          config::SaveGameConfig(title_id, table);
          config::LoadGameConfig(title_hex);
        }
      }
      ImGui::TreePop();
    }
  }
}

}  // namespace app
}  // namespace xe
