CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
project(vjoule)


# --- Fetch FTXUI --------------------------------------------------------------

include(FetchContent)

set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)
FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
  GIT_TAG v3.0.0
)

FetchContent_GetProperties(ftxui)
if(NOT ftxui_POPULATED)
  FetchContent_Populate(ftxui)
  add_subdirectory(${ftxui_SOURCE_DIR} ${ftxui_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

# ------------------------------------------------------------------------------


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=c++17")

file(  
  GLOB_RECURSE
  SRC
  src/*.cc
  )

add_executable (vjoule ${SRC})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/)
target_include_directories (vjoule PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../service/src>)
target_link_libraries (vjoule LINK_PUBLIC vjoule_common vjoule_service_lib ${CMAKE_BINARY_DIR}/patch_libpfm/libpfm_patched.a)

target_link_libraries (vjoule PRIVATE ftxui::screen PRIVATE ftxui::dom PRIVATE ftxui::component)

add_custom_command (TARGET vjoule POST_BUILD
  COMMAND sudo setcap "cap_dac_override,cap_sys_admin,cap_sys_rawio+eip" ${CMAKE_CURRENT_BINARY_DIR}/vjoule
  )

install (TARGETS vjoule DESTINATION /usr/bin/)
install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/res/_vjoule DESTINATION /etc/bash_completion.d/)
