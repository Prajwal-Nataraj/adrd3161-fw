/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <stdlib.h>

#include "cia402.h"
#include "cia402_impl.h"

LOG_MODULE_REGISTER(cia402, LOG_LEVEL_INF);

enum cia402_statusword_bit {
    CIA402_SW_READYTOSWITCHON = 1 << 0,  // Ready to switch on
    CIA402_SW_SWITCHON = 1 << 1,         // Switched on (voltage applied to drive)
    CIA402_SW_OPERATIONEN = 1 << 2,      // Operation Enabled (+ quick stop / fault reaction)
    CIA402_SW_FAULT = 1 << 3,
    CIA402_SW_VOLTAGEEN = 1 << 4,        // Voltage is applied to drive
    CIA402_SW_QUICKSTOP = 1 << 5,        // 
    CIA402_SW_SWITCHONDISABLED = 1 << 6, //
    CIA402_SW_WARNING = 1 << 7,          // 
    CIA402_SW_MANUF1 = 1 << 8,           // Manufacturer-specific, unused
    CIA402_SW_REMOTE = 1 << 9,           // Set to 0 if PDOs are ignored (never??)
    CIA402_SW_TARGET = 1 << 10,          // Target reached
    CIA402_SW_LIMIT = 1 << 11,           // Internal limit active (e.g. software position limit)
    CIA402_SW_MOP1 = 1 << 12,            // Mode-of-operation specific:
    CIA402_SW_PP_ACK = CIA402_SW_MOP1,   // pp: Set-point acknowledge
    CIA402_SW_PV_SPEED = CIA402_SW_MOP1, // pv: Speed
    CIA402_SW_CS_FOL = CIA402_SW_MOP1,   // csp, csv, cst: Drive follows the command value
    CIA402_SW_MOP2 = 1 << 13,            // Mode-of-operation specific:
    CIA402_SW_PP_ERR = CIA402_SW_MOP2,   // pp: Following error
    CIA402_SW_PV_ERR = CIA402_SW_MOP2,   // pv: Max slippage error
    CIA402_SW_CS_ERR = CIA402_SW_MOP2,   // csp, csv, cst: Folowing error
    CIA402_SW_MANUF2 = 1 << 14,          // Manufacturer-specific, unused
    CIA402_SW_MANUF3 = 1 << 15,          // Manufacturer-specific, unused
};

enum cia402_controlword_bit {
    CIA402_CW_SWITCH_ON = 1 << 0,        // Switch on
    CIA402_CW_ENABLE_VOLTAGE = 1 << 1,   // Enable voltage
    CIA402_CW_QUICK_STOP = 1 << 2,       // Quick stop (0 = quick stop)
    CIA402_CW_ENABLE_OPERATION = 1 << 3, // Enable operation
    CIA402_CW_MOP1 = 1 << 4,             // Mode-of-operation specific:
    CIA402_CW_PP_NEWSP = CIA402_CW_MOP1, // pp: New set-point
    CIA402_CW_MOP2 = 1 << 5,             // Mode-of-operation specific:
    CIA402_CW_PP_IMM = CIA402_CW_MOP2,   // pp: Immediately assume set-point
    CIA402_CW_MOP3 = 1 << 6,             // Mode-of-operation specific:
    CIA402_CW_PP_REL = CIA402_CW_MOP3,   // pp: Set-point is relative
    CIA402_CW_FAULT_RESET = 1 << 7,      // Fault reset
    CIA402_CW_HALT = 1 << 8,             // Halt
};

struct cia402_priv {
    struct k_mutex lock; // ioan: What did I intend to symchronize with this lock...?

    // Profile position state
    bool pp_target_reached;
    int pp_target;
};

// CIA402_STATE_NOT_READY_TO_SWITCH_ON: low power, disabled drive, brake enabled if exists. run self-test + init
static inline void enter_not_ready_to_switch_on(struct cia402 *cia402) {
    LOG_INF("Entering state: NOT_READY_TO_SWITCH_ON");

    *cia402->statusword = 0;
    cia402_impl_statusword_tpdo(cia402);
}


// CIA402_STATE_SWITCH_ON_DISABLED: self-test + init ok. idle powered-down state
static inline void enter_switch_on_disabled(struct cia402 *cia402) {
    LOG_INF("Entering state: SWITCH_ON_DISABLED");

    cia402_impl_shutdown(cia402);

    int sw = *cia402->statusword;
    sw = (sw & ~0b1001111) | 0b1000000; // x1xx 0000
    sw &= ~CIA402_SW_VOLTAGEEN;
    // sw |= CIA402_SW_REMOTE; // ioan: is this the right place?
    *cia402->statusword = sw;
    cia402_impl_statusword_tpdo(cia402);
}


