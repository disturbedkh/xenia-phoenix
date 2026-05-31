/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_RECOMMENDED_SETTINGS_H_
#define XENIA_RECOMMENDED_SETTINGS_H_

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace config {

// Sentinel stored in per-game config when user selects "Recommended".
inline constexpr const char* kRecommendedMode = "recommended";

// Load config/recommended_settings.toml (cached). Safe to call multiple times.
void LoadRecommendedSettingsTable();

std::filesystem::path GetRecommendedSettingsPath();

// Apply title-specific recommended cvars before per-game user overrides load.
void ApplyRecommendedSettings(std::string_view title_id);

// True when config/<title_id>.config.toml explicitly sets this cvar.
bool HasGameConfigOverride(uint32_t title_id, const char* section,
                           const char* cvar_name);

// Remove override and re-apply recommended + remaining game config.
void ClearGameConfigOverride(uint32_t title_id, const char* section,
                             const char* cvar_name);

// True when no explicit per-game override exists for this cvar.
bool IsUsingRecommendedMode(uint32_t title_id, const char* section,
                            const char* cvar_name);

// Resolved value for UI preview: table value or global default.
std::string ResolveRecommendedDisplayValue(uint32_t title_id,
                                           const char* cvar_name,
                                           const std::string& global_value);

std::optional<std::string> LookupRecommendedString(std::string_view title_id,
                                                   const char* cvar_name);

}  // namespace config

#endif  // XENIA_RECOMMENDED_SETTINGS_H_
