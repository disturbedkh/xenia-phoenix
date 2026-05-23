# Tier 0 tools (vmx128-fuzz, etc.) — requires XENIA_BUILD_MISC=ON at configure time.

if(NOT XENIA_BUILD_MISC)
  return()
endif()

add_executable(vmx128-fuzz
  ${CMAKE_CURRENT_SOURCE_DIR}/vmx128_fuzz/main.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/vmx128_fuzz/fuzz_registry.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/vmx128_fuzz/fam_integer_addsub.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/vmx128_fuzz/fam_logical_minmax_avg.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/vmx128_fuzz/fam_compare_select.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/vmx128_fuzz/fam_shifts_rotates.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/vmx128_fuzz/fam_perm_splat_merge.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/vmx128_fuzz/fam_pack_unpack.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/vmx128_fuzz/fam_mul_sum.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/vmx128_fuzz/fam_sums.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/vmx128_fuzz/fam_float_converts.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/vmx128_fuzz/fam_float_binary_vm128.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/vmx128_fuzz/vec128_fuzz_ref.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/vmx128_fuzz/vmx128_pin_table.cc
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

add_executable(xma2-diff
  ${CMAKE_CURRENT_SOURCE_DIR}/xma2_diff/main.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/xma2_diff/xma2_test_harness.cc
)
if(WIN32)
  target_sources(xma2-diff PRIVATE
    ${CMAKE_SOURCE_DIR}/src/xenia/base/console_app_main_win.cc)
else()
  target_sources(xma2-diff PRIVATE
    ${CMAKE_SOURCE_DIR}/src/xenia/base/console_app_main_posix.cc)
endif()
target_link_libraries(xma2-diff PRIVATE
  libavcodec libavutil
  xenia-apu xenia-base xenia-core xenia-cpu
  $<$<BOOL:${XE_TARGET_X86_64}>:xenia-cpu-backend-x64>
  $<$<BOOL:${XE_TARGET_AARCH64}>:xenia-cpu-backend-a64>
)
target_include_directories(xma2-diff PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/xma2_diff
  ${PROJECT_SOURCE_DIR}/third_party/FFmpeg
  ${PROJECT_SOURCE_DIR}/third_party/ffmpeg-xenia
)
xe_target_defaults(xma2-diff)
set_target_properties(xma2-diff PROPERTIES FOLDER "tools")
