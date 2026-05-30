/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/app/settings_ui_registry.h"

#include <algorithm>

namespace xe {
namespace app {

const char* PreferencesTabLabel(PreferencesTab tab) {
  switch (tab) {
    case PreferencesTab::kGraphics:
      return "Graphics";
    case PreferencesTab::kVideoDisplay:
      return "Video & Display";
    case PreferencesTab::kAudio:
      return "Audio";
    case PreferencesTab::kInput:
      return "Input";
    case PreferencesTab::kStorage:
      return "Storage & Paths";
    case PreferencesTab::kGeneral:
      return "General";
    case PreferencesTab::kCpuSystem:
      return "CPU & System";
    case PreferencesTab::kLogging:
      return "Logging";
    case PreferencesTab::kAdvanced:
      return "Advanced";
    case PreferencesTab::kGameOverrides:
      return "Game Overrides";
    case PreferencesTab::kXboxConsole:
      return "Xbox Console";
    default:
      return "Settings";
  }
}

std::vector<std::string_view> CategoriesForTab(PreferencesTab tab) {
  switch (tab) {
    case PreferencesTab::kGraphics:
      return {"GPU"};
    case PreferencesTab::kVideoDisplay:
      return {"Display", "Video"};
    case PreferencesTab::kAudio:
      return {"APU"};
    case PreferencesTab::kInput:
      return {"HID"};
    case PreferencesTab::kStorage:
      return {"Storage"};
    case PreferencesTab::kGeneral:
      return {"General", "UI", "Profiles"};
    case PreferencesTab::kCpuSystem:
      return {"CPU", "Kernel", "x64"};
    case PreferencesTab::kLogging:
      return {"Logging", "Debug"};
    case PreferencesTab::kAdvanced:
      return {"D3D12", "Vulkan", "Memory", "Win32", "HACKS", "GPU"};
    default:
      return {};
  }
}

bool IsCuratedCvar(std::string_view name) {
  static constexpr const char* kCurated[] = {
      "draw_resolution_scale_x",
      "draw_resolution_scale_y",
      "gpu",
      "vsync",
      "framerate_limit",
      "anisotropic_override",
      "native_2x_msaa",
      "async_shader_compilation",
      "store_shaders",
      "readback_resolve",
      "draw_resolution_scaled_texture_offsets",
      "postprocess_antialiasing",
      "postprocess_scaling_and_sharpening",
      "postprocess_ffx_cas_additional_sharpness",
      "postprocess_ffx_fsr_sharpness_reduction",
      "postprocess_dither",
      "fullscreen",
      "apu",
      "hid",
      "discord",
      "mount_scratch",
      "mount_cache",
      "controller_hotkeys",
  };
  for (const char* curated : kCurated) {
    if (name == curated) {
      return true;
    }
  }
  return false;
}

bool IsAdvancedOnlyCvar(std::string_view name) {
  if (name == "defaults_date") {
    return true;
  }
  auto starts_with = [](std::string_view haystack, std::string_view prefix) {
    return haystack.size() >= prefix.size() &&
           haystack.substr(0, prefix.size()) == prefix;
  };
  auto contains = [](std::string_view haystack, std::string_view needle) {
    return haystack.find(needle) != std::string_view::npos;
  };
  if (starts_with(name, "trace_") || starts_with(name, "dump_")) {
    return true;
  }
  if (starts_with(name, "query_occlusion_")) {
    return true;
  }
  if (name == "gpu_allow_invalid_fetch_constants" ||
      name == "gpu_allow_invalid_upload_range") {
    return true;
  }
  if (contains(name, "breakpoint") || contains(name, "disasm")) {
    return true;
  }
  return false;
}

bool RequiresRestart(std::string_view name) {
  return name == "gpu" || name == "draw_resolution_scale_x" ||
         name == "draw_resolution_scale_y";
}

}  // namespace app
}  // namespace xe
