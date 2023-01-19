CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
project(dumper)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++20 -O3")

file(  
  GLOB_RECURSE
  SRC
  src/*.cc
  )

set(SENSOR_LIBRARIES pfm)
add_library (dumper SHARED ${SRC})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/)
target_link_libraries (dumper vjoule_common)

set_target_properties(dumper PROPERTIES PREFIX "")

install (TARGETS dumper DESTINATION /etc/vjoule/plugins/)