// CIA402_STATE_READY_TO_SWITCH_ON: maybe high power, drive disabled. intermediary state during motor power-up
static inline void enter_ready_to_switch_on(struct cia402 *cia402) {
    LOG_INF("Entering state: READY_TO_SWITCH_ON");
    
    int sw = *cia402->statusword;
    sw = (sw & ~0b1101111) | 0b0100001; // x01x 0001
    sw &= ~CIA402_SW_VOLTAGEEN;
    *cia402->statusword = sw;
    cia402_impl_statusword_tpdo(cia402);
}


// CIA402_STATE_SWITCHED_ON: high power, drive disabled. intermediary state during motor power-up
static inline void enter_switched_on(struct cia402 *cia402) {
    LOG_INF("Entering state: SWITCHED_ON");

    cia402_impl_start(cia402);
    
    int sw = *cia402->statusword;
    sw = (sw & ~0b1101111) | 0b0100011; // x01x 0011
    sw |= CIA402_SW_VOLTAGEEN;
    *cia402->statusword = sw;
    cia402_impl_statusword_tpdo(cia402);
}


// CIA402_STATE_OPERATION_ENABLED: high power, drive enabled, motor actively communicating and moving
static inline void enter_operation_enabled(struct cia402 *cia402) {
    LOG_INF("Entering state: OPERATION_ENABLED");
    
    int sw = *cia402->statusword;
    sw = (sw & ~0b1101111) | 0b0100111; // x01x 0111
    sw |= CIA402_SW_VOLTAGEEN;
    *cia402->statusword = sw;
    cia402_impl_statusword_tpdo(cia402);
}

static inline void run_mode_pp(struct cia402 *cia402) {
    uint16_t controlword = *cia402->controlword;
    uint16_t statusword = *cia402->statusword;

    // Parse controlword
    if(controlword & CIA402_CW_PP_NEWSP) {
        if((controlword & CIA402_CW_PP_IMM) || (cia402->priv->pp_target_reached || true)) { // target_reached doesn't work yet
            if(controlword & CIA402_CW_PP_REL) {
                // TODO: object 60F2 - relative to what
                cia402->priv->pp_target += *cia402->target_pos;
            } else {
                cia402->priv->pp_target = *cia402->target_pos;
            }

            statusword |= CIA402_SW_PP_ACK;
            *cia402->statusword = statusword;
            cia402_impl_statusword_tpdo(cia402);
        }
    } else {
        // Un-assert "ack" only after master un-asserts "new set-point"
        if(statusword & CIA402_SW_PP_ACK) {
            statusword &= ~CIA402_SW_PP_ACK;
            *cia402->statusword = statusword;
            cia402_impl_statusword_tpdo(cia402);
        }
    }

    bool halt = controlword & CIA402_CW_HALT;

    // Execute
    struct cia402_impl_pp_out out = cia402_impl_pp((struct cia402_impl_pp_in) {
        .halt = halt,
        .target_pos = cia402->priv->pp_target
    });

    // Set statusword
    statusword &= ~(CIA402_SW_PP_ERR | CIA402_SW_TARGET);
    if(out.pp_err) statusword |= CIA402_SW_PP_ERR;
    if(out.target_reached) statusword |= CIA402_SW_TARGET;
    *cia402->statusword = statusword;
}

static inline void run_mode_pv(struct cia402 *cia402) {
    uint16_t controlword = *cia402->controlword;
    uint16_t statusword = *cia402->statusword;

    // Parse controlword
    bool halt = controlword & CIA402_CW_HALT;

    // Execute
    struct cia402_impl_pv_out out = cia402_impl_pv((struct cia402_impl_pv_in) {
        .halt = halt,
        .target_vel = *cia402->target_vel
    });

    // Set statusword
    statusword &= ~(CIA402_SW_PV_SPEED | CIA402_SW_PV_ERR | CIA402_SW_TARGET);
    if(out.speed) statusword |= CIA402_SW_PV_SPEED;
    if(out.max_slip_err) statusword |= CIA402_SW_PV_ERR;    
    if(out.target_reached) statusword |= CIA402_SW_TARGET;
    *cia402->statusword = statusword;
}

