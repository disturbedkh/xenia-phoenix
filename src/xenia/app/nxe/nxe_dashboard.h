/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_APP_NXE_NXE_DASHBOARD_H_
#define XENIA_APP_NXE_NXE_DASHBOARD_H_

#include "xenia/app/content_manager.h"
#include "xenia/app/cover_art_fetcher.h"
#include "xenia/app/game_metadata.h"
#include "xenia/app/library/game_library.h"
#include "xenia/app/nxe/nxe_data.h"
#include "xenia/app/nxe/nxe_nav.h"
#include "xenia/app/nxe/nxe_state.h"
#include "xenia/app/nxe/nxe_texture_cache.h"
#include "xenia/app/patch_catalog.h"

namespace xe {
namespace ui {
class ImGuiDrawer;
}  // namespace ui
namespace app {

class EmulatorWindow;

namespace nxe {

class NxeDashboard {
 public:
  NxeDashboard(EmulatorWindow& window, library::GameLibrary& library,
               std::filesystem::path patches_root);

  void SetCallbacks(NxeContext& ctx);
  void Draw(ImGuiIO& io);

 private:
  void InitBlades();
  void RefreshFilteredEntries(NxeContext& ctx);
  void HandleNavigation(NxeContext& ctx, float dt);
  void DrawBackground(NxeContext& ctx, ImGuiIO& io);
  void DrawChrome(NxeContext& ctx, ImGuiIO& io);
  void DrawScreen(NxeContext& ctx, ImGuiIO& io);
  void DrawDashboardHome(NxeContext& ctx, ImGuiIO& io);
  void DrawGameLibrary(NxeContext& ctx, ImGuiIO& io);
  void DrawAchievements(NxeContext& ctx, ImGuiIO& io);
  void DrawSettingsHub(NxeContext& ctx, ImGuiIO& io);
  void DrawSettingsSub(NxeContext& ctx, ImGuiIO& io, NxeScreen sub);
  void DrawProfileSwitch(NxeContext& ctx, ImGuiIO& io);
  void DrawProfilePanel(NxeContext& ctx, ImGuiIO& io);
  void DrawNoProfileBanner(NxeContext& ctx, ImGuiIO& io);
  void DrawProfileCardButton(NxeContext& ctx, ImGuiIO& io);
  void DrawAboutHub(NxeContext& ctx, ImGuiIO& io);
  void DrawFriends(NxeContext& ctx, ImGuiIO& io);
  void DrawContentManager(NxeContext& ctx, ImGuiIO& io);
  void DrawPatchesManager(NxeContext& ctx, ImGuiIO& io);
  void NavigateTo(NxeContext& ctx, NxeScreen screen);
  ui::ImmediateTexture* CoverForEntry(const library::LibraryEntry& entry);

  EmulatorWindow& window_;
  library::GameLibrary& library_;
  NxeContext ctx_;
  NxeNav nav_;
  NxeTextureCache textures_;
  NxeDataAdapter data_;

  GameMetadataService metadata_;
  CoverArtFetcher cover_fetcher_;
  PatchCatalog patch_catalog_;
  ContentManager content_manager_;

  ui::ImmediateTexture* wallpaper_tex_ = nullptr;
  ui::ImmediateTexture* stage_tex_ = nullptr;
  ui::ImmediateTexture* flourish_tex_ = nullptr;
  ui::ImmediateTexture* guide_orb_tex_ = nullptr;
};

}  // namespace nxe
}  // namespace app
}  // namespace xe

#endif  // XENIA_APP_NXE_NXE_DASHBOARD_H_
