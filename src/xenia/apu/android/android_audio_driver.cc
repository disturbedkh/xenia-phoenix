/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/apu/android/android_audio_driver.h"

#include <cstring>

#include "xenia/base/assert.h"
#include "xenia/base/logging.h"
#include "xenia/base/main_android.h"

namespace xe {
namespace apu {
namespace android {

AndroidAudioDriver::AndroidAudioDriver(xe::threading::Semaphore* semaphore,
                                       uint32_t frequency, uint32_t channels,
                                       bool need_format_conversion)
    : semaphore_(semaphore),
      frame_frequency_(frequency),
      frame_channels_(channels),
      need_format_conversion_(need_format_conversion) {
  switch (frame_channels_) {
    case 6:
      channel_samples_ = 256;
      break;
    case 2:
      channel_samples_ = 768;
      break;
    default:
      channel_samples_ = 256;
      break;
  }
  frame_size_ = sizeof(float) * frame_channels_ * channel_samples_;
  assert_true(frame_size_ <= kFrameSizeMax);
}

AndroidAudioDriver::~AndroidAudioDriver() { Shutdown(); }

bool AndroidAudioDriver::Initialize() {
  if (GetAndroidApiLevel() < 26) {
    XELOGE("AndroidAudioDriver requires API 26+ (AAudio)");
    return false;
  }

  AAudioStreamBuilder* builder = nullptr;
  if (AAudio_createStreamBuilder(&builder) != AAUDIO_OK || !builder) {
    return false;
  }

  AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_OUTPUT);
  AAudioStreamBuilder_setPerformanceMode(builder,
                                         AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
  AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_SHARED);
  AAudioStreamBuilder_setSampleRate(builder, frame_frequency_);
  AAudioStreamBuilder_setChannelCount(builder, frame_channels_);
  AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_FLOAT);
  AAudioStreamBuilder_setDataCallback(builder, DataCallback, this);

  aaudio_result_t result = AAudioStreamBuilder_openStream(builder, &stream_);
  AAudioStreamBuilder_delete(builder);
  if (result != AAUDIO_OK || !stream_) {
    XELOGE("AAudioStreamBuilder_openStream failed: {}", result);
    return false;
  }

  result = AAudioStream_requestStart(stream_);
  if (result != AAUDIO_OK) {
    XELOGE("AAudioStream_requestStart failed: {}", result);
    Shutdown();
    return false;
  }
  return true;
}

void AndroidAudioDriver::Shutdown() {
  if (stream_) {
    AAudioStream_requestStop(stream_);
    AAudioStream_close(stream_);
    stream_ = nullptr;
  }
  std::lock_guard<std::mutex> guard(frames_mutex_);
  while (!frames_queued_.empty()) {
    delete[] frames_queued_.front();
    frames_queued_.pop();
  }
  while (!frames_unused_.empty()) {
    delete[] frames_unused_.front();
    frames_unused_.pop();
  }
}

void AndroidAudioDriver::SubmitFrame(float* frame) {
  float* output_frame;
  {
    std::lock_guard<std::mutex> guard(frames_mutex_);
    if (frames_unused_.empty()) {
      output_frame = new float[frame_channels_ * channel_samples_];
    } else {
      output_frame = frames_unused_.front();
      frames_unused_.pop();
    }
  }
  std::memcpy(output_frame, frame, frame_size_);
  {
    std::lock_guard<std::mutex> guard(frames_mutex_);
    frames_queued_.push(output_frame);
  }
  if (semaphore_) {
    int previous_count = 0;
    semaphore_->Release(1, &previous_count);
  }
}

void AndroidAudioDriver::Pause() {
  if (stream_) {
    AAudioStream_requestPause(stream_);
  }
}

void AndroidAudioDriver::Resume() {
  if (stream_) {
    AAudioStream_requestStart(stream_);
  }
}

void AndroidAudioDriver::SetVolume(float volume) {
  (void)volume;
#if __ANDROID_API__ >= 29
  if (stream_) {
    AAudioStream_setVolume(stream_, volume);
  }
#endif
}

aaudio_data_callback_result_t AndroidAudioDriver::DataCallback(
    AAudioStream* stream, void* user_data, void* audio_data,
    int32_t num_frames) {
  auto* driver = static_cast<AndroidAudioDriver*>(user_data);
  const size_t samples_needed =
      static_cast<size_t>(num_frames) * driver->frame_channels_;
  float* out = static_cast<float*>(audio_data);
  float* frame = nullptr;
  {
    std::lock_guard<std::mutex> guard(driver->frames_mutex_);
    if (!driver->frames_queued_.empty()) {
      frame = driver->frames_queued_.front();
      driver->frames_queued_.pop();
    }
  }
  if (frame) {
    const size_t copy_samples =
        std::min(samples_needed, static_cast<size_t>(driver->frame_channels_ *
                                                     driver->channel_samples_));
    std::memcpy(out, frame, copy_samples * sizeof(float));
    if (copy_samples < samples_needed) {
      std::memset(out + copy_samples, 0,
                  (samples_needed - copy_samples) * sizeof(float));
    }
    std::lock_guard<std::mutex> guard(driver->frames_mutex_);
    driver->frames_unused_.push(frame);
  } else {
    std::memset(out, 0, samples_needed * sizeof(float));
  }
  return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

}  // namespace android
}  // namespace apu
}  // namespace xe
