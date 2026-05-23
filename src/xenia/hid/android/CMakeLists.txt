add_library(xenia-hid-android STATIC)
xe_platform_sources(xenia-hid-android ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(xenia-hid-android PUBLIC xenia-base xenia-hid xenia-ui android log)
xe_target_defaults(xenia-hid-android)
