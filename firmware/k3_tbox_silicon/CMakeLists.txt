cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(k3_tbox_silicon)

# Role select: -DROLE_EDGE=1 sender, -DROLE_EDGE=0 receiver (default sender).
if(NOT DEFINED ROLE_EDGE)
  set(ROLE_EDGE 1)
endif()

target_sources(app PRIVATE src/main.c)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../common)
target_compile_definitions(app PRIVATE ROLE_EDGE=${ROLE_EDGE})
