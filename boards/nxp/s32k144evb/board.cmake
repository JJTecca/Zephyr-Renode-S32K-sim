# Copyright (c) 2026 Maior Cristian-Alexandru
# SPDX-License-Identifier: Apache-2.0

# OpenSDA on the EVB is PEmicro; `west flash` runners don't see it.
# Flash the .hex via S32 Design Studio (PEmicro), or reflash OpenSDA with
# SEGGER J-Link firmware and use `west flash -r jlink`.
board_runner_args(jlink "--device=S32K144" "--speed=4000" "--reset")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
