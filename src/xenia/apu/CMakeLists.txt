add_library(xenia-apu STATIC)
xe_platform_sources(xenia-apu ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(xenia-apu PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/util/apu_trace.cc
)
# sha256 implementation is compiled once via kernel (xboxkrnl_crypt.cc includes sha256.cpp).
target_include_directories(xenia-apu PRIVATE
  ${PROJECT_SOURCE_DIR}/third_party/FFmpeg
  ${PROJECT_SOURCE_DIR}/third_party/ffmpeg-xenia
)
target_link_libraries(xenia-apu PUBLIC libavcodec libavutil libavformat xenia-base xenia-obs
  xenia-phoenix-probe)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries(xenia-apu PRIVATE SDL2)
endif()
xe_target_defaults(xenia-apu)
