/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_APP_NXE_NXE_AUDIO_H_
#define XENIA_APP_NXE_NXE_AUDIO_H_

#include <filesystem>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace xe {
namespace app {
namespace nxe {

enum class NxeSound {
  kFocus,
  kSelect,
  kBack,
  kPanelLeft,
  kPanelRight,
  kPanelUnfold,
  kChannelUp,
  kChannelDown,
};

class NxeAudio {
 public:
  static NxeAudio& Instance();

  void Initialize(const std::filesystem::path& storage_root);
  void Shutdown();

  void SetEnabled(bool enabled) { enabled_ = enabled; }
  void SetSoundVolume(float volume) { sound_volume_ = volume; }
  void SetBgmVolume(float volume) { bgm_volume_ = volume; }

  void Play(NxeSound sound);

 private:
  NxeAudio() = default;
  ~NxeAudio() { Shutdown(); }

  bool LoadWav(const std::filesystem::path& path, std::vector<uint8_t>& out,
               uint32_t& sample_rate, uint16_t& channels);

  bool initialized_ = false;
  bool enabled_ = true;
  float sound_volume_ = 1.f;
  float bgm_volume_ = 0.5f;
  std::mutex mu_;
  std::map<NxeSound, std::vector<uint8_t>> samples_;
  std::map<NxeSound, uint32_t> sample_rates_;
  std::map<NxeSound, uint16_t> sample_channels_;
};

}  // namespace nxe
}  // namespace app
}  // namespace xe

#endif  // XENIA_APP_NXE_NXE_AUDIO_H_