static inline void run_mode_tq(struct cia402 *cia402) {
    uint16_t controlword = *cia402->controlword;
    uint16_t statusword = *cia402->statusword;

    // Parse controlword
    bool halt = controlword & CIA402_CW_HALT;

    // TODO: implement torque slope (object 6087)

    struct cia402_impl_tq_out out = cia402_impl_tq((struct cia402_impl_tq_in) {
        .halt = halt,
        .target_tor = *cia402->target_tor
    });

    statusword &= ~CIA402_SW_TARGET;
    if(out.target_reached) statusword |= CIA402_SW_TARGET;
    *cia402->statusword = statusword;
}

static inline void run_mode_csp(struct cia402 *cia402) {
    uint16_t statusword = *cia402->statusword;

    // Controlword: Cyclic synchronous modes have no mode-specific bits

    // Execute
    struct cia402_impl_csp_out out = cia402_impl_csp((struct cia402_impl_csp_in) {
        .target_pos = *cia402->target_pos
    });

    // Set statusword
    statusword &= ~(CIA402_SW_CS_ERR | CIA402_SW_CS_FOL);
    if(out.following_err) statusword |= CIA402_SW_CS_ERR;
    if(out.following) statusword |= CIA402_SW_CS_FOL;
    *cia402->statusword = statusword;
}

static inline void run_mode_csv(struct cia402 *cia402) {
    uint16_t statusword = *cia402->statusword;

    // Controlword: Cyclic synchronous modes have no mode-specific bits

    // Execute
    struct cia402_impl_csv_out out = cia402_impl_csv((struct cia402_impl_csv_in) {
        .target_vel = *cia402->target_vel
    });

    // Set statusword
    statusword &= ~(CIA402_SW_CS_ERR | CIA402_SW_CS_FOL);
    if(out.following) statusword |= CIA402_SW_CS_FOL;
    *cia402->statusword = statusword;
}

static inline void run_mode_cst(struct cia402 *cia402) {
    uint16_t statusword = *cia402->statusword;

    // Controlword: Cyclic synchronous modes have no mode-specific bits

    // Execute
    struct cia402_impl_cst_out out = cia402_impl_cst((struct cia402_impl_cst_in) {
        .target_tor = *cia402->target_tor
    });

    // Set statusword
    statusword &= ~(CIA402_SW_CS_ERR | CIA402_SW_CS_FOL);
    if(out.following) statusword |= CIA402_SW_CS_FOL;
    *cia402->statusword = statusword;
}

static inline void run_operation_enabled(struct cia402 *cia402) {
    const uint16_t initial_statusword = *cia402->statusword;
    enum cia402_mode_of_operation mode_req = *cia402->mop;
    enum cia402_mode_of_operation mode = *cia402->mop_display;

    // Handle mode change
    if(mode_req != mode) {
        LOG_INF("Setting mode of operation: %d %s", mode_req,
            mode_req == CIA402_MODE_PROFILE_POSITION ? "pp" :
            mode_req == CIA402_MODE_PROFILE_VELOCITY ? "pv" :
            mode_req == CIA402_MODE_PROFILE_TORQUE ? "tq" :
            mode_req == CIA402_MODE_CYCLIC_SYNCHRONOUS_POSITION ? "csp" :
            mode_req == CIA402_MODE_CYCLIC_SYNCHRONOUS_VELOCITY ? "csv" :
            mode_req == CIA402_MODE_CYCLIC_SYNCHRONOUS_TORQUE ? "cst" : "??");

        // Reset mode-specific bits
        cia402->priv->pp_target = *cia402->current_pos;
        cia402->priv->pp_target_reached = false;

        *cia402->mop_display = mode = mode_req;
        cia402_impl_mop_tpdo(cia402);
        cia402_impl_statusword_tpdo(cia402);
    }

    // Mode-specific logic
    switch(mode) {
        default: cia402_impl_halt(cia402); break; // Invalid mode
        case CIA402_MODE_PROFILE_POSITION:            run_mode_pp(cia402);  break;
        case CIA402_MODE_PROFILE_VELOCITY:            run_mode_pv(cia402);  break;
        case CIA402_MODE_PROFILE_TORQUE:              run_mode_tq(cia402);  break;
        case CIA402_MODE_CYCLIC_SYNCHRONOUS_POSITION: run_mode_csp(cia402); break;
        case CIA402_MODE_CYCLIC_SYNCHRONOUS_VELOCITY: run_mode_csv(cia402); break;
        case CIA402_MODE_CYCLIC_SYNCHRONOUS_TORQUE:   run_mode_cst(cia402); break;
    }

    if(*cia402->statusword != initial_statusword) {
        cia402_impl_statusword_tpdo(cia402);
    }
}


