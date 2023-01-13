cmake_minimum_required (VERSION 3.1)
project (vjoule)

add_subdirectory (patch_libpfm)
add_subdirectory (common)
add_subdirectory (sensors/std_sensor)
add_subdirectory (formulas/simple_formula)
add_subdirectory (formulas/dumping_formula)
add_subdirectory (tools/vjoule)
