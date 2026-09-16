# Copyright (c) 2026 Maior Cristian-Alexandru
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=S32K344" "--reset-after-load")
board_runner_args(pyocd "--target=s32k344")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
