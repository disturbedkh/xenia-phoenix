/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/ui/windowed_app_context_mac.h"

#import <Cocoa/Cocoa.h>

namespace xe {
namespace ui {

MacWindowedAppContext::~MacWindowedAppContext() = default;

void MacWindowedAppContext::NotifyUILoopOfPendingFunctions() {
  if (!IsInUIThread()) {
    dispatch_async(dispatch_get_main_queue(), ^{
      ExecutePendingFunctionsFromUIThread();
    });
    return;
  }
  ExecutePendingFunctionsFromUIThread();
}

void MacWindowedAppContext::PlatformQuitFromUIThread() {
  if (NSApp) {
    [NSApp terminate:nil];
  }
}

void MacWindowedAppContext::RunMainNSApplicationLoop() {
  [NSApp run];
  QuitFromUIThread();
}

}  // namespace ui
}  // namespace xe
