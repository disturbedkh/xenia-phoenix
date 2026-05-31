/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_APP_SOCIAL_PANEL_H_
#define XENIA_APP_SOCIAL_PANEL_H_

#include <functional>

namespace xe {
namespace app {

class EmulatorWindow;

class SocialPanel {
 public:
  explicit SocialPanel(EmulatorWindow& window);

  void Draw();
  void SetOnOpenNetplaySettings(std::function<void()> cb);
  void SetOnOpenFriends(std::function<void()> cb);

 private:
  EmulatorWindow& window_;
  std::function<void()> on_netplay_settings_;
  std::function<void()> on_friends_;
};

}  // namespace app
}  // namespace xe

#endif  // XENIA_APP_SOCIAL_PANEL_H_
