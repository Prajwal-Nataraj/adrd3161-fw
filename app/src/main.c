/*
 * Copyright (c) 2024-2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/can.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/zms.h>
#include <canopennode.h>
#include <app_version.h>

#include <app/drivers/misc/tmc9660/tmc9660.h>

#include "cia402.h"
#include "cia402_impl.h"
#include "OD.h"
#include "tmc9660_params.h"

#include "bodge.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

static const struct gpio_dt_spec led_green = GPIO_DT_SPEC_GET(DT_PATH(leds, led_status), gpios);
static const struct gpio_dt_spec led_fault = GPIO_DT_SPEC_GET(DT_PATH(buttons, fault), gpios);
// static const struct spi_dt_spec spi0 = SPI_DT_SPEC_GET(DT_NODELABEL(tmc9660_spi), SPI_MODE_CPHA | SPI_MODE_CPOL | SPI_WORD_SET(8), 0);
static const struct device *can = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

static const struct gpio_dt_spec dip1 = GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(buttons, dip), gpios, 0);
static const struct gpio_dt_spec dip2 = GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(buttons, dip), gpios, 1);
static const struct gpio_dt_spec dip3 = GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(buttons, dip), gpios, 2);

#define CAN_BITRATE (DT_PROP_OR(DT_CHOSEN(zephyr_canbus), bitrate, \
					  DT_PROP_OR(DT_CHOSEN(zephyr_canbus), bus_speed, \
						     CONFIG_CAN_DEFAULT_BITRATE)) / 1000)

const struct device *g_tmc9660 = DEVICE_DT_GET(DT_NODELABEL(tmc9660));
struct canopen co;
struct cia402 cia402;

#define PARTITION        storage_partition
#define PARTITION_DEVICE FIXED_PARTITION_DEVICE(PARTITION)
#define PARTITION_OFFSET FIXED_PARTITION_OFFSET(PARTITION)

struct zms_fs fs = {
    .offset = PARTITION_OFFSET,
    .sector_size = 8192,
    .sector_count = 2,
    .flash_device = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash))
};

CANOPEN_STORAGE_DEFINE(
    storage_entries,
    CANOPEN_STORAGE_ENTRY(OD_PERSIST_COMM, 2, CO_storage_cmd), // index 2 = communication params
    CANOPEN_STORAGE_ENTRY(OD_PERSIST_TMC, 4, CO_storage_cmd) // index 4 = manufacturer specific
);

/* Indicator LEDs */

/* CiA 303-3 standardizes how to indicate the status of the device using a red
 * and green LED, covering blink patterns for various errors / states.
 */
void led_callback(struct canopen *co, bool green, bool red)
{
    gpio_pin_set_dt(&led_green, green);

	/* Red LED is shared between the TMC and MAX. To light it up,
	* its GPIO must be enabled and pulled low. To turn it off,
	* its GPIO should be floated, so that the TMC may control it, and also so
	* we can read the TMC fault line, if need be.
	*/
    gpio_pin_configure_dt(&led_fault, red ? GPIO_OUTPUT_ACTIVE : GPIO_INPUT);
}

int init_leds()
{
    int err;

    if(!gpio_is_ready_dt(&led_green))
    {
        LOG_ERR("Green LED device not ready");
        return -1;
    }
    
    if(!gpio_is_ready_dt(&led_fault))
    {
        LOG_ERR("Red LED device not ready");
        return -1;
    }

    if((err = gpio_pin_configure_dt(&led_green, GPIO_OUTPUT_INACTIVE)))
    {
        LOG_ERR("Green LED configure failed: %d", err);
        return err;
    }
    
    if((err = gpio_pin_configure_dt(&led_fault, GPIO_INPUT)))
    {
        LOG_ERR("Fault LED configure failed: %d", err);
        return err;
    }

    return 0;
}

// Implement CiA 402 abstract application-specific functions

