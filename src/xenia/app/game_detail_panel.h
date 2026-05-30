/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_APP_GAME_DETAIL_PANEL_H_
#define XENIA_APP_GAME_DETAIL_PANEL_H_

#include <functional>

#include "xenia/app/content_manager.h"
#include "xenia/app/cover_art_fetcher.h"
#include "xenia/app/game_metadata.h"
#include "xenia/app/library/game_library.h"
#include "xenia/app/patch_catalog.h"

namespace xe {
namespace app {

class EmulatorWindow;

class GameDetailPanel {
 public:
  GameDetailPanel(EmulatorWindow& window, library::GameLibrary& library,
                  GameMetadataService& metadata, CoverArtFetcher& covers,
                  PatchCatalog& patches, ContentManager& content);

  void SetEntry(const library::LibraryEntry* entry);
  const library::LibraryEntry* entry() const { return entry_; }

  void Draw(float width);
  void SetOnLaunch(std::function<void(const std::filesystem::path&)> cb);
  void SetOnConfigure(std::function<void(uint32_t)> cb);

 private:
  EmulatorWindow& window_;
  library::GameLibrary& library_;
  GameMetadataService& metadata_;
  CoverArtFetcher& covers_;
  PatchCatalog& patches_;
  ContentManager& content_;
  const library::LibraryEntry* entry_ = nullptr;
  std::function<void(const std::filesystem::path&)> on_launch_;
  std::function<void(uint32_t)> on_configure_;
};

}  // namespace app
}  // namespace xe

#endif  // XENIA_APP_GAME_DETAIL_PANEL_H_
