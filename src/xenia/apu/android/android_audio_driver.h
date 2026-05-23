/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_APU_ANDROID_ANDROID_AUDIO_DRIVER_H_
#define XENIA_APU_ANDROID_ANDROID_AUDIO_DRIVER_H_

#include <aaudio/AAudio.h>
#include <atomic>
#include <mutex>
#include <queue>

#include "xenia/apu/audio_driver.h"
#include "xenia/base/threading.h"

namespace xe {
namespace apu {
namespace android {

class AndroidAudioDriver : public AudioDriver {
 public:
  explicit AndroidAudioDriver(xe::threading::Semaphore* semaphore,
                              uint32_t frequency, uint32_t channels,
                              bool need_format_conversion);
  ~AndroidAudioDriver() override;

  bool Initialize() override;
  void Shutdown() override;

  void SubmitFrame(float* samples) override;
  void Pause() override;
  void Resume() override;
  void SetVolume(float volume) override;

 private:
  static aaudio_data_callback_result_t DataCallback(AAudioStream* stream,
                                                    void* user_data,
                                                    void* audio_data,
                                                    int32_t num_frames);

  xe::threading::Semaphore* semaphore_;
  uint32_t frame_frequency_;
  uint32_t frame_channels_;
  uint32_t channel_samples_;
  uint32_t frame_size_;
  bool need_format_conversion_;

  AAudioStream* stream_ = nullptr;
  std::mutex frames_mutex_;
  std::queue<float*> frames_queued_;
  std::queue<float*> frames_unused_;
};

}  // namespace android
}  // namespace apu
}  // namespace xe

#endif  // XENIA_APU_ANDROID_ANDROID_AUDIO_DRIVER_H_
