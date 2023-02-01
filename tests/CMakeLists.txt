CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
project(tests)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=c++20")

file(  
  GLOB_RECURSE
  SRC
  src/*.cc
  )
add_executable (tests ${SRC})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/)
target_include_directories (tests PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../service/src>)
target_link_libraries (tests LINK_PUBLIC vjoule_common vjoule_service_lib ${CMAKE_BINARY_DIR}/patch_libpfm/libpfm_patched.a cgroup)

add_custom_target(test COMMAND tests ${CMAKE_CURRENT_SOURCE_DIR}/res)
