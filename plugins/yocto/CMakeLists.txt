CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
project(yocto)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++17 -O3")

file(
  GLOB_RECURSE
  SRC
  src/*.cc
  )

set(SENSOR_LIBRARIES pfm)
add_library (yocto SHARED ${SRC})

add_dependencies (yocto vjoule_build_yocto)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/)
target_link_libraries (yocto vjoule_common ${CMAKE_BINARY_DIR}/yocto_build/libyocto-static.a usb-1.0)

set_target_properties(yocto PROPERTIES PREFIX "")

install (TARGETS yocto DESTINATION /etc/vjoule/plugins/)
