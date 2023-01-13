CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
project(vjoule_cxx_puller)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=c++20")

file(  
  GLOB_RECURSE
  SRC
  src/*.cc
  )

add_executable (vjoule_cxx_puller ${SRC})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/)
include_directories(/usr/local/include/python3.7)
