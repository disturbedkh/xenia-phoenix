/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/ui/surface_mac.h"

#import <QuartzCore/CAMetalLayer.h>

namespace xe {
namespace ui {

bool MacMetalLayerSurface::GetSizeImpl(uint32_t& width_out,
                                       uint32_t& height_out) const {
  auto* layer = static_cast<CAMetalLayer*>(metal_layer_);
  if (!layer) {
    width_out = 0;
    height_out = 0;
    return false;
  }
  const CGSize size = layer.drawableSize;
  width_out = uint32_t(size.width);
  height_out = uint32_t(size.height);
  return width_out != 0 && height_out != 0;
}

}  // namespace ui
}  // namespace xe
