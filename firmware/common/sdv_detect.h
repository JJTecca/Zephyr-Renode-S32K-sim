/*****************************************************************************
* File:        sdv_detect.h
* Description: Shared detector + supervisor API (defined in sdv_detect.c),
*              compiled into both k3_hub (Renode) and k3_silicon.
* Layer:       firmware/common
* Project:     Zephyr-Renode-S32K-sim -- SDV Fault-Prediction & Self-Healing
* Copyright (c) 2026 Maior Cristian-Alexandru
*****************************************************************************/

#ifndef SDV_DETECT_H
#define SDV_DETECT_H

/***************************************************
* INCLUDE FILES
***************************************************/
#include <stdint.h>

/***************************************************
* FUNCTION PROTOTYPES
***************************************************/
extern float ae_score(const float *x);
extern int   supervisor_step(int src, int alarm, float ttf, int proposed);
extern void  detector_step(uint16_t seq, uint32_t heap_free, uint32_t loop_latency);
extern void  sdv_send_cmd(int node, int action);

#endif /* SDV_DETECT_H */
