# Phoenix localhost probe (not the ImGui debug UI under debug/ui/).

add_library(xenia-phoenix-probe STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/phoenix_flags.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/phoenix_probe.cc
)
target_include_directories(xenia-phoenix-probe PUBLIC
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/third_party
)
target_link_libraries(xenia-phoenix-probe PUBLIC xenia-base xenia-obs)
if(WIN32)
  target_link_libraries(xenia-phoenix-probe PRIVATE ws2_32)
endif()
xe_target_defaults(xenia-phoenix-probe)

if(NOT ANDROID)
  add_subdirectory(ui)
endif()