// CIA402_STATE_QUICK_STOP_ACTIVE: stop motor, then go to SWITCH_ON_DISABLED
static inline void enter_quick_stop_active(struct cia402 *cia402) {
    LOG_INF("Entering state: QUICK_STOP_ACTIVE");

    cia402_impl_stop(cia402);
    
    int sw = *cia402->statusword;
    sw = (sw & ~0b1101111) | 0b0000111; // x00x 0111
    *cia402->statusword = sw;
    cia402_impl_statusword_tpdo(cia402);
}


// CIA402_STATE_FAULT_REACTION_ACTIVE: stop motor, then go to FAULT
static inline void enter_fault_reaction_active(struct cia402 *cia402) {
    LOG_INF("Entering state: FAULT_REACTION_ACTIVE");

    cia402_impl_stop(cia402);
    
    int sw = *cia402->statusword;
    sw = (sw & ~0b1001111) | 0b0001111; // x0xx 1111
    *cia402->statusword = sw;
    cia402_impl_statusword_tpdo(cia402);
}

// CIA402_STATE_FAULT: stay in this state until explicitly taken out
static inline void enter_fault(struct cia402 *cia402) {
    LOG_INF("Entering state: FAULT");
    
    cia402_impl_shutdown(cia402);

    int sw = *cia402->statusword;
    sw = (sw & ~0b1001111) | 0b0001000; // x0xx 1000
    sw &= ~CIA402_SW_VOLTAGEEN;
    *cia402->statusword = sw;
    cia402_impl_statusword_tpdo(cia402);
}

