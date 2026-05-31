/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_APP_SETTINGS_UI_REGISTRY_H_
#define XENIA_APP_SETTINGS_UI_REGISTRY_H_

#include <string_view>
#include <vector>

namespace xe {
class Emulator;
namespace app {

enum class PreferencesTab {
  kGraphics,
  kVideoDisplay,
  kAudio,
  kInput,
  kStorage,
  kGeneral,
  kCpuSystem,
  kLogging,
  kAdvanced,
  kGameOverrides,
  kXboxConsole,
  kCount,
};

const char* PreferencesTabLabel(PreferencesTab tab);

// TOML categories shown on each tab (excluding curated-only keys).
std::vector<std::string_view> CategoriesForTab(PreferencesTab tab);

bool IsCuratedCvar(std::string_view name);
bool IsAdvancedOnlyCvar(std::string_view name);
bool RequiresRestart(std::string_view name);

}  // namespace app
}  // namespace xe

#endif  // XENIA_APP_SETTINGS_UI_REGISTRY_H_
