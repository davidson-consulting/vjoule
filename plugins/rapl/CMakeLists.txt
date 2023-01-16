CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
project(rapl)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++20")

file(  
  GLOB_RECURSE
  SRC
  src/*.cc
  )

set(SENSOR_LIBRARIES pfm)
add_library (rapl SHARED ${SRC})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/)
target_link_libraries (rapl vjoule_common)

set_target_properties(rapl PROPERTIES PREFIX "")

install (TARGETS rapl DESTINATION /etc/vjoule/plugins/)


