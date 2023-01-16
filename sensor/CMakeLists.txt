CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
project(vjoule_sensor)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++20")

file(  
  GLOB_RECURSE
  SRC
  src/*.cc
  )

set(SENSOR_LIBRARIES pfm)
add_executable (vjoule_sensor ${SRC})

add_dependencies (vjoule_sensor vjoule_patch_libpfm)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/)
target_link_libraries (vjoule_sensor vjoule_common ${CMAKE_BINARY_DIR}/patch_libpfm/libpfm_patched.a pthread)
  
install (TARGETS vjoule_sensor DESTINATION /usr/bin/)
install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/res/config.toml DESTINATION /etc/vjoule/)
install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/res/cgroups DESTINATION /etc/vjoule/)

install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/res/vjoule_sensor.service DESTINATION /etc/systemd/system)


