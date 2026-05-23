/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/apu/android/android_audio_system.h"

#include "xenia/apu/android/android_audio_driver.h"
#include "xenia/base/assert.h"

namespace xe {
namespace apu {
namespace android {

std::unique_ptr<AudioSystem> AndroidAudioSystem::Create(
    cpu::Processor* processor) {
  return std::make_unique<AndroidAudioSystem>(processor);
}

AndroidAudioSystem::AndroidAudioSystem(cpu::Processor* processor)
    : AudioSystem(processor) {}

AndroidAudioSystem::~AndroidAudioSystem() = default;

X_STATUS AndroidAudioSystem::CreateDriver(size_t index,
                                          xe::threading::Semaphore* semaphore,
                                          AudioDriver** out_driver) {
  assert_not_null(out_driver);
  auto driver = std::make_unique<AndroidAudioDriver>(
      semaphore, AndroidAudioDriver::kFrameFrequencyDefault,
      AndroidAudioDriver::kFrameChannelsDefault, true);
  if (!driver->Initialize()) {
    return X_STATUS_UNSUCCESSFUL;
  }
  *out_driver = driver.release();
  return X_STATUS_SUCCESS;
}

AudioDriver* AndroidAudioSystem::CreateDriver(
    xe::threading::Semaphore* semaphore, uint32_t frequency, uint32_t channels,
    bool need_format_conversion) {
  auto driver = std::make_unique<AndroidAudioDriver>(
      semaphore, frequency, channels, need_format_conversion);
  if (!driver->Initialize()) {
    return nullptr;
  }
  return driver.release();
}

void AndroidAudioSystem::DestroyDriver(AudioDriver* driver) {
  assert_not_null(driver);
  auto* android_driver = static_cast<AndroidAudioDriver*>(driver);
  android_driver->Shutdown();
  delete android_driver;
}

}  // namespace android
}  // namespace apu
}  // namespace xe
