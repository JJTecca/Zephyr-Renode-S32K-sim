# Standalone host build for the Observer design-pattern demo (v0).
# Not part of the Zephyr `west` firmware build -- invoked by scripts/run_all.ps1.
cmake_minimum_required(VERSION 3.20)
project(sdv_observer_demo CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# design_pattern.c holds C++ classes; force the C++ compiler despite the .c name.
set_source_files_properties(design_pattern.c PROPERTIES LANGUAGE CXX)

add_executable(observer_demo design_pattern.c)
target_include_directories(observer_demo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../common)
