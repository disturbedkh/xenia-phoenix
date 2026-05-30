/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_APP_NXE_NXE_STATE_H_
#define XENIA_APP_NXE_NXE_STATE_H_

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include "xenia/app/nxe/nxe_anim.h"

namespace xe {
namespace app {

class CoverArtFetcher;
class GameMetadataService;
class PatchCatalog;
class ContentManager;

namespace library {
class GameLibrary;
struct LibraryEntry;
}  // namespace library

namespace nxe {

enum class NxeScreen {
  kDashboard,
  kGameLibrary,
  kAchievements,
  kSettingsHub,
  kSettingsCore,
  kSettingsColors,
  kSettingsSystem,
  kSettingsAudio,
  kSettingsDisplay,
  kSettingsConfig,
  kPatchesManager,
  kAboutHub,
  kProfileSwitch,
  kFriends,
  kContentManager,
  kGuide,
};

struct BladeItem {
  std::string id;
  std::string label;
  std::string description;
  std::filesystem::path icon_path;
  NxeScreen target = NxeScreen::kDashboard;
};

struct NxeContext {
  library::GameLibrary* game_library = nullptr;
  GameMetadataService* metadata = nullptr;
  CoverArtFetcher* cover_fetcher = nullptr;
  PatchCatalog* patch_catalog = nullptr;
  ContentManager* content_manager = nullptr;

  std::function<void(const std::filesystem::path&)> on_launch;
  std::function<void(uint32_t title_id)> on_configure;
  std::function<void()> on_library_settings;
  std::function<void()> on_preferences;
  std::function<void()> on_netplay;
  std::function<void()> on_friends;
  std::function<void()> on_guide;
  std::function<void()> on_manage_profiles;
  std::function<void()> on_profile_switch;

  NxeScreen current_screen = NxeScreen::kDashboard;
  NxeScreen previous_screen = NxeScreen::kDashboard;
  float transition_alpha = 1.f;
  Tween transition_tween;

  int master_index = 1;
  AnimatedFloat detail_index_anim;
  int detail_index = 0;
  int library_focus_index = 0;
  AnimatedFloat library_focus_anim;
  int achievement_title_index = 0;

  char search_buffer[128] = {};
  int sort_mode = 0;

  std::vector<BladeItem> detail_blades;
  std::vector<const library::LibraryEntry*> filtered_entries;

  bool overlay_while_playing = false;
};

}  // namespace nxe
}  // namespace app
}  // namespace xe

#endif  // XENIA_APP_NXE_NXE_STATE_H_
