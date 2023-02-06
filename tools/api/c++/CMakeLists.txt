CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
project(vjoule)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=c++20")

file(  
  GLOB_RECURSE
  SRC
  src/*.cc
  )

file(  
  GLOB
  HEADERS
  src/*.hh
  )

add_library (vjoule_api ${SRC})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/)
target_link_libraries (vjoule_api LINK_PUBLIC vjoule_common)

install (TARGETS vjoule_api DESTINATION /usr/lib/)
install (FILES ${HEADERS} DESTINATION /usr/include/vjoule/)