extern inline void cia402_impl_statusword_tpdo(struct cia402*) {
    OD_requestTPDO(OD_getFlagsPDO(OD_ENTRY_H6041_statusword), 0);
    LOG_DBG("SW %04x", OD_RAM.x6041_statusword);
}

extern inline void cia402_impl_mop_tpdo(struct cia402*) {
    OD_requestTPDO(OD_getFlagsPDO(OD_ENTRY_H6061_modesOfOperationDisplay), 0);
    LOG_DBG("MOPD %04x", OD_RAM.x6061_modesOfOperationDisplay);
}

extern inline bool cia402_impl_initialize(struct cia402*) {
    // Load parameters from flash
    //tmc_params_save(g_tmc9660); // BODGEEEEE
    int ret = 0; // tmc_params_load(g_tmc9660);

    // Make sure the motor is stopped, again
    tmc9660_set_param_retry(g_tmc9660, COMMUTATION_MODE, 0, 3);

    return ret == 0;
}

extern inline void cia402_impl_shutdown(struct cia402*) {
    tmc9660_set_param_retry(g_tmc9660, COMMUTATION_MODE, 0, 3);
}

extern inline void cia402_impl_start(struct cia402*) {
    tmc9660_set_param_retry(g_tmc9660, COMMUTATION_MODE, 5, 3);
}

extern inline bool cia402_impl_is_started(struct cia402*) {
    // Check if ABN is initialized. TODO: support for non-ABN setups
    int val;
    int ret = tmc9660_get_param(g_tmc9660, ABN_1_INIT_STATE, &val);
    return ret == 0 && val == 3;
}

extern inline bool cia402_impl_halt() {
    // Change to velocity mode and ramp down to 0
    tmc9660_set_param_retry(g_tmc9660, TARGET_VELOCITY, 0, 3);

    uint32_t flags;
    tmc9660_get_param_retry(g_tmc9660, GENERAL_STATUS_FLAGS, &flags, 3);
    return flags & (1 << 10);
}

extern inline struct cia402_impl_pp_out cia402_impl_pp(struct cia402_impl_pp_in in) {
    if(in.halt) {
        bool halted = cia402_impl_halt();
        return (struct cia402_impl_pp_out) {
            .target_reached = halted
        };
    }

    tmc9660_set_param_retry(g_tmc9660, TARGET_POSITION, in.target_pos, 3);

    uint32_t flags;
    tmc9660_get_param_retry(g_tmc9660, GENERAL_STATUS_FLAGS, &flags, 3);
    bool target_reached = flags & (1 << 9);

    return (struct cia402_impl_pp_out) {
        .target_reached = target_reached,
        .pp_err = false // Bug: ros2_canopen assumes "error" means what it does in software, not in control theory, and spams the logs with warning messages ("Mode handler has error.") whenever the drive is not exactly on target.
    };
}

extern inline struct cia402_impl_pv_out cia402_impl_pv(struct cia402_impl_pv_in in) {
    if(in.halt) {
        bool halted = cia402_impl_halt();
        return (struct cia402_impl_pv_out) {
            .target_reached = halted
        };
    }

    tmc9660_set_param_retry(g_tmc9660, TARGET_VELOCITY, in.target_vel, 3);

    uint32_t flags;
    tmc9660_get_param_retry(g_tmc9660, GENERAL_STATUS_FLAGS, &flags, 3);
    bool vel_reached = flags & (1 << 10);

    return (struct cia402_impl_pv_out) {
        .target_reached = vel_reached,
        .speed = false, // TODO
        .max_slip_err = false // TODO
    };
}

extern inline struct cia402_impl_tq_out cia402_impl_tq(struct cia402_impl_tq_in in) {
    if(in.halt) {
        bool halted = cia402_impl_halt();
        return (struct cia402_impl_tq_out) {
            .target_reached = halted
        };
    }

    tmc9660_set_param_retry(g_tmc9660, TARGET_TORQUE, in.target_tor, 3);

    int actual_torque;
    int ret = tmc9660_get_param(g_tmc9660, ACTUAL_TORQUE, &actual_torque);

    // Check torque +-5%
    // TODO: saner target_reached condition
    int torque_diff = actual_torque - in.target_tor;
    int torque_diff_acceptable = in.target_tor / 20;
    if(torque_diff_acceptable < 0) torque_diff_acceptable = -torque_diff_acceptable;
    bool tor_reached = ret == 0 && torque_diff > -torque_diff_acceptable && torque_diff < torque_diff_acceptable;

    return (struct cia402_impl_tq_out) {
        .target_reached = tor_reached
    };
}

