CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
project(vjoule)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=c++20")

file(  
  GLOB_RECURSE
  SRC
  src/*.cc
  )

add_executable (vjoule ${SRC})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/)
target_include_directories (vjoule PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../sensor/src>)
target_link_libraries (vjoule LINK_PUBLIC vjoule_common vjoule_sensor_lib ${CMAKE_BINARY_DIR}/patch_libpfm/libpfm_patched.a cgroup)
  
install (TARGETS vjoule DESTINATION /usr/bin/)
