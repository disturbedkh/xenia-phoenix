add_library(xenia-obs STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/obs.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/obs_preset.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/obs_event.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/obs_jsonl_sink.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/obs_channel.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/obs_invariant.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/obs_trace_sidecar.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/obs_guest_ring.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/obs_pm4_bridge.cc
)
target_include_directories(xenia-obs PUBLIC
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/third_party
)
target_link_libraries(xenia-obs PUBLIC fmt xenia-base)
xe_target_defaults(xenia-obs)

if(XENIA_BUILD_TESTS)
  set(CMAKE_FOLDER "tests")
  add_executable(xenia-obs-preset-test obs_preset_test.cc)
  target_link_libraries(xenia-obs-preset-test PRIVATE xenia-obs xenia-base fmt)
  xe_target_defaults(xenia-obs-preset-test)
  add_test(NAME xenia-obs-preset-test COMMAND xenia-obs-preset-test)
endif()
