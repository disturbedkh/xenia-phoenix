/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include <cstdlib>

#import <Cocoa/Cocoa.h>

#include "xenia/base/cvar.h"
#include "xenia/base/logging.h"
#include "xenia/ui/windowed_app.h"
#include "xenia/ui/windowed_app_context_mac.h"

int main(int argc, char** argv) {
  @autoreleasepool {
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    xe::ui::MacWindowedAppContext app_context;

    std::unique_ptr<xe::ui::WindowedApp> app =
        xe::ui::GetWindowedAppCreator()(app_context);

    cvar::ParseLaunchArguments(argc, argv, app->GetPositionalOptionsUsage(),
                               app->GetPositionalOptions());

    xe::InitializeLogging(app->GetName());

    int result = EXIT_FAILURE;
    if (app->OnInitialize()) {
      app_context.RunMainNSApplicationLoop();
      result = EXIT_SUCCESS;
    }

    app->InvokeOnDestroy();
    xe::ShutdownLogging();
    return result;
  }
}
