# Tier 0 tools (vmx128-fuzz, etc.) — requires XENIA_BUILD_MISC=ON at configure time.

if(NOT XENIA_BUILD_MISC)
  return()
endif()

add_executable(vmx128-fuzz
  ${CMAKE_CURRENT_SOURCE_DIR}/vmx128_fuzz/main.cc
  ${CMAKE_SOURCE_DIR}/src/xenia/cpu/testing/guest_ppc_test_util.cc
)
if(WIN32)
  target_sources(vmx128-fuzz PRIVATE
    ${CMAKE_SOURCE_DIR}/src/xenia/base/console_app_main_win.cc)
else()
  target_sources(vmx128-fuzz PRIVATE
    ${CMAKE_SOURCE_DIR}/src/xenia/base/console_app_main_posix.cc)
endif()

target_link_libraries(vmx128-fuzz PRIVATE
  capstone fmt imgui
  xenia-base xenia-core xenia-cpu xenia-gpu
  xenia-kernel xenia-ui xenia-patcher
  $<$<BOOL:${XE_TARGET_X86_64}>:xenia-cpu-backend-x64>
  $<$<BOOL:${XE_TARGET_AARCH64}>:xenia-cpu-backend-a64>
)
xe_target_defaults(vmx128-fuzz)
set_target_properties(vmx128-fuzz PROPERTIES FOLDER "tools")
