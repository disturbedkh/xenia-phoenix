/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_UI_WINDOW_MAC_H_
#define XENIA_UI_WINDOW_MAC_H_

#include <memory>
#include <string>

#include "xenia/ui/menu_item.h"
#include "xenia/ui/window.h"

namespace xe {
namespace ui {

class MacWindow;

class MacMenuItem final : public MenuItem {
 public:
  MacMenuItem(Type type, const std::string& text, const std::string& hotkey,
              std::function<void()> callback)
      : MenuItem(type, text, hotkey, std::move(callback)) {}
};

class MacWindow : public Window {
  using super = Window;

 public:
  MacWindow(WindowedAppContext& app_context, const std::string_view title,
            uint32_t desired_logical_width, uint32_t desired_logical_height);
  ~MacWindow() override;

  void* ns_window() const { return ns_window_; }
  void* metal_layer() const { return metal_layer_; }

 protected:
  bool OpenImpl() override;
  void RequestCloseImpl() override;

  void ApplyNewFullscreen() override;
  void ApplyNewTitle() override;
  void ApplyNewMainMenu(MenuItem* old_main_menu) override;
  void FocusImpl() override;

  std::unique_ptr<Surface> CreateSurfaceImpl(
      Surface::TypeFlags allowed_types) override;
  void RequestPaintImpl() override;

  void OnMacClose();
  void OnMacResize(uint32_t width, uint32_t height);
  void OnMacFocus(bool focused);

 private:
  void* ns_window_ = nullptr;
  void* metal_view_ = nullptr;
  void* metal_layer_ = nullptr;
};

}  // namespace ui
}  // namespace xe

#endif  // XENIA_UI_WINDOW_MAC_H_
