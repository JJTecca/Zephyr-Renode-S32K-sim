/*****************************************************************************
* File:        telemetry.h
* Description: Frozen on-wire telemetry schema (ADR-007), shared by all nodes.
* Layer:       firmware/common  (shared contract, no Zephyr dependencies)
* Project:     Zephyr-Renode-S32K-sim -- SDV Fault-Prediction & Self-Healing
* Copyright (c) 2026 Maior Cristian-Alexandru
*****************************************************************************/

#ifndef SDV_TELEMETRY_H
#define SDV_TELEMETRY_H

/***************************************************
* INCLUDE FILES
***************************************************/
#include <stdint.h>

/***************************************************
* MACRO DEFINITIONS
***************************************************/
/* CAN arbitration IDs: 0x100 | node_id */
#define SDV_CAN_BASE_ID  0x100

/* Written into sdv_fault_ctl.magic by Renode to arm a fault (virtual time). */
#define SDV_FAULT_MAGIC  0xFA17C0DEU

/***************************************************
* ENUMERATIONS
***************************************************/
/* Node IDs */
enum sdv_node_id {
    SDV_NODE_POWERTRAIN = 1,
    SDV_NODE_CHASSIS    = 2,
    SDV_NODE_BODY       = 3,
    SDV_NODE_ACOUSTIC   = 4,
    SDV_NODE_HUB        = 0x10,
};

/* Signal IDs */
enum sdv_signal {
    SIG_HEAP_FREE    = 1,
    SIG_HEAP_USED    = 2,
    SIG_CAN_TX_ERR   = 3,
    SIG_CAN_RX_ERR   = 4,
    SIG_LOOP_LATENCY = 5,
    SIG_ACTION_TAKEN = 6,
    SIG_HEARTBEAT    = 0xFE,
};

/***************************************************
* TYPE DEFINITIONS
***************************************************/
/*
 * 8-byte packed telemetry CAN frame (ADR-007).
 * Timestamp added at RX by the hub; label + true_ttf added host-side
 * by run_campaign.py.
 */
struct __attribute__((packed)) sdv_telem_frame {
    uint8_t  node;
    uint8_t  signal;
    uint16_t seq;
    uint32_t value;
};

/*
 * Fault-control block -- lives at a fixed symbol so Renode's fault_hooks.py
 * can locate it with GetSymbolAddress and write fields in virtual time.
 */
struct sdv_fault_ctl {
    uint32_t magic;               /* SDV_FAULT_MAGIC when armed            */
    uint32_t leak_bytes_per_tick; /* memory-leak injection rate            */
    uint32_t busy_spin_us;        /* timing/deadline-miss injection        */
};

#endif /* SDV_TELEMETRY_H */
