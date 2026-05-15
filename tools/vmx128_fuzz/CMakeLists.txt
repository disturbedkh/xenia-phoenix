project(vmx128-fuzz CXX)
cmake_minimum_required(VERSION 3.20)

add_executable(vmx128-fuzz
  main.cc
)

target_link_libraries(vmx128-fuzz
  xenia-cpu
  xenia-base
)

target_include_directories(vmx128-fuzz PRIVATE
  ${PROJECT_SOURCE_DIR}/../../
)

set_target_properties(vmx128-fuzz PROPERTIES
  FOLDER "tools"
)
