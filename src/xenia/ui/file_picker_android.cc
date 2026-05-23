/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2022 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include <memory>

#include "xenia/base/filesystem.h"
#include "xenia/base/logging.h"
#include "xenia/ui/file_picker.h"

namespace xe {
namespace ui {

class AndroidFilePicker : public FilePicker {
 public:
  bool Show(Window* parent_window) override {
    const char* mime = "*/*";
    if (!extensions().empty()) {
      mime = "application/octet-stream";
    }
    if (!xe::filesystem::AndroidRequestOpenDocument(mime)) {
      XELOGE("AndroidFilePicker: failed to launch OPEN_DOCUMENT intent");
      return false;
    }
    // Result URI is delivered via onActivityResult → launch Bundle → cvars.
    // Caller should read `target_content_uri` from launch arguments.
    XELOGI(
        "AndroidFilePicker: OPEN_DOCUMENT started; pass result URI via "
        "WindowedAppActivity launch Bundle (target_content_uri)");
    return true;
  }
};

std::unique_ptr<FilePicker> FilePicker::Create() {
  return std::make_unique<AndroidFilePicker>();
}

}  // namespace ui
}  // namespace xe
