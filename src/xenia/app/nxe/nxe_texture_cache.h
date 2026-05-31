/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_APP_NXE_NXE_TEXTURE_CACHE_H_
#define XENIA_APP_NXE_NXE_TEXTURE_CACHE_H_

#include <filesystem>
#include <map>
#include <memory>
#include <span>
#include <string>

namespace xe {
namespace ui {
class ImmediateDrawer;
class ImmediateTexture;
}  // namespace ui
}  // namespace xe

namespace xe {
namespace app {
namespace nxe {

class NxeTextureCache {
 public:
  explicit NxeTextureCache(ui::ImmediateDrawer* drawer);

  ui::ImmediateTexture* LoadFile(const std::filesystem::path& path);
  ui::ImmediateTexture* LoadMemory(const std::string& key,
                                   std::span<const uint8_t> rgba, int w, int h);
  void Invalidate(const std::string& key);
  void Clear();

 private:
  ui::ImmediateDrawer* drawer_ = nullptr;
  std::map<std::string, std::unique_ptr<ui::ImmediateTexture>> cache_;
};

}  // namespace nxe
}  // namespace app
}  // namespace xe

#endif  // XENIA_APP_NXE_NXE_TEXTURE_CACHE_H_
