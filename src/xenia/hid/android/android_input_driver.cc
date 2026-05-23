/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/hid/android/android_input_driver.h"

#include <cstring>

#include "xenia/base/assert.h"
#include "xenia/hid/input.h"

namespace xe {
namespace hid {
namespace android {

AndroidInputDriver::AndroidInputDriver(xe::ui::Window* window,
                                       size_t window_z_order)
    : InputDriver(window, window_z_order) {
  std::memset(&state_, 0, sizeof(state_));
}

AndroidInputDriver::~AndroidInputDriver() = default;

X_STATUS AndroidInputDriver::Setup() { return X_STATUS_SUCCESS; }

X_RESULT AndroidInputDriver::GetCapabilities(uint32_t user_index, uint32_t flags,
                                             X_INPUT_CAPABILITIES* out_caps) {
  if (user_index != 0 || !out_caps) {
    return X_ERROR_DEVICE_NOT_CONNECTED;
  }
  std::memset(out_caps, 0, sizeof(*out_caps));
  out_caps->type = 0x01;  // XINPUT_DEVTYPE_GAMEPAD
  out_caps->sub_type = 0x01;  // XINPUT_DEVSUBTYPE_GAMEPAD
  out_caps->flags = 0;
  out_caps->gamepad.buttons = 0xFFFF;
  out_caps->gamepad.left_trigger = 0xFF;
  out_caps->gamepad.right_trigger = 0xFF;
  out_caps->gamepad.thumb_lx = static_cast<int16_t>(0x7FFF);
  out_caps->gamepad.thumb_ly = static_cast<int16_t>(0x7FFF);
  out_caps->gamepad.thumb_rx = static_cast<int16_t>(0x7FFF);
  out_caps->gamepad.thumb_ry = static_cast<int16_t>(0x7FFF);
  return X_ERROR_SUCCESS;
}

X_RESULT AndroidInputDriver::GetState(uint32_t user_index,
                                    X_INPUT_STATE* out_state) {
  if (user_index != 0 || !out_state) {
    return X_ERROR_DEVICE_NOT_CONNECTED;
  }
  std::lock_guard<std::mutex> guard(state_mutex_);
  *out_state = state_;
  return X_ERROR_SUCCESS;
}

X_RESULT AndroidInputDriver::SetState(uint32_t user_index,
                                      X_INPUT_VIBRATION* vibration) {
  if (user_index != 0) {
    return X_ERROR_DEVICE_NOT_CONNECTED;
  }
  (void)vibration;
  return X_ERROR_SUCCESS;
}

X_RESULT AndroidInputDriver::GetKeystroke(uint32_t user_index, uint32_t flags,
                                          X_INPUT_KEYSTROKE* out_keystroke) {
  return X_ERROR_EMPTY;
}

void AndroidInputDriver::OnTouch(float norm_x, float norm_y, bool down) {
  std::lock_guard<std::mutex> guard(state_mutex_);
  // Map touch position to left stick; A when pressed.
  const int16_t lx = static_cast<int16_t>(norm_x * 32767.0f);
  const int16_t ly = static_cast<int16_t>(-norm_y * 32767.0f);
  state_.gamepad.thumb_lx = lx;
  state_.gamepad.thumb_ly = ly;
  uint16_t buttons = state_.gamepad.buttons;
  if (down) {
    buttons |= X_INPUT_GAMEPAD_A;
  } else {
    buttons &= ~X_INPUT_GAMEPAD_A;
  }
  state_.gamepad.buttons = buttons;
  state_.packet_number = ++packet_number_;
}

}  // namespace android
}  // namespace hid
}  // namespace xe
