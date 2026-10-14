/*****************************************************************************
* File:        config.h
* Description: Cross-platform tuning shared by k3_hub (Renode) and k3_silicon.
* Layer:       firmware/common
* Project:     Zephyr-Renode-S32K-sim -- SDV Fault-Prediction & Self-Healing
* Copyright (c) 2026 Maior Cristian-Alexandru
*****************************************************************************/

#ifndef SDV_CONFIG_H
#define SDV_CONFIG_H

/***************************************************
* MACRO DEFINITIONS
***************************************************/
#define AE_WIN      10
#define AE_DT_S     0.1f
#define AE_W1       (AE_WIN + 1)
#define TTF_GUARD_S 3.0f

#endif /* SDV_CONFIG_H */
