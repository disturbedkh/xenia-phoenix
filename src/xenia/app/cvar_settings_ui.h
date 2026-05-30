/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_APP_CVAR_SETTINGS_UI_H_
#define XENIA_APP_CVAR_SETTINGS_UI_H_

#include <string>
#include <string_view>
#include <vector>

#include "xenia/base/cvar.h"

namespace xe {
class Emulator;
namespace app {

struct CvarSettingsUiState {
  char search_buffer[128] = {};
  bool show_advanced = false;
};

class CvarSettingsUi {
 public:
  static void ApplyCvarChange(const char* category, const char* cvar_name,
                              const std::string& value, Emulator* emulator,
                              bool per_game_mode);
  static void ApplyCvarChange(const char* category, const char* cvar_name,
                              bool value, Emulator* emulator,
                              bool per_game_mode);
  static void ApplyCvarChange(const char* category, const char* cvar_name,
                              int32_t value, Emulator* emulator,
                              bool per_game_mode);
  static void ApplyCvarChange(const char* category, const char* cvar_name,
                              uint32_t value, Emulator* emulator,
                              bool per_game_mode);
  static void ApplyCvarChange(const char* category, const char* cvar_name,
                              uint64_t value, Emulator* emulator,
                              bool per_game_mode);
  static void ApplyCvarChange(const char* category, const char* cvar_name,
                              int64_t value, Emulator* emulator,
                              bool per_game_mode);
  static void ApplyCvarChange(const char* category, const char* cvar_name,
                              double value, Emulator* emulator,
                              bool per_game_mode);

  // Draw cvars whose category is in `categories`. Skips curated keys unless
  // `include_curated`.
  static void DrawCategories(const std::vector<std::string_view>& categories,
                             CvarSettingsUiState& state, Emulator* emulator,
                             bool per_game_mode, bool include_curated = false,
                             bool gpu_advanced_only = false);

  static void DrawGameOverridesPanel(Emulator* emulator);

  static bool MatchesSearch(const ::cvar::IConfigVar& config_var,
                            std::string_view filter);

  // Combo with "Recommended" as index 0 when a title is open.
  static bool DrawStringComboWithRecommended(
      const char* label, std::string& value, const char* const* options,
      int option_count, const char* category, const char* cvar_name,
      Emulator* emulator, bool per_game_mode);

  // Hover "?" showing the cvar description (no-op if unknown or empty).
  static void DrawCvarHelpMarker(const char* cvar_name);
};

}  // namespace app
}  // namespace xe

#endif  // XENIA_APP_CVAR_SETTINGS_UI_H_
