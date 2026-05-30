/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/app/nxe/nxe_texture_cache.h"

#include "third_party/stb/stb_image.h"
#include "xenia/base/platform.h"
#include "xenia/ui/immediate_drawer.h"

namespace xe {
namespace app {
namespace nxe {

NxeTextureCache::NxeTextureCache(ui::ImmediateDrawer* drawer)
    : drawer_(drawer) {}

ui::ImmediateTexture* NxeTextureCache::LoadFile(
    const std::filesystem::path& path) {
  if (!drawer_ || path.empty()) {
    return nullptr;
  }
  const std::string key = path_to_utf8(path);
  auto it = cache_.find(key);
  if (it != cache_.end()) {
    return it->second.get();
  }
  std::error_code ec;
  if (!std::filesystem::exists(path, ec)) {
    return nullptr;
  }
  int w = 0, h = 0, ch = 0;
  unsigned char* pixels =
      stbi_load(path_to_utf8(path).c_str(), &w, &h, &ch, STBI_rgb_alpha);
  if (!pixels) {
    return nullptr;
  }
  auto tex = drawer_->CreateTexture(w, h, ui::ImmediateTextureFilter::kLinear,
                                    true, pixels);
  stbi_image_free(pixels);
  if (!tex) {
    return nullptr;
  }
  auto* ptr = tex.get();
  cache_[key] = std::move(tex);
  return ptr;
}

ui::ImmediateTexture* NxeTextureCache::LoadMemory(const std::string& key,
                                                  std::span<const uint8_t> rgba,
                                                  int w, int h) {
  if (!drawer_ || rgba.empty() || w <= 0 || h <= 0) {
    return nullptr;
  }
  auto it = cache_.find(key);
  if (it != cache_.end()) {
    return it->second.get();
  }
  auto tex = drawer_->CreateTexture(
      static_cast<uint32_t>(w), static_cast<uint32_t>(h),
      ui::ImmediateTextureFilter::kLinear, true, rgba.data());
  if (!tex) {
    return nullptr;
  }
  auto* ptr = tex.get();
  cache_[key] = std::move(tex);
  return ptr;
}

void NxeTextureCache::Invalidate(const std::string& key) { cache_.erase(key); }

void NxeTextureCache::Clear() { cache_.clear(); }

}  // namespace nxe
}  // namespace app
}  // namespace xe
