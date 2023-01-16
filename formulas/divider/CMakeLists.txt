CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
project(divider)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++20")

file(  
  GLOB_RECURSE
  SRC
  src/*.cc
  )

set(SENSOR_LIBRARIES pfm)
add_library (divider SHARED ${SRC})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/)
target_link_libraries (divider vjoule_common)

set_target_properties(divider PROPERTIES PREFIX "")

install (TARGETS divider DESTINATION /etc/vjoule/plugins/)


