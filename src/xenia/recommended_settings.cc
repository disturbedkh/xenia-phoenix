/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/recommended_settings.h"

#include "third_party/fmt/include/fmt/format.h"
#include "xenia/base/cvar.h"
#include "xenia/base/filesystem.h"
#include "xenia/base/logging.h"
#include "xenia/config.h"

namespace config {
namespace {

toml::table g_recommended_table;
bool g_recommended_loaded = false;

void EnsureRecommendedTableLoaded() {
  if (g_recommended_loaded) {
    return;
  }
  LoadRecommendedSettingsTable();
}

const toml::table* GetTitleSection(std::string_view title_id) {
  EnsureRecommendedTableLoaded();
  const auto* node = g_recommended_table.get(title_id);
  if (!node) {
    return nullptr;
  }
  return node->as_table();
}

const toml::node* GetTitleCvarNode(std::string_view title_id,
                                   const char* cvar_name) {
  const toml::table* section = GetTitleSection(title_id);
  if (!section) {
    return nullptr;
  }
  return section->get(cvar_name);
}

void ApplyCvarFromNode(cvar::IConfigVar* config_var, const toml::node* node) {
  if (!node || !config_var) {
    return;
  }
  config_var->LoadConfigValue(node);
}

void ApplyTitleSection(const toml::table& section) {
  if (!cvar::ConfigVars) {
    return;
  }
  for (auto& [key, value] : section) {
    const std::string key_str(key);
    if (key_str == "title_name") {
      continue;
    }
    auto it = cvar::ConfigVars->find(key_str);
    if (it == cvar::ConfigVars->end() || !it->second) {
      continue;
    }
    ApplyCvarFromNode(it->second, &value);
  }
}

bool GameConfigHasKey(uint32_t title_id, const char* section,
                      const char* cvar_name) {
  const toml::table table = LoadGameConfig(title_id);
  const auto* section_node = table.get(section);
  if (!section_node) {
    return false;
  }
  const auto* section_table = section_node->as_table();
  if (!section_table) {
    return false;
  }
  return section_table->get(cvar_name) != nullptr;
}

}  // namespace

void LoadRecommendedSettingsTable() {
  g_recommended_loaded = true;
  g_recommended_table = toml::table{};

  const auto path = GetRecommendedSettingsPath();
  if (!std::filesystem::exists(path)) {
    XELOGW("Recommended settings table not found: {}", path);
    return;
  }

  try {
    g_recommended_table = toml::parse_file(xe::path_to_utf8(path));
    XELOGI("Loaded recommended settings: {}", path);
  } catch (const std::exception& e) {
    XELOGE("Failed to parse recommended settings {}: {}", path, e.what());
    g_recommended_table = toml::table{};
  }
}

std::filesystem::path GetRecommendedSettingsPath() {
  const auto exe_config = xe::filesystem::GetExecutableFolder() / "config" /
                          "recommended_settings.toml";
  if (std::filesystem::exists(exe_config)) {
    return exe_config;
  }
  const auto dev_config = xe::filesystem::GetExecutableFolder() / ".." / ".." /
                          ".." / "config" / "recommended_settings.toml";
  if (std::filesystem::exists(dev_config)) {
    return std::filesystem::weakly_canonical(dev_config);
  }
  if (!config_folder.empty()) {
    const auto user_copy = config_folder / "recommended_settings.toml";
    if (std::filesystem::exists(user_copy)) {
      return user_copy;
    }
  }
  return exe_config;
}

void ApplyRecommendedSettings(std::string_view title_id) {
  const toml::table* section = GetTitleSection(title_id);
  if (!section) {
    return;
  }
  XELOGI("Applying recommended settings for title {}", title_id);
  ApplyTitleSection(*section);
}

bool HasGameConfigOverride(uint32_t title_id, const char* section,
                           const char* cvar_name) {
  return GameConfigHasKey(title_id, section, cvar_name);
}

void ClearGameConfigOverride(uint32_t title_id, const char* section,
                             const char* cvar_name) {
  toml::table table = LoadGameConfig(title_id);
  auto* section_node = table.get(section);
  if (!section_node) {
    return;
  }
  auto* section_table = section_node->as_table();
  if (!section_table) {
    return;
  }
  section_table->erase(cvar_name);
  if (section_table->empty()) {
    table.erase(section);
  }
  SaveGameConfig(title_id, table);

  const std::string title_hex = fmt::format("{:08X}", title_id);
  ApplyRecommendedSettings(title_hex);
  ReadGameConfigFile(title_hex);
}

bool IsUsingRecommendedMode(uint32_t title_id, const char* section,
                            const char* cvar_name) {
  if (!HasGameConfigOverride(title_id, section, cvar_name)) {
    return true;
  }
  const toml::table table = LoadGameConfig(title_id);
  const auto* section_node = table.get(section);
  if (!section_node) {
    return true;
  }
  const auto* section_table = section_node->as_table();
  if (!section_table) {
    return true;
  }
  const auto* value = section_table->get(cvar_name);
  if (!value) {
    return true;
  }
  if (auto* str = value->as_string()) {
    return str->get() == kRecommendedMode;
  }
  return false;
}

std::optional<std::string> LookupRecommendedString(std::string_view title_id,
                                                   const char* cvar_name) {
  const toml::node* node = GetTitleCvarNode(title_id, cvar_name);
  if (!node) {
    return std::nullopt;
  }
  if (auto* str = node->as_string()) {
    return str->get();
  }
  if (auto* boolean = node->as_boolean()) {
    return boolean->get() ? "true" : "false";
  }
  if (auto* integer = node->as_integer()) {
    return std::to_string(integer->get());
  }
  if (auto* floating = node->as_floating_point()) {
    return fmt::format("{}", floating->get());
  }
  return std::nullopt;
}

std::string ResolveRecommendedDisplayValue(uint32_t title_id,
                                           const char* cvar_name,
                                           const std::string& global_value) {
  const std::string title_hex = fmt::format("{:08X}", title_id);
  if (auto recommended = LookupRecommendedString(title_hex, cvar_name)) {
    return *recommended;
  }
  return global_value;
}

}  // namespace config
