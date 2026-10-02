cmake_minimum_required(VERSION 3.20.0)

# Per-board selector (both target s32k344mini): -DNODE=frdma (FRDM-A, flexcan0,
# LPUART6) or -DNODE=tbox (T-box, flexcan3/CAN3, semihosting).
if(NOT DEFINED NODE)
  set(NODE "frdma")
endif()
set(EXTRA_DTC_OVERLAY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${NODE}.overlay")
set(EXTRA_CONF_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${NODE}.conf")

# Role: -DROLE_EDGE=1 sender, -DROLE_EDGE=0 receiver.
if(NOT DEFINED ROLE_EDGE)
  set(ROLE_EDGE 1)
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(k3_silicon)

target_sources(app PRIVATE src/main.c)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../common)
target_compile_definitions(app PRIVATE ROLE_EDGE=${ROLE_EDGE})
