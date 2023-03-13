CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
project(simple)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++20 -g")

file(
  GLOB_RECURSE
  SRC
  src/*.cc
  )

add_library (simple SHARED ${SRC})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/)
target_link_libraries (simple vjoule_common)

set_target_properties(simple PROPERTIES PREFIX "")

install (TARGETS simple DESTINATION /etc/vjoule/plugins/)
