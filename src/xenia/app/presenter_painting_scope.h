/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_APP_PRESENTER_PAINTING_SCOPE_H_
#define XENIA_APP_PRESENTER_PAINTING_SCOPE_H_

#include "xenia/app/emulator_window.h"

namespace xe {
namespace app {

// RAII: tears down presenter painting on entry, restores on exit.
class PresenterPaintingScope {
 public:
  explicit PresenterPaintingScope(EmulatorWindow& window)
      : window_(window), active_(true) {
    window_.ShutdownGraphicsSystemPresenterPainting();
  }

  ~PresenterPaintingScope() {
    if (active_) {
      window_.SetupGraphicsSystemPresenterPainting();
    }
  }

  PresenterPaintingScope(const PresenterPaintingScope&) = delete;
  PresenterPaintingScope& operator=(const PresenterPaintingScope&) = delete;

  void Release() { active_ = false; }

 private:
  EmulatorWindow& window_;
  bool active_;
};

}  // namespace app
}  // namespace xe

#endif  // XENIA_APP_PRESENTER_PAINTING_SCOPE_H_
