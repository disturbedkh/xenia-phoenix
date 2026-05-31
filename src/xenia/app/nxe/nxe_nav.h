/**

 ******************************************************************************

 * Xenia : Xbox 360 Emulator Research Project                                 *

 ******************************************************************************

 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *

 * Released under the BSD license - see LICENSE in the root for more details. *

 ******************************************************************************

 */

#ifndef XENIA_APP_NXE_NXE_NAV_H_

#define XENIA_APP_NXE_NXE_NAV_H_

#include "third_party/imgui/imgui.h"

namespace xe {

namespace app {

namespace nxe {

enum class FocusList { kMaster, kDetail, kContent };

enum class NavAxis { kHorizontal, kVertical };

struct NxeNavState {
  FocusList focused_list = FocusList::kDetail;

  int master_index = 1;

  int detail_index = 0;

  int content_index = 0;

  int item_count = 0;

  int column_count = 1;

  bool confirm = false;

  bool back = false;

  bool configure = false;

  bool manage = false;

  bool guide = false;

  bool tab_prev = false;

  bool tab_next = false;

  bool move_left = false;

  bool move_right = false;

  bool move_up = false;

  bool move_down = false;
};

class NxeNav {
 public:
  static constexpr float kRepeatDelay = 0.4f;

  static constexpr float kRepeatInterval = 0.15f;

  void BeginFrame(ImGuiIO& io, float delta_time);

  void ResetEdgeFlags();

  NxeNavState& state() { return state_; }

  const NxeNavState& state() const { return state_; }

  void MoveDetail(int delta, NavAxis axis);

  void MoveContent(int delta, NavAxis axis);

 private:
  void PollDirectionRepeat(float delta_time, bool held, bool& edge_out,

                           float& hold_timer, bool& was_held);

  NxeNavState state_;

  float left_timer_ = 0.f;

  float right_timer_ = 0.f;

  float up_timer_ = 0.f;

  float down_timer_ = 0.f;

  bool left_was_held_ = false;

  bool right_was_held_ = false;

  bool up_was_held_ = false;

  bool down_was_held_ = false;
};

}  // namespace nxe

}  // namespace app

}  // namespace xe

#endif  // XENIA_APP_NXE_NXE_NAV_H_
