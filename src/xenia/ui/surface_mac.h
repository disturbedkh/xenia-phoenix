/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_UI_SURFACE_MAC_H_
#define XENIA_UI_SURFACE_MAC_H_

#include "xenia/ui/surface.h"

namespace xe {
namespace ui {

// CAMetalLayer-backed surface for MoltenVK (VK_EXT_metal_surface).
class MacMetalLayerSurface final : public Surface {
 public:
  explicit MacMetalLayerSurface(void* metal_layer)
      : metal_layer_(metal_layer) {}
  TypeIndex GetType() const override { return kTypeIndex_MacMetalLayer; }
  void* metal_layer() const { return metal_layer_; }

 protected:
  bool GetSizeImpl(uint32_t& width_out, uint32_t& height_out) const override;

 private:
  void* metal_layer_;
};

}  // namespace ui
}  // namespace xe

#endif  // XENIA_UI_SURFACE_MAC_H_