extern inline struct cia402_impl_csp_out cia402_impl_csp(struct cia402_impl_csp_in in) {
    tmc9660_set_param_retry(g_tmc9660, TARGET_POSITION, in.target_pos, 3);

    return (struct cia402_impl_csp_out) {
        .following = true,
        .following_err = false // TODO implement with objects 6065, 6066
    };
}

extern inline struct cia402_impl_csv_out cia402_impl_csv(struct cia402_impl_csv_in in) {
    tmc9660_set_param_retry(g_tmc9660, TARGET_VELOCITY, in.target_vel, 3);

    return (struct cia402_impl_csv_out) {
        .following = true
    };
}

extern inline struct cia402_impl_cst_out cia402_impl_cst(struct cia402_impl_cst_in in) {
    tmc9660_set_param_retry(g_tmc9660, TARGET_TORQUE, in.target_tor, 3);

    return (struct cia402_impl_cst_out) {
        .following = true
    };
}

extern inline void cia402_impl_stop(struct cia402*) {
    tmc9660_set_param_retry(g_tmc9660, TARGET_VELOCITY, 0, 3);
    tmc9660_motor_stop(g_tmc9660);
}

extern inline bool cia402_impl_is_stopped(struct cia402*) {
    uint32_t flags;
    tmc9660_get_param_retry(g_tmc9660, GENERAL_STATUS_FLAGS, &flags, 3);
    bool vel_reached = flags & (1 << 10);

    return vel_reached;
}


void sync_callback(struct canopen *co, bool sync) {
    if(!sync) {
        cia402_tick(&cia402, true);
        return;
    }

    if(cia402.state == CIA402_STATE_OPERATION_ENABLED) {
        enum cia402_mode_of_operation mode = OD_RAM.x6061_modesOfOperationDisplay;

        // TODO: choose which variables to monitor based on the current mode a bit more selectively
        // e.g. ramper is irrelevant during torque control

        // Read all values from control loop
        // Don't retry. In case one read fails, it's more important to move on quickly than waste a lot of time here
        int torqueActualValue, targetTorque;
        tmc9660_get_param(g_tmc9660, ACTUAL_POSITION, &OD_RAM.x6064_positionActualValue);
        tmc9660_get_param(g_tmc9660, ACTUAL_VELOCITY, &OD_RAM.x606C_velocityActualValue);
        tmc9660_get_param(g_tmc9660, ACTUAL_TORQUE, &torqueActualValue); OD_RAM.x6077_torqueActualValue = torqueActualValue;
        // if(mode != CIA402_MODE_PROFILE_POSITION) tmc9660_get_param_retry(g_tmc9660, TARGET_POSITION, &OD_RAM.x6???_targetPosition);
        if(mode != CIA402_MODE_PROFILE_VELOCITY && mode != CIA402_MODE_CYCLIC_SYNCHRONOUS_VELOCITY) tmc9660_get_param(g_tmc9660, TARGET_VELOCITY, &OD_RAM.x60FF_targetVelocity);
        if(mode != CIA402_MODE_PROFILE_TORQUE && mode != CIA402_MODE_CYCLIC_SYNCHRONOUS_TORQUE) { tmc9660_get_param(g_tmc9660, TARGET_TORQUE, &targetTorque); OD_RAM.x6071_targetTorque = targetTorque; }
        // tmc9660_get_param(g_tmc9660, RAMP_POSITION, &OD_RAM.x6???_positionDemandValue);
        tmc9660_get_param(g_tmc9660, RAMP_VELOCITY, &OD_RAM.x606B_velocityDemandValue);
    }
    
    cia402_tick(&cia402, false);
}


