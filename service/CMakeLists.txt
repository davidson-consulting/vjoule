CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
project(anon_service)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=c++17")

file(  
  GLOB_RECURSE
  SRC
  src/*.cc
  )

file(
  GLOB_RECURSE
  SRC_NO_MAIN
  src/*.cc
  )
list(REMOVE_ITEM SRC_NO_MAIN src/main.cc)

set(SERVICE_LIBRARIES pfm)
add_executable (anon_service ${SRC})
add_library (anon_service_lib ${SRC_NO_MAIN})

add_dependencies (anon_service anon_patch_libpfm)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/)
target_link_libraries (anon_service anon_common ${CMAKE_BINARY_DIR}/patch_libpfm/libpfm_patched.a)
target_link_libraries (anon_service_lib anon_common ${CMAKE_BINARY_DIR}/patch_libpfm/libpfm_patched.a)
 
install (TARGETS anon_service DESTINATION /usr/bin/)
install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/res/config.toml DESTINATION /etc/anon/)
install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/res/cgroups DESTINATION /etc/anon/)

install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/res/anon_service.service DESTINATION /etc/systemd/system)


