/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #ifndef CIA402_IMPL_H_
#define CIA402_IMPL_H_

#include <stdbool.h>

/**
 * @defgroup cia402_impl CiA 402 application-specific implementation
 * @{
 */

/** @brief Trigger TPDO of StatusWord */
extern void cia402_impl_statusword_tpdo();

/** @brief Trigger TPDO of ModeOfOperationDisplay */
extern void cia402_impl_mop_tpdo();

/** @brief Initialize drive */
extern bool cia402_impl_initialize();

/** @brief Power off motor */
extern void cia402_impl_shutdown();

/** @brief Power on motor, starting with initialization */
extern void cia402_impl_start();

/** @brief Check if initialization was finished and motor may be controller. Called after `cia402_impl_start`. */
extern bool cia402_impl_is_started();

/** @brief Halt motor without powering it off. Returns true if the motor has stopped. */
extern bool cia402_impl_halt();

/** @brief Initiate fault / quick stop but keep drive powered on (as opposed to `cia402_impl_shutdown`) */
extern void cia402_impl_stop();

/** @brief Check if stop action was finished. Called after `cia402_impl_stop`. */
extern bool cia402_impl_is_stopped();

/**
 * @defgroup cia402_impl_pp Profile Position mode
 * @{
 */

/** @brief Data passed to profile position implementation */
struct cia402_impl_pp_in {
    int target_pos;
    bool halt;
};

/** @brief Data returned from profile position implementation */
struct cia402_impl_pp_out {
    bool target_reached;
    bool pp_err;
};

/** @brief Implementation of profile position mode of operation */
extern struct cia402_impl_pp_out cia402_impl_pp(struct cia402_impl_pp_in);

/** @} */ // cia402_impl_pp

/**
 * @defgroup cia402_impl_pv Profile Velocity mode
 * @{
 */

/** @brief Data passed to profile velocity implementation */
struct cia402_impl_pv_in {
    int target_vel;
    bool halt;
};

/** @brief Data returned from profile velocity implementation */
struct cia402_impl_pv_out {
    bool target_reached;
    bool speed;
    bool max_slip_err;
};

/** @brief Implementation of profile velocity mode of operation */
extern struct cia402_impl_pv_out cia402_impl_pv(struct cia402_impl_pv_in);

/** @} */ // cia402_impl_pv

/**
 * @defgroup cia402_impl_tq Profile Torque mode
 * @{
 */

/** @brief Data passed to profile velocity implementation */
struct cia402_impl_tq_in {
    int target_tor;
    bool halt;
};

/** @brief Data returned from profile velocity implementation */
struct cia402_impl_tq_out {
    bool target_reached;
};

/** @brief Implementation of profile toruqe mode of operation */
extern struct cia402_impl_tq_out cia402_impl_tq(struct cia402_impl_tq_in);

/** @} */ // cia402_impl_tq

/**
 * @defgroup cia402_impl_csp Cyclic Synchronous Position mode
 * @{
 */

/** @brief Data passed to cyclic synchronous position implementation */
struct cia402_impl_csp_in {
    int target_pos;
};

/** @brief Data returned from profile velocity implementation */
struct cia402_impl_csp_out {
    bool following;
    bool following_err;
};

/** @brief Implementation of cyclic synchronous position mode of operation */
extern struct cia402_impl_csp_out cia402_impl_csp(struct cia402_impl_csp_in);

/** @} */ // cia402_impl_csp

/**
 * @defgroup cia402_impl_csv Cyclic Synchronous Velocity mode
 * @{
 */

/** @brief Data passed to cyclic synchronous velocity implementation */
struct cia402_impl_csv_in {
    int target_vel;
};

/** @brief Data returned from profile velocity implementation */
struct cia402_impl_csv_out {
    bool following;
};

/** @brief Implementation of cyclic synchronous velocity mode of operation */
extern struct cia402_impl_csv_out cia402_impl_csv(struct cia402_impl_csv_in);

/** @} */ // cia402_impl_csv

/**
 * @defgroup cia402_impl_cst Cyclic Synchronous Torque mode
 * @{
 */

/** @brief Data passed to cyclic synchronous torque implementation */
struct cia402_impl_cst_in {
    int target_tor;
};

/** @brief Data returned from profile velocity implementation */
struct cia402_impl_cst_out {
    bool following;
};

/** @brief Implementation of cyclic synchronous torque mode of operation */
extern struct cia402_impl_cst_out cia402_impl_cst(struct cia402_impl_cst_in);

/** @} */ // cia402_impl_cst

/** @} */ // cia402_impl

#endif /* CIA402_IMPL_H_ */
