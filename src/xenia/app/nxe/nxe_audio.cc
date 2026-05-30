/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/app/nxe/nxe_audio.h"

#include <cstring>
#include <fstream>

#include "xenia/app/nxe/nxe_assets.h"
#include "xenia/base/logging.h"
#include "xenia/base/platform.h"

#if XE_PLATFORM_WIN32
#include <Windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#endif

namespace xe {
namespace app {
namespace nxe {

namespace {

const char* SoundFile(NxeSound sound) {
  switch (sound) {
    case NxeSound::kFocus:
      return "focus.wav";
    case NxeSound::kSelect:
      return "select.wav";
    case NxeSound::kBack:
      return "back.wav";
    case NxeSound::kPanelLeft:
      return "panel-left.wav";
    case NxeSound::kPanelRight:
      return "panel-right.wav";
    case NxeSound::kPanelUnfold:
      return "panel-unfold.wav";
    case NxeSound::kChannelUp:
      return "channel-up.wav";
    case NxeSound::kChannelDown:
      return "channel-down.wav";
  }
  return "focus.wav";
}

}  // namespace

NxeAudio& NxeAudio::Instance() {
  static NxeAudio instance;
  return instance;
}

bool NxeAudio::LoadWav(const std::filesystem::path& path,
                       std::vector<uint8_t>& out, uint32_t& sample_rate,
                       uint16_t& channels) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    return false;
  }
  out.assign(std::istreambuf_iterator<char>(in),
             std::istreambuf_iterator<char>());
  if (out.size() < 44) {
    return false;
  }
  sample_rate = *reinterpret_cast<const uint32_t*>(&out[24]);
  channels = *reinterpret_cast<const uint16_t*>(&out[22]);
  return true;
}

void NxeAudio::Initialize(const std::filesystem::path& storage_root) {
  if (initialized_) {
    return;
  }
  NxeAssets::Instance().Initialize(storage_root);
  const NxeSound sounds[] = {
      NxeSound::kFocus,     NxeSound::kSelect,      NxeSound::kBack,
      NxeSound::kPanelLeft, NxeSound::kPanelRight,  NxeSound::kPanelUnfold,
      NxeSound::kChannelUp, NxeSound::kChannelDown,
  };
  for (NxeSound s : sounds) {
    const auto path = NxeAssets::Instance().Audio(SoundFile(s));
    std::vector<uint8_t> data;
    uint32_t rate = 0;
    uint16_t ch = 0;
    if (LoadWav(path, data, rate, ch)) {
      samples_[s] = std::move(data);
      sample_rates_[s] = rate;
      sample_channels_[s] = ch;
    }
  }
  initialized_ = true;
}

void NxeAudio::Shutdown() {
  samples_.clear();
  initialized_ = false;
}

void NxeAudio::Play(NxeSound sound) {
  if (!enabled_ || !initialized_) {
    return;
  }
  std::lock_guard lock(mu_);
  auto it = samples_.find(sound);
  if (it == samples_.end() || it->second.empty()) {
    return;
  }
#if XE_PLATFORM_WIN32
  PlaySoundA(reinterpret_cast<LPCSTR>(it->second.data()), nullptr,
             SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
#else
  (void)sound;
#endif
}

}  // namespace nxe
}  // namespace app
}  // namespace xe
