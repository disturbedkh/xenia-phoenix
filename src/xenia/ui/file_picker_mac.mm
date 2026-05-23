/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/ui/file_picker.h"

#import <Cocoa/Cocoa.h>

#include "xenia/base/filesystem.h"
#include "xenia/base/string.h"
#include "xenia/ui/window_mac.h"

namespace xe {
namespace ui {

class MacFilePicker : public FilePicker {
 public:
  bool Show(Window* parent_window) override;
};

std::unique_ptr<FilePicker> FilePicker::Create() {
  return std::make_unique<MacFilePicker>();
}

bool MacFilePicker::Show(Window* parent_window) {
  NSOpenPanel* panel = [NSOpenPanel openPanel];
  panel.canChooseFiles = type() == Type::kFile;
  panel.canChooseDirectories = type() == Type::kDirectory;
  panel.allowsMultipleSelection = multi_selection();
  panel.title = [NSString stringWithUTF8String:title().c_str()];

  NSWindow* parent = nullptr;
  if (parent_window) {
    parent = (__bridge NSWindow*)static_cast<MacWindow*>(parent_window)->ns_window();
  }

  (void)parent;
  if ([panel runModal] != NSModalResponseOK) {
    return false;
  }

  std::vector<std::filesystem::path> selected;
  for (NSURL* url in panel.URLs) {
    selected.push_back(xe::to_path([[url path] UTF8String]));
  }
  set_selected_files(std::move(selected));
  return !selected.empty();
}

}  // namespace ui
}  // namespace xe
