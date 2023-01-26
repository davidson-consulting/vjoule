CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
project(vjoule_service)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=c++20")

file(  
  GLOB_RECURSE
  SRC
  src/*.cc
  )

set(SERVICE_LIBRARIES pfm)
add_executable (vjoule_service ${SRC})

add_dependencies (vjoule_service vjoule_patch_libpfm)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/)
target_link_libraries (vjoule_service vjoule_common ${CMAKE_BINARY_DIR}/patch_libpfm/libpfm_patched.a)
  
install (TARGETS vjoule_service DESTINATION /usr/bin/)
install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/res/config.toml DESTINATION /etc/vjoule/)
install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/res/cgroups DESTINATION /etc/vjoule/)

install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/res/vjoule_service.service DESTINATION /etc/systemd/system)


