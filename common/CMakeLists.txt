CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
project(anon_common)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=c++17 -static -fPIC")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -static")

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

add_library (anon_common ${SRC} ${SRC_C})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/)
target_include_directories (anon_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/)







