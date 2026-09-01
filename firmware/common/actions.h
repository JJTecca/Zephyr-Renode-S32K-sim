/*****************************************************************************
* File:        actions.h
* Description: Actions taken based on fault received edge nodes
* Layer:       firmware/common
* Project:     Zephyr-Renode-S32K-sim -- SDV Fault-Prediction & Self-Healing
* Copyright (c) 2026 Maior Cristian-Alexandru
*****************************************************************************/

#ifndef SDV_ACTIONS_H
#define SDV_ACTIONS_H

/***************************************************
* INCLUDE FILES
***************************************************/
#include <stdint.h>

/***************************************************
* MACRO DEFINITIONS
***************************************************/
#define DEADBEEF 0xDEADBEEF

/***************************************************
* ENUMERATIONS
***************************************************/
enum sdv_actions_id {
    SDV_RESTART = 1,
    SDV_DEGRADED_MODE = 2,
    SDV_LOAD_SHED = 3,
};