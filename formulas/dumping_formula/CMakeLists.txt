CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
project(vjoule_dumping_formula)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++20 -static")

file(  
  GLOB_RECURSE
  SRC
  src/*.cc
  )

add_executable (vjoule_dumping_formula ${SRC})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/)
target_link_libraries(vjoule_dumping_formula LINK_PUBLIC vjoule_common)

install (TARGETS vjoule_dumping_formula DESTINATION /usr/bin/)
install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/res/config.toml DESTINATION /etc/vjoule/dumping_formula/)
install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/res/vjoule_dumping_formula.service DESTINATION /etc/systemd/system)



