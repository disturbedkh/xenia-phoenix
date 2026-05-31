/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_APP_NXE_NXE_ANIM_H_
#define XENIA_APP_NXE_NXE_ANIM_H_

#include <cmath>

namespace xe {
namespace app {
namespace nxe {

enum class EaseType {
  kLinear,
  kEaseOutCubic,
  kEaseInOutCubic,
  kEaseOutQuad,
};

inline float Ease(EaseType type, float t) {
  t = t < 0.f ? 0.f : (t > 1.f ? 1.f : t);
  switch (type) {
    case EaseType::kLinear:
      return t;
    case EaseType::kEaseOutCubic: {
      const float u = 1.f - t;
      return 1.f - u * u * u;
    }
    case EaseType::kEaseInOutCubic:
      return t < 0.5f ? 4.f * t * t * t
                      : 1.f - std::pow(-2.f * t + 2.f, 3.f) / 2.f;
    case EaseType::kEaseOutQuad: {
      const float u = 1.f - t;
      return 1.f - u * u;
    }
  }
  return t;
}

// Smoothly approaches target each frame (frame-rate independent).
inline float SmoothDamp(float current, float target, float delta_time,
                        float speed = 8.f) {
  if (speed <= 0.f) {
    return target;
  }
  const float t = 1.f - std::exp(-speed * delta_time);
  return current + (target - current) * t;
}

class AnimatedFloat {
 public:
  AnimatedFloat() = default;
  explicit AnimatedFloat(float value) : current_(value), target_(value) {}

  void SetTarget(float target) { target_ = target; }
  void SetImmediate(float value) { current_ = target_ = value; }

  float Update(float delta_time, float speed = 8.f) {
    current_ = SmoothDamp(current_, target_, delta_time, speed);
    return current_;
  }

  float current() const { return current_; }
  float target() const { return target_; }
  bool IsSettled(float epsilon = 0.001f) const {
    return std::fabs(current_ - target_) <= epsilon;
  }

 private:
  float current_ = 0.f;
  float target_ = 0.f;
};

class Tween {
 public:
  void Start(float from, float to, float duration,
             EaseType ease = EaseType::kEaseOutCubic) {
    from_ = from;
    to_ = to;
    duration_ = duration > 0.f ? duration : 0.001f;
    elapsed_ = 0.f;
    active_ = true;
    ease_ = ease;
  }

  float Update(float delta_time) {
    if (!active_) {
      return to_;
    }
    elapsed_ += delta_time;
    const float t = elapsed_ / duration_;
    if (t >= 1.f) {
      active_ = false;
      return to_;
    }
    return from_ + (to_ - from_) * Ease(ease_, t);
  }

  bool active() const { return active_; }

 private:
  float from_ = 0.f;
  float to_ = 0.f;
  float duration_ = 0.4f;
  float elapsed_ = 0.f;
  bool active_ = false;
  EaseType ease_ = EaseType::kEaseOutCubic;
};

}  // namespace nxe
}  // namespace app
}  // namespace xe

#endif  // XENIA_APP_NXE_NXE_ANIM_H_
