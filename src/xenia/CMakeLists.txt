# src/xenia/CMakeLists.txt - Master file for all xenia modules.

# xenia-core (this directory's own sources)
add_library(xenia-core STATIC)
file(GLOB _core_sources   "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)
target_sources(xenia-core PRIVATE ${_core_sources})
target_link_libraries(xenia-core PUBLIC fmt xenia-base xenia-obs)
xe_target_defaults(xenia-core)

# All subdirectories
add_subdirectory(base)
add_subdirectory(cpu)
if(XE_TARGET_X86_64)
  add_subdirectory(cpu/backend/x64)
endif()
if(XE_TARGET_AARCH64)
  add_subdirectory(cpu/backend/a64)
endif()
# Phoenix localhost probe builds everywhere; the ImGui debug UI under debug/ui
# is excluded on Android inside debug/CMakeLists.txt.
add_subdirectory(debug)
add_subdirectory(apu)
add_subdirectory(apu/nop)
add_subdirectory(gpu)
add_subdirectory(gpu/null)
add_subdirectory(gpu/vulkan)
add_subdirectory(hid)
add_subdirectory(hid/nop)
add_subdirectory(hid/portal)
add_subdirectory(kernel)
add_subdirectory(patcher)
add_subdirectory(ui)
add_subdirectory(ui/vulkan)
add_subdirectory(vfs)
# Discord presence: real discord-rpc backend on desktop, no-op stub on Android
# (discord-rpc + FFmpeg base64 are desktop-only). Built everywhere so the
# kernel/app link references resolve.
add_subdirectory(app/discord)
add_subdirectory(app)

# SDL-based modules (desktop Linux / Windows only — not Android)
if(NOT ANDROID)
  add_subdirectory(apu/sdl)
  add_subdirectory(helper/sdl)
  add_subdirectory(hid/sdl)
endif()

if(ANDROID)
  add_subdirectory(hid/android)
  add_subdirectory(apu/android)
endif()

# Platform-conditional modules
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  add_subdirectory(apu/alsa)
endif()

if(WIN32)
  add_subdirectory(apu/xaudio2)
  add_subdirectory(gpu/d3d12)
  add_subdirectory(hid/winkey)
  add_subdirectory(hid/xinput)
  add_subdirectory(ui/d3d12)
endif()
