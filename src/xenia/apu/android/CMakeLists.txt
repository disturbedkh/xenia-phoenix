add_library(xenia-apu-android STATIC)
xe_platform_sources(xenia-apu-android ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(xenia-apu-android PUBLIC xenia-base xenia-apu aaudio)
xe_target_defaults(xenia-apu-android)
