/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_APU_ANDROID_ANDROID_AUDIO_SYSTEM_H_
#define XENIA_APU_ANDROID_ANDROID_AUDIO_SYSTEM_H_

#include <memory>

#include "xenia/apu/audio_system.h"

namespace xe {
namespace cpu {
class Processor;
}  // namespace cpu
}  // namespace xe

namespace xe {
namespace apu {
namespace android {

class AndroidAudioSystem : public AudioSystem {
 public:
  static bool IsAvailable() { return true; }

  static std::unique_ptr<AudioSystem> Create(cpu::Processor* processor);

  explicit AndroidAudioSystem(cpu::Processor* processor);
  ~AndroidAudioSystem() override;

  std::string name() const override { return "android"; }

  X_STATUS CreateDriver(size_t index, xe::threading::Semaphore* semaphore,
                        AudioDriver** out_driver) override;

  AudioDriver* CreateDriver(xe::threading::Semaphore* semaphore,
                            uint32_t frequency, uint32_t channels,
                            bool need_format_conversion) override;

  void DestroyDriver(AudioDriver* driver) override;
};

}  // namespace android
}  // namespace apu
}  // namespace xe

#endif  // XENIA_APU_ANDROID_ANDROID_AUDIO_SYSTEM_H_
