/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2020 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_CONFIG_H_
#define XENIA_CONFIG_H_

#include <filesystem>
#include <functional>
#include <string_view>
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wabsolute-value"
#endif
#include "third_party/tomlplusplus/toml.hpp"
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

toml::parse_result ParseFile(const std::filesystem::path& filename);

namespace xe {
class Emulator;
}  // namespace xe

namespace config {
extern std::filesystem::path config_folder;
extern std::string game_config_suffix;

void SetupConfig(const std::filesystem::path& config_folder);
void LoadGameConfig(const std::string_view title_id);
toml::table LoadGameConfig(uint32_t title_id);
std::filesystem::path GetGameConfigPath(const std::string& title_id);
void SaveConfig();
void SaveGameConfig(uint32_t title_id, const toml::table& config_table);
void SaveGameConfigSetting(xe::Emulator* emulator, const char* section,
                           const char* cvar_name, const std::string& value);
void SaveGameConfigSetting(xe::Emulator* emulator, const char* section,
                           const char* cvar_name, bool value);
void SaveGameConfigSetting(xe::Emulator* emulator, const char* section,
                           const char* cvar_name, int32_t value);
void SaveGameConfigSetting(xe::Emulator* emulator, const char* section,
                           const char* cvar_name, uint32_t value);
void SaveGameConfigSetting(xe::Emulator* emulator, const char* section,
                           const char* cvar_name, double value);
}  // namespace config

#endif  // XENIA_CONFIG_H_
