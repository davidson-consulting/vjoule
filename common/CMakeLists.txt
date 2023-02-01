CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
project(vjoule_common)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++20 -static -fPIC")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -static")

file(  
  GLOB_RECURSE
  SRC
  src/*.cc
  )

file(  
  GLOB_RECURSE
  SRC_C
  src/*.c
  )

add_library (vjoule_common ${SRC} ${SRC_C})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/)
target_include_directories (vjoule_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/)