void cia402_tick(struct cia402 *cia402, bool quick) {
    k_mutex_lock(&cia402->priv->lock, K_FOREVER);

    const enum cia402_state state = cia402->state;
    enum cia402_state nextState = state;
    uint16_t controlword = *cia402->controlword;

    if(state == CIA402_STATE_OPERATION_ENABLED) {
        run_operation_enabled(cia402);
    }

    // Parse ControlWord bits (cia402 v2.0 10.3.1)
    const bool cw_fault_reset      = controlword & CIA402_CW_FAULT_RESET;
    const bool cw_enable_operation = controlword & CIA402_CW_ENABLE_OPERATION;
    const bool cw_quick_stop       = controlword & CIA402_CW_QUICK_STOP;
    const bool cw_enable_voltage   = controlword & CIA402_CW_ENABLE_VOLTAGE;
    const bool cw_switch_on        = controlword & CIA402_CW_SWITCH_ON;

    // Compute commands (cia402 v2.0 10.3.1 table 4)
    const bool com_fault_reset       = cw_fault_reset;
    const bool com_shutdown          = !cw_fault_reset && cw_quick_stop && cw_enable_voltage && !cw_switch_on;
    const bool com_switch_on         = !cw_fault_reset && cw_quick_stop && cw_enable_voltage && cw_switch_on; // Enable operaion is "=0 OR =1" so an effective X
    const bool com_enable_operation  = !cw_fault_reset && cw_enable_operation && cw_quick_stop && cw_enable_voltage && cw_switch_on;
    const bool com_disable_operation = !cw_fault_reset && !cw_enable_operation && cw_quick_stop && cw_enable_voltage && cw_switch_on;
    const bool com_disable_voltage   = !cw_fault_reset && !cw_enable_voltage;
    const bool com_quick_stop        = !cw_fault_reset && !cw_quick_stop && cw_enable_voltage;

    // Compute transitions (via402 v2.0 10.1.1.2)
    // Transition 1: Initialization
    if(state == CIA402_STATE_NOT_READY_TO_SWITCH_ON) {
        if(cia402_impl_initialize(cia402)) {
            nextState = CIA402_STATE_SWITCH_ON_DISABLED;
        } else {
            LOG_ERR("COULD NOT INITIALIZE DRIVE");
            k_oops();
            return;
        }
    }

    // Transitions 2, 3, 4: SWITCH_ON_DISABLED -> READY_TO_SWITCH_ON -> SWITCHED_ON -> OPERATION_ENABLE
    if(state == CIA402_STATE_SWITCH_ON_DISABLED && com_shutdown)                                           nextState = CIA402_STATE_READY_TO_SWITCH_ON;
    if(state == CIA402_STATE_READY_TO_SWITCH_ON && com_switch_on)                                          nextState = CIA402_STATE_SWITCHED_ON;
    if(state == CIA402_STATE_SWITCHED_ON        && com_enable_operation && cia402_impl_is_started(cia402)) nextState = CIA402_STATE_OPERATION_ENABLED;

    // Transitions 5, 6, 7: OPERATION_ENABLE -> SWITCHED_ON -> READY_TO_SWITCH_ON -> SWITCH_ON_DISABLED
    if(state == CIA402_STATE_OPERATION_ENABLED  && com_disable_operation)                   nextState = CIA402_STATE_SWITCHED_ON;
    if(state == CIA402_STATE_SWITCHED_ON        && com_shutdown)                            nextState = CIA402_STATE_READY_TO_SWITCH_ON;
    if(state == CIA402_STATE_READY_TO_SWITCH_ON && (com_quick_stop || com_disable_voltage)) nextState = CIA402_STATE_SWITCH_ON_DISABLED;

    // Transitions 8, 9: Power off from OPERATION_ENABLED
    if(state == CIA402_STATE_OPERATION_ENABLED && com_shutdown)        nextState = CIA402_STATE_READY_TO_SWITCH_ON;
    if(state == CIA402_STATE_OPERATION_ENABLED && com_disable_voltage) nextState = CIA402_STATE_SWITCH_ON_DISABLED;

    // Transitions 10, 11, 12: Quick stop
    if(state == CIA402_STATE_SWITCHED_ON && (com_quick_stop || com_disable_voltage)) nextState = CIA402_STATE_SWITCH_ON_DISABLED;
    if(state == CIA402_STATE_OPERATION_ENABLED && com_quick_stop)                    nextState = CIA402_STATE_QUICK_STOP_ACTIVE;
    if(state == CIA402_STATE_QUICK_STOP_ACTIVE && com_disable_voltage)               nextState = CIA402_STATE_SWITCH_ON_DISABLED;
    if(state == CIA402_STATE_QUICK_STOP_ACTIVE && cia402_impl_is_stopped(cia402))    nextState = CIA402_STATE_SWITCH_ON_DISABLED;

    // Transition 13 triggered by cia402_enter_fault
    // Transition 14, 15: FAULT_REACTION_ACTIVE -> FAULT -> SWITCH_ON_DISABLED
    if(state == CIA402_STATE_FAULT_REACTION_ACTIVE && cia402_impl_is_stopped(cia402)) nextState = CIA402_STATE_FAULT;
    if(state == CIA402_STATE_FAULT && com_fault_reset)                                nextState = CIA402_STATE_SWITCH_ON_DISABLED;

    if(nextState != state) {
        switch(cia402->state = nextState) {
            case CIA402_STATE_NOT_READY_TO_SWITCH_ON: enter_not_ready_to_switch_on(cia402); break;
            case CIA402_STATE_SWITCH_ON_DISABLED:     enter_switch_on_disabled(cia402); break;
            case CIA402_STATE_READY_TO_SWITCH_ON:     enter_ready_to_switch_on(cia402); break;
            case CIA402_STATE_SWITCHED_ON:            enter_switched_on(cia402); break;
            case CIA402_STATE_OPERATION_ENABLED:      enter_operation_enabled(cia402); break;
            case CIA402_STATE_QUICK_STOP_ACTIVE:      enter_quick_stop_active(cia402); break;
            case CIA402_STATE_FAULT_REACTION_ACTIVE:  enter_fault_reaction_active(cia402); break;
            case CIA402_STATE_FAULT:                  enter_fault(cia402); break;
        }
    }

    //cia402_impl_statusword_tpdo(cia402);

    k_mutex_unlock(&cia402->priv->lock);
}

void cia402_enter_fault(struct cia402 *cia402) {
    k_mutex_lock(&cia402->priv->lock, K_FOREVER);
    cia402->state = CIA402_STATE_FAULT_REACTION_ACTIVE;
    enter_fault_reaction_active(cia402);
    k_mutex_unlock(&cia402->priv->lock);
}

void cia402_init(struct cia402* cia402) {
    cia402->priv = malloc(sizeof(struct cia402_priv));
    memset(cia402->priv, 0, sizeof(*cia402->priv));

    k_mutex_init(&cia402->priv->lock);
    k_mutex_lock(&cia402->priv->lock, K_FOREVER);

    cia402->state = CIA402_STATE_NOT_READY_TO_SWITCH_ON;
    enter_not_ready_to_switch_on(cia402);

    k_mutex_unlock(&cia402->priv->lock);
}
