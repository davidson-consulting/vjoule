CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
project(vjoule_cgutils)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=c++17")

file(  
  GLOB_RECURSE
  SRC
  src/*.cc
  )

add_executable (vjoule_cgutils ${SRC})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/)
target_link_libraries (vjoule_cgutils LINK_PUBLIC vjoule_common)


add_custom_command (TARGET vjoule_cgutils POST_BUILD
  COMMAND sudo setcap "cap_dac_override,cap_sys_rawio+eip" ${CMAKE_CURRENT_BINARY_DIR}/vjoule_cgutils
  )

install (TARGETS vjoule_cgutils DESTINATION /usr/bin/)
