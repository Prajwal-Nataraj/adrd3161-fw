/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CIA402_H_
#define CIA402_H_

#include <zephyr/kernel.h>

enum cia402_state {
    CIA402_STATE_NOT_READY_TO_SWITCH_ON, // Low power, disabled drive, brake enabled if exists. Run self-test + init
    CIA402_STATE_SWITCH_ON_DISABLED,     // Idle powered-down state
    CIA402_STATE_READY_TO_SWITCH_ON,     // Intermediary state during motor power-up. Maybe high power (depends on application), drive disabled
    CIA402_STATE_SWITCHED_ON,            // Intermediary state during motor power-up. High power, drive disabled
    CIA402_STATE_OPERATION_ENABLED,      // Motor powered on, drive enabled
    CIA402_STATE_QUICK_STOP_ACTIVE,      // Quick stop active
    CIA402_STATE_FAULT_REACTION_ACTIVE,  // Stopping because of a fault condition
    CIA402_STATE_FAULT                   // Final fault state. Needs explicit action to return to the rest of the state machine
};

enum cia402_mode_of_operation {
    CIA402_MODE_INACTIVE = 0,
    CIA402_MODE_PROFILE_POSITION = 1,
    CIA402_MODE_PROFILE_VELOCITY = 3,
    CIA402_MODE_PROFILE_TORQUE = 4,
    CIA402_MODE_CYCLIC_SYNCHRONOUS_POSITION = 8,
    CIA402_MODE_CYCLIC_SYNCHRONOUS_VELOCITY = 9,
    CIA402_MODE_CYCLIC_SYNCHRONOUS_TORQUE = 10,
};

struct cia402_priv;

struct cia402 {
    struct cia402_priv *priv;

    enum cia402_state state;

    // Pointers to OD storage
    uint16_t *controlword; // 6040 ControlWord
    uint16_t *statusword;  // 6041 StatusWord
    uint8_t  *mop;         // 6060 ModesOfOperation
    uint8_t  *mop_display; // 6061 ModesOfOperationDisplay
    int32_t  *target_pos;  // 607A TargetPosition
    int32_t  *target_vel;  // 60FF TargetVelocity
    int16_t  *target_tor;  // 6071 TargetTorque
    int32_t  *current_pos; // 6063 PositionActualValue
};

// Initialize CiA 402 state machine 
void cia402_init(struct cia402* cia402);

// Enter fault and trigger application specific motor stop logic
void cia402_enter_fault(struct cia402 *cia402);

// Step CiA 402 state machine once
void cia402_tick(struct cia402 *cia402, bool quick);

#endif /* CIA402_H_ */
