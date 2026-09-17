/*****************************************************************************
* File:        main.c
* Description: Minimal board bring-up sanity app -- console + heartbeat only.
*              No CAN, no heap, no floats: isolates board port from firmware.
* Copyright (c) 2026 Maior Cristian-Alexandru
*****************************************************************************/

#include <zephyr/kernel.h>

int main(void)
{
	int i = 0;
	printk("BOARD_CHECK,boot,s32k144evb\n");
	while (1) {
		printk("BOARD_CHECK,alive,%d\n", i++);
		k_msleep(500);
	}
	return 0;
}
