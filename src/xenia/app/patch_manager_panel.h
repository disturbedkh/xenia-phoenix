/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_APP_PATCH_MANAGER_PANEL_H_
#define XENIA_APP_PATCH_MANAGER_PANEL_H_

#include <cstdint>

#include "xenia/app/patch_catalog.h"

namespace xe {
namespace app {

class PatchManagerPanel {
 public:
  explicit PatchManagerPanel(PatchCatalog& catalog);

  void SetTitle(uint32_t title_id);
  void Draw();

 private:
  PatchCatalog& catalog_;
  uint32_t title_id_ = 0;
};

}  // namespace app
}  // namespace xe

#endif  // XENIA_APP_PATCH_MANAGER_PANEL_H_