ODR_t writeControlword(OD_stream_t *stream, const void *buf,
    OD_size_t count, OD_size_t *countWritten) {
        int ret = OD_writeOriginal(stream, buf, count, countWritten);

        if(ret != ODR_OK) {
            return ret;
        }

        LOG_DBG("CW = 0x%04X", OD_RAM.x6040_controlword);
        
        cia402_tick(&cia402, false);
        cia402_impl_statusword_tpdo(&cia402); // Return a statusword after every controlword write, even if no transitions happened

        return ret;
    }

// Debug CAN error counters and bus state
static void can_debug_state() {
    static int prev_tx_err_cnt = 0, prev_rx_err_cnt = 0;
    static enum can_state prev_state = CAN_STATE_STOPPED;
	struct can_bus_err_cnt err_cnt;
	enum can_state state;
	int err;

	err = can_get_state(co.can_dev, &state, &err_cnt);
	if (err != 0) {
		LOG_ERR("failed to get CAN controller state (err %d)", err);
		return;
	}
    if(prev_tx_err_cnt != err_cnt.tx_err_cnt || prev_rx_err_cnt != err_cnt.rx_err_cnt || prev_state != state) {
		LOG_DBG("CAN TEC = %d\tREC = %d\tstate = %s", err_cnt.tx_err_cnt, err_cnt.rx_err_cnt,
            state == CAN_STATE_ERROR_ACTIVE ? "ACTIVE" :
            state == CAN_STATE_ERROR_WARNING ? "WARNING" :
            state == CAN_STATE_ERROR_PASSIVE ? "PASSIVE" :
            state == CAN_STATE_BUS_OFF ? "BUS_OFF" :
            state == CAN_STATE_STOPPED ? "STOPPED" : "???"
        );
    }

    if(state == CAN_STATE_BUS_OFF) {
        // TODO rate limit, if it goes into bus off a lot, stop resetting and just leave it in the fault state
        LOG_ERR("Bus-off: killing motor");
        CO_NMT_sendInternalCommand(co.CO->NMT, CO_NMT_ENTER_STOPPED);
        cia402_enter_fault(&cia402);
    }

    int coerr = CO_getErrorRegister(co.CO->em);
    if(coerr) {
        LOG_ERR("CANopenNode EM: 0x%04x", coerr);
    }

    prev_tx_err_cnt = err_cnt.tx_err_cnt;
    prev_rx_err_cnt = err_cnt.rx_err_cnt;
    prev_state = state;
}

