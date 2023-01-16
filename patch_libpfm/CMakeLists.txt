CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
project(vjoule_patch_libpfm)

set_directory_properties(PROPERTIES
  ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_CURRENT_SOURCE_DIR}/libpfm_patched.a
  )

add_custom_target(vjoule_patch_libpfm ALL
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/build.sh 
  #COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/libpfm_patched.a ${CMAKE_CURRENT_BINARY_DIR}/libpfm_patched.a
)