int main()
{
    int err;

    calibrate_oscillator();

    LOG_INF("Firmware git revision %s", STRINGIFY(APP_BUILD_VERSION));

    // Initialize TMC9660
    if(!device_is_ready(g_tmc9660))
    {
        LOG_ERR("TMC9660 init failed");
        cia402_enter_fault(&cia402);
        CO_errorReport(co.CO->em, CO_ERR_REG_GENERIC_ERR, CO_EMC_HARDWARE, 9660);
        CO_NMT_sendInternalCommand(co.CO->NMT, CO_NMT_ENTER_STOPPED);
        return -1;
    }

    // Make sure motor is stopped
    tmc9660_motor_stop(g_tmc9660);

    // Initialize and read DIP switches for CAN and I2C ID
    err = gpio_pin_configure_dt(&dip1, GPIO_INPUT);
    if(err < 0) {
        LOG_ERR("Could not initialize DIP GPIO 1: %d", err);
        goto fail;
    }
    err = gpio_pin_configure_dt(&dip2, GPIO_INPUT);
    if(err < 0) {
        LOG_ERR("Could not initialize DIP GPIO 2: %d", err);
        goto fail;
    }
    err = gpio_pin_configure_dt(&dip3, GPIO_INPUT);
    if(err < 0) {
        LOG_ERR("Could not initialize DIP GPIO 3: %d", err);
        goto fail;
    }
    
    int dip_setting = (gpio_pin_get_dt(&dip1) << 2) | (gpio_pin_get_dt(&dip2) << 1) | (gpio_pin_get_dt(&dip3) << 0); // Looking at the DIP switch with "1 2 3" going left to right = reading the number in binary
    LOG_INF("DIP switches set to %d", dip_setting);

    k_msleep(dip_setting * 5); // Delay each node's bringup differently to reduce initial clashes

    // Initialize CiA 303 LEDs
	err = init_leds();
    if(err < 0)
    {
        LOG_ERR("CiA 303 LED indicator init failed");
        goto fail;
    }

    // Initialize CiA 402
    cia402.controlword = &OD_RAM.x6040_controlword;
    cia402.statusword = &OD_RAM.x6041_statusword;
    cia402.mop = &OD_RAM.x6060_modesOfOperation;
    cia402.mop_display = &OD_RAM.x6061_modesOfOperationDisplay;
    cia402.target_pos = &OD_RAM.x607A_targetPosition;
    cia402.target_vel = &OD_RAM.x60FF_targetVelocity;
    cia402.target_tor = &OD_RAM.x6071_targetTorque;
    cia402.current_pos = &OD_RAM.x6064_positionActualValue;

    // Create noop extensions to allow event-driven TPDO requests
    OD_extension_t statusword_ext = {
        .read = OD_readOriginal, .write = OD_writeOriginal, .object = NULL
    }, mopd_ext = {
        .read = OD_readOriginal, .write = OD_writeOriginal, .object = NULL
    }, cw_ext = {
        .read = OD_readOriginal, .write = writeControlword, .object = NULL
    };
    OD_extension_init(OD_ENTRY_H6041_statusword, &statusword_ext);
    OD_extension_init(OD_ENTRY_H6061_modesOfOperationDisplay, &mopd_ext);
    OD_extension_init(OD_ENTRY_H6040_controlword, &cw_ext);

    cia402_init(&cia402);

    // Initialize and start CANOpen
    co.can_dev = can;
    co.node_id = 0x10 + dip_setting;
    LOG_INF("CANopen node ID = %d", co.node_id);
    co.bitrate = CAN_BITRATE;
    co.nmt_control = CO_NMT_STARTUP_TO_OPERATIONAL;
    co.led_callback = led_callback;
    co.sync_callback = sync_callback;
#if CO_CONFIG_STORAGE & CO_CONFIG_STORAGE_ENABLE
    co.storage_entries = storage_entries;
    co.storage_entries_count = ARRAY_SIZE(storage_entries);
#endif  // CO_CONFIG_STORAGE

    err = canopen_init(&co);
    if(err < 0)
    {
        LOG_ERR("CANopenNode init failed");
        goto fail;
    }

    // Write persisted params to TMC and set up extensions for intercepting reads and writes
    tmc_params_init();
    
    LOG_INF("Initialization OK!");

    // Main loop for housekeeping
    while(true) {
        can_debug_state();

        // static int prev_cw = 0x1234;
        // if(OD_RAM.x6040_controlword != prev_cw) {
        //     LOG_DBG("CW = 0x%04X", OD_RAM.x6040_controlword);
        //     prev_cw = OD_RAM.x6040_controlword;
        // }

        if(co.CO->NMT->operatingState == CO_NMT_STOPPED) {
            cia402_enter_fault(&cia402);
        }

        /*if(k_timer_status_get(&supply_voltage_timer)) {
            int32_t supply_voltage;
            tmc9660_get_param(g_tmc9660, SUPPLY_VOLTAGE, &supply_voltage);
            //CO_LOCK_OD(CO->CANmodule);
            OD_set_u16(OD_ENTRY_H2122_SUPPLY_VOLTAGE, 0, (uint16_t) supply_voltage, false);
            //CO_UNLOCK_OD(CO->CANmodule);
        }*/

        k_msleep(1);
    }

    return 0;

fail:
    return err;
}
