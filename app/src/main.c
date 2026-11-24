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

#define CAN_BITRATE (DT_PROP_OR(DT_CHOSEN(zephyr_canbus), bitrate, \
					  DT_PROP_OR(DT_CHOSEN(zephyr_canbus), bus_speed, \
						     CONFIG_CAN_DEFAULT_BITRATE)) / 1000)

const struct device *g_tmc9660 = DEVICE_DT_GET(DT_NODELABEL(tmc9660));
struct canopen co;
struct cia402 cia402;

#define PARTITION        storage_partition
#define PARTITION_DEVICE FIXED_PARTITION_DEVICE(PARTITION)
#define PARTITION_OFFSET FIXED_PARTITION_OFFSET(PARTITION)

/* Safety Doors, Faults and Limit Switches */
static const struct device *gpio0_dr_ls = DEVICE_DT_GET(DT_NODELABEL(gpio0));       /* dr: Door, ls: Limit Switch */
#define FRONT_DOOR_PIN  7
#define REAR_DOOR_PIN   8
#define FRONT_DOOR_BIT  2
#define REAR_DOOR_BIT   1

#define LIM_SW_UP_PIN   19
#define LIM_SW_LO_PIN   18
#define LIM_SW_UP_BIT   4       /* Originally, according to system module, LIM_SW_UP_BIT should be 3 and LIM_SW_LO_BIT should be 4 */
#define LIM_SW_LO_BIT   3       /* But in order to match the AMC hardware configuration, the bits are swapped */

/* Fault bits */
#define DRIVE_RESET            BIT(0)
#define SHORT_CIRCUIT          BIT(2)
#define DRIVE_INTERNAL_ERROR   BIT(1)
#define OVER_CURRENT           BIT(3)
#define UNDER_VOLTAGE          BIT(4)
#define OVER_VOLTAGE           BIT(5)
#define OVER_TEMPERATURE       BIT(6)

/* Machine type */
#define LS1     (0x01)
#define LS5     (0x02)

static struct gpio_callback front_door_gpio_cb;
static struct gpio_callback rear_door_gpio_cb;
static struct gpio_callback ls_up_gpio_cb;
static struct gpio_callback ls_lo_gpio_cb;
static struct k_spinlock drv_dig_inputs_lock;
bool set_error_flags = false;

static void drv_dig_inputs_set_bit(uint32_t mask, bool set)
{
    k_spinlock_key_t key = k_spin_lock(&drv_dig_inputs_lock);
    uint32_t inputs = OD_RAM.x2200_drv_DigInputs;

    if (set) {
        inputs |= mask;
    } else {
        inputs &= ~mask;
    }

    OD_RAM.x2200_drv_DigInputs = inputs;
    k_spin_unlock(&drv_dig_inputs_lock, key);
}

static void front_door_gpio_callback(const struct device *port,
                                     struct gpio_callback *cb,
                                     gpio_port_pins_t pins)
{
    if ((pins & BIT(FRONT_DOOR_PIN)) == 0U) {
        return;
    }

    int value = gpio_pin_get(port, FRONT_DOOR_PIN);
    if (value >= 0) {
        drv_dig_inputs_set_bit(BIT(FRONT_DOOR_BIT), value != 0);
    }
}

static void rear_door_gpio_callback(const struct device *port,
                                    struct gpio_callback *cb,
                                    gpio_port_pins_t pins)
{
    if ((pins & BIT(REAR_DOOR_PIN)) == 0U) {
        return;
    }

    int value = gpio_pin_get(port, REAR_DOOR_PIN);
    if (value >= 0) {
        drv_dig_inputs_set_bit(BIT(REAR_DOOR_BIT), value != 0);
    }
}

static void ls_up_gpio_callback(const struct device *port,
                                    struct gpio_callback *cb,
                                    gpio_port_pins_t pins)
{
    if ((pins & BIT(LIM_SW_UP_PIN)) == 0U) {
        return;
    }

    int value = gpio_pin_get(port, LIM_SW_UP_PIN);
    if (value >= 0) {
        drv_dig_inputs_set_bit(BIT(LIM_SW_UP_BIT), value != 0);
    }
}

static void ls_lo_gpio_callback(const struct device *port,
                                     struct gpio_callback *cb,
                                     gpio_port_pins_t pins)
{
    if ((pins & BIT(LIM_SW_LO_PIN)) == 0U) {
        return;
    }

    int value = gpio_pin_get(port, LIM_SW_LO_PIN);
    if (value >= 0) {
        drv_dig_inputs_set_bit(BIT(LIM_SW_LO_BIT), value != 0);
    }
}

static void adi_to_amc_faults(uint32_t gen_err_flags,
			   uint32_t gdrv_err_flags)
{
	uint16_t amc_faults = 0;

	/* Drive Reset */
	if (gen_err_flags & (BIT(9)  |  /* Watchdog event */
			     BIT(22))) /* Fault retry happened */
	{
		amc_faults |= DRIVE_RESET;
	}

	/* Drive Internal Error */
	if (gen_err_flags & (BIT(1)  |  /* TMCL script error */
			     BIT(23))) /* Fault retries failed */
	{
		amc_faults |= DRIVE_INTERNAL_ERROR;
	}

	/* Short Circuit
	 * U/V only, W/Y2 excluded
	 */
	if (gdrv_err_flags & (
		BIT(4)  | BIT(5)  |  /* low-side discharge short */
		BIT(8)  | BIT(9)  |  /* low-side charge short */
		BIT(20) | BIT(21) |  /* high-side discharge short */
		BIT(24) | BIT(25)))  /* high-side charge short */
	{
		amc_faults |= SHORT_CIRCUIT;
	}

	/* Over Current
	 * U/V only, W/Y2 excluded
	 */
	if ((gdrv_err_flags & (
			BIT(0)  | BIT(1)  |   /* low-side overcurrent */
			BIT(16) | BIT(17))) || /* high-side overcurrent */
	    (gen_err_flags & (
			BIT(16) |           /* I²t limit 1 exceeded */
			BIT(17))))          /* I²t limit 2 exceeded */
	{
		amc_faults |= OVER_CURRENT;
	}

	/* Drive Under Voltage
	 * U/V only, W/Y2 excluded
	 */
	if ((gen_err_flags & BIT(20)) || /* Supply undervoltage warning */
	    (gdrv_err_flags & (
			BIT(12) | BIT(13) | /* U/V bootstrap undervoltage */
			BIT(29) |           /* Gate-driver undervoltage */
			BIT(30) |           /* Gate-driver low voltage */
			BIT(31))))          /* Gate-driver supply undervoltage */
	{
		amc_faults |= UNDER_VOLTAGE;
	}

	/* Drive Over Voltage */
    if (gen_err_flags & (
		BIT(19) | /* Supply overvoltage warning */
		BIT(21))) /* ADC input overvoltage */
	{
		amc_faults |= OVER_VOLTAGE;
	}

	/* Drive Over Temperature */
	if (gen_err_flags & (
		BIT(13) | /* External temperature exceeded */
		BIT(14))) /* Chip temperature exceeded */
	{
		amc_faults |= OVER_TEMPERATURE;
	}

	OD_RAM.x2200_drv_DigInputs |= (amc_faults << 16);
}

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

    int gen_error_flags, gdrv_error_flags;
    if ((tmc9660_get_param(g_tmc9660, GENERAL_ERROR_FLAGS, &gen_error_flags) == 0) &&
        (tmc9660_get_param(g_tmc9660, GDRV_ERROR_FLAGS, &gdrv_error_flags) == 0)) {
        OD_APP_SHADOW.x212B_GENERAL_ERROR_FLAGS = gen_error_flags;
        OD_APP_SHADOW.x212C_GDRV_ERROR_FLAGS = gdrv_error_flags;
        set_error_flags = true;
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

/*
 * Apply INI parameters based on machine type
 * mType: machine type identifier
 */
void configureMachine(uint8_t mType)
{
    OD_PERSIST_TMC.x2000_MOTOR_TYPE = 1;
    OD_PERSIST_TMC.x2002_MOTOR_DIRECTION = 2;
    OD_PERSIST_TMC.x2005_OUTPUT_VOLTAGE_LIMIT = 32767;
    OD_PERSIST_TMC.x2015_CURRENT_SCALING_FACTOR = 390;
    OD_PERSIST_TMC.x201F_ADC_I1_INVERTED = 0;
    OD_PERSIST_TMC.x2020_ADC_I2_INVERTED = 0;
    OD_PERSIST_TMC.x2034_RAMP_ENABLE = 1;
    OD_PERSIST_TMC.x2036_RAMP_AMAX = 4000;
    OD_PERSIST_TMC.x2037_RAMP_A1 = 2000;
    OD_PERSIST_TMC.x2039_RAMP_DMAX = 6000;
    OD_PERSIST_TMC.x203B_RAMP_D2 = 4000;
    OD_PERSIST_TMC.x203D_RAMP_V1 = 32000;
    OD_PERSIST_TMC.x203E_RAMP_V2 = 64000;
    OD_PERSIST_TMC.x205A_ABN_1_STEPS = 20000;
    OD_PERSIST_TMC.x207B_VELOCITY_SENSOR_SELECTION = 2;
    OD_PERSIST_TMC.x207F_VELOCITY_P = 50;
    OD_PERSIST_TMC.x2081_VELOCITY_NORM_P = 0;
    OD_PERSIST_TMC.x2082_VELOCITY_NORM_I = 0;
    OD_PERSIST_TMC.x2087_VELOCITY_LOOP_DOWNSAMPLING = 0;
    OD_PERSIST_TMC.x2089_VELOCITY_METER_SWITCH_THRESHOLD = 0;
    OD_PERSIST_TMC.x208E_POSITION_SENSOR_SELECTION = 2;
    OD_PERSIST_TMC.x20A1_REFERENCE_SWITCH_ENABLE = 3;
    OD_PERSIST_TMC.x20A4_EVENT_STOP_SETTINGS = 1;
    OD_PERSIST_TMC.x20AE_ABN_2_STEPS = 0;
    OD_PERSIST_TMC.x20B0_ABN_2_GEAR_RATIO = 0;
    OD_PERSIST_TMC.x20B7_SPI_ENCODER_MAIN_TRANSFER_CMD_SIZE = 0;
    OD_PERSIST_TMC.x20B9_SPI_ENCODER_TRANSFER_DATA_3_0 = 0;
    OD_PERSIST_TMC.x20BA_SPI_ENCODER_TRANSFER_DATA_7_4 = 0;
    OD_PERSIST_TMC.x20BB_SPI_ENCODER_TRANSFER_DATA_11_8 = 0;
    OD_PERSIST_TMC.x20BC_SPI_ENCODER_TRANSFER_DATA_15_12 = 0;
    OD_PERSIST_TMC.x20BE_SPI_ENCODER_POSITION_COUNTER_MASK = 0;
    OD_PERSIST_TMC.x20C4_SPI_ENCODER_OFFSET = 0;
    OD_PERSIST_TMC.x20D4_BRAKE_CHOPPER_ENABLE = 1;
    OD_PERSIST_TMC.x20D5_BRAKE_CHOPPER_VOLTAGE_LIMIT = 495;
    OD_PERSIST_TMC.x20DD_INVERT_BRAKE_OUTPUT = 1;
    OD_PERSIST_TMC.x20F1_DRIVE_TIME_SINK_UVW = 40;
    OD_PERSIST_TMC.x20F2_DRIVE_TIME_SOURCE_UVW = 40;
    OD_PERSIST_TMC.x20F3_DRIVE_TIME_SINK_Y2 = 40;
    OD_PERSIST_TMC.x20F4_DRIVE_TIME_SOURCE_Y2 = 40;
    OD_PERSIST_TMC.x20F5_UVW_SINK_CURRENT = 10;
    OD_PERSIST_TMC.x20F6_UVW_SOURCE_CURRENT = 5;
    OD_PERSIST_TMC.x20F7_Y2_SINK_CURRENT = 10;
    OD_PERSIST_TMC.x20F8_Y2_SOURCE_CURRENT = 5;
    OD_PERSIST_TMC.x2102_OVERCURRENT_PROTECTION_UVW_LOW_SIDE_THRESHOLD = 1;
    OD_PERSIST_TMC.x2103_OVERCURRENT_PROTECTION_UVW_HIGH_SIDE_THRESHOLD = 2;
    OD_PERSIST_TMC.x2104_OVERCURRENT_PROTECTION_Y2_LOW_SIDE_THRESHOLD = 1;
    OD_PERSIST_TMC.x2105_OVERCURRENT_PROTECTION_Y2_HIGH_SIDE_THRESHOLD = 4;
    OD_PERSIST_TMC.x210E_OVERCURRENT_PROTECTION_UVW_LOW_SIDE_USE_VDS = 0;
    OD_PERSIST_TMC.x210F_OVERCURRENT_PROTECTION_Y2_LOW_SIDE_USE_VDS = 0;
    OD_PERSIST_TMC.x2123_SUPPLY_OVERVOLTAGE_WARNING_THRESHOLD = 710;
    OD_PERSIST_TMC.x2124_SUPPLY_UNDERVOLTAGE_WARNING_THRESHOLD = 78;
    OD_PERSIST_TMC.x213E_TARGET_TORQUE_BIQUAD_FILTER_ENABLE = 1;

    if(mType == LS1) {
        
        OD_PERSIST_TMC.x2006_MAX_TORQUE = 10000;
        OD_PERSIST_TMC.x2007_MAX_FLUX = 1690;
        OD_PERSIST_TMC.x2022_ADC_I0_OFFSET = -14;
        OD_PERSIST_TMC.x2023_ADC_I1_OFFSET = 8;
        OD_PERSIST_TMC.x2025_ADC_I3_OFFSET = 7;
        OD_PERSIST_TMC.x202E_OPENLOOP_CURRENT = 3000;
        OD_PERSIST_TMC.x206D_TORQUE_P = 500;
        OD_PERSIST_TMC.x206E_TORQUE_I = 300;
        OD_PERSIST_TMC.x206F_FLUX_P = 500;
        OD_PERSIST_TMC.x2070_FLUX_I = 300;
        OD_PERSIST_TMC.x2080_VELOCITY_I = 120;
        OD_PERSIST_TMC.x20E1_IIT_LIMIT_1 = 75000;
        OD_PERSIST_TMC.x20E4_IIT_LIMIT_2 = 150000;

        // Write persisted params to TMC and set up extensions for intercepting reads and writes
        tmc_params_init();
        LOG_INF("Initialization OK!");
    }
    else if(mType == LS5)
    {
        OD_PERSIST_TMC.x2006_MAX_TORQUE = 12000;
        OD_PERSIST_TMC.x2007_MAX_FLUX = 947;
        OD_PERSIST_TMC.x2022_ADC_I0_OFFSET = -3;
        OD_PERSIST_TMC.x2023_ADC_I1_OFFSET = 9;
        OD_PERSIST_TMC.x2024_ADC_I2_OFFSET = -4;
        OD_PERSIST_TMC.x2025_ADC_I3_OFFSET = 21;
        OD_PERSIST_TMC.x202E_OPENLOOP_CURRENT = 5000;
        OD_PERSIST_TMC.x206D_TORQUE_P = 1000;
        OD_PERSIST_TMC.x206E_TORQUE_I = 4000;
        OD_PERSIST_TMC.x206F_FLUX_P = 1000;
        OD_PERSIST_TMC.x2070_FLUX_I = 4000;
        OD_PERSIST_TMC.x2080_VELOCITY_I = 200;
        OD_PERSIST_TMC.x20E1_IIT_LIMIT_1 = -1;
        OD_PERSIST_TMC.x20E4_IIT_LIMIT_2 = -1;
        
        // Write persisted params to TMC and set up extensions for intercepting reads and writes
        tmc_params_init();
        LOG_INF("Initialization OK!");
    }

    else {
        LOG_ERR("Machine configuration failed");
    }
}

int main()
{
    int err = -1;
    bool machineConfigured = false;

    LOG_INF("Firmware git revision %s", OD_PERSIST_COMM.x100A_manufacturerSoftwareVersion);
    LOG_INF("Serial number %d", OD_PERSIST_COMM.x1018_identity.serialNumber);


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

    tmc9660_set_gpio(g_tmc9660, 16, 1);
    calibrate_oscillator();
    k_msleep(100);
    tmc9660_set_gpio(g_tmc9660, 16, 0);

    if (!device_is_ready(gpio0_dr_ls)) {
        LOG_ERR("GPIO0 device not ready");
        goto fail;
    }
    err = gpio_pin_configure(gpio0_dr_ls, FRONT_DOOR_PIN, GPIO_INPUT);
    if(err < 0) {
        LOG_ERR("Could not initialize GPIO %d: %d", FRONT_DOOR_PIN, err);
        goto fail;
    }
    err = gpio_pin_configure(gpio0_dr_ls, REAR_DOOR_PIN, GPIO_INPUT);
    if(err < 0) {
        LOG_ERR("Could not initialize GPIO %d: %d", REAR_DOOR_PIN, err);
        goto fail;
    }
    err = gpio_pin_configure(gpio0_dr_ls, LIM_SW_UP_PIN, GPIO_INPUT);
    if(err < 0) {
        LOG_ERR("Could not initialize GPIO %d: %d", LIM_SW_UP_PIN, err);
        goto fail;
    }
    err = gpio_pin_configure(gpio0_dr_ls, LIM_SW_LO_PIN, GPIO_INPUT);
    if(err < 0) {
        LOG_ERR("Could not initialize GPIO %d: %d", LIM_SW_LO_PIN, err);
        goto fail;
    }

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
    co.node_id = 0x30;
    LOG_INF("CANopen node ID = %d", co.node_id);
    co.bitrate = CAN_BITRATE;
    co.nmt_control = 0; // CO_NMT_STARTUP_TO_OPERATIONAL;
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

    gpio_init_callback(&front_door_gpio_cb, front_door_gpio_callback,
                       BIT(FRONT_DOOR_PIN));
    err = gpio_add_callback(gpio0_dr_ls, &front_door_gpio_cb);
    if (err < 0) {
        LOG_ERR("Could not add front door GPIO callback: %d", err);
        goto fail;
    }

    gpio_init_callback(&rear_door_gpio_cb, rear_door_gpio_callback,
                       BIT(REAR_DOOR_PIN));
    err = gpio_add_callback(gpio0_dr_ls, &rear_door_gpio_cb);
    if (err < 0) {
        LOG_ERR("Could not add rear door GPIO callback: %d", err);
        goto fail;
    }

    err = gpio_pin_interrupt_configure(gpio0_dr_ls, FRONT_DOOR_PIN,
                                       GPIO_INT_EDGE_BOTH);
    if (err < 0) {
        LOG_ERR("Could not configure front door GPIO interrupt: %d", err);
        gpio_remove_callback(gpio0_dr_ls, &front_door_gpio_cb);
        goto fail;
    }

    err = gpio_pin_interrupt_configure(gpio0_dr_ls, REAR_DOOR_PIN,
                                       GPIO_INT_EDGE_BOTH);
    if (err < 0) {
        LOG_ERR("Could not configure rear door GPIO interrupt: %d", err);
        gpio_remove_callback(gpio0_dr_ls, &rear_door_gpio_cb);
        goto fail;
    }
    
    int FDvalue = gpio_pin_get(gpio0_dr_ls, FRONT_DOOR_PIN);
    if (FDvalue < 0) {
        LOG_ERR("Could not read front door GPIO: %d", FDvalue);
        gpio_pin_interrupt_configure(gpio0_dr_ls, FRONT_DOOR_PIN,
                                     GPIO_INT_DISABLE);
        gpio_remove_callback(gpio0_dr_ls, &front_door_gpio_cb);
        goto fail;
    }
    drv_dig_inputs_set_bit(BIT(FRONT_DOOR_BIT), FDvalue != 0);

    int RDvalue = gpio_pin_get(gpio0_dr_ls, REAR_DOOR_PIN);
    if (RDvalue < 0) {
        LOG_ERR("Could not read rear door GPIO: %d", RDvalue);
        gpio_pin_interrupt_configure(gpio0_dr_ls, REAR_DOOR_PIN,
                                     GPIO_INT_DISABLE);
        gpio_remove_callback(gpio0_dr_ls, &rear_door_gpio_cb);
        goto fail;
    }
    drv_dig_inputs_set_bit(BIT(REAR_DOOR_BIT), RDvalue != 0);

    gpio_init_callback(&ls_up_gpio_cb, ls_up_gpio_callback,
                       BIT(LIM_SW_UP_PIN));
    err = gpio_add_callback(gpio0_dr_ls, &ls_up_gpio_cb);
    if (err < 0) {
        LOG_ERR("Could not add LS UP GPIO callback: %d", err);
        goto fail;
    }

    gpio_init_callback(&ls_lo_gpio_cb, ls_lo_gpio_callback,
                       BIT(LIM_SW_LO_PIN));
    err = gpio_add_callback(gpio0_dr_ls, &ls_lo_gpio_cb);
    if (err < 0) {
        LOG_ERR("Could not add LS LO GPIO callback: %d", err);
        goto fail;
    }

    err = gpio_pin_interrupt_configure(gpio0_dr_ls, LIM_SW_UP_PIN,
                                       GPIO_INT_EDGE_FALLING);
    if (err < 0) {
        LOG_ERR("Could not configure LS UP GPIO interrupt: %d", err);
        gpio_remove_callback(gpio0_dr_ls, &ls_up_gpio_cb);
        goto fail;
    }

    err = gpio_pin_interrupt_configure(gpio0_dr_ls, LIM_SW_LO_PIN,
                                       GPIO_INT_EDGE_FALLING);
    if (err < 0) {
        LOG_ERR("Could not configure LS LO GPIO interrupt: %d", err);
        gpio_remove_callback(gpio0_dr_ls, &ls_lo_gpio_cb);
        goto fail;
    }
    
    int LSUPvalue = gpio_pin_get(gpio0_dr_ls, LIM_SW_UP_PIN);
    if (LSUPvalue < 0) {
        LOG_ERR("Could not read LS UP GPIO: %d", LSUPvalue);
        gpio_pin_interrupt_configure(gpio0_dr_ls, LIM_SW_UP_PIN,
                                     GPIO_INT_DISABLE);
        gpio_remove_callback(gpio0_dr_ls, &ls_up_gpio_cb);
        goto fail;
    }
    drv_dig_inputs_set_bit(BIT(LIM_SW_UP_BIT), LSUPvalue != 0);

    int LSLOvalue = gpio_pin_get(gpio0_dr_ls, LIM_SW_LO_PIN);
    if (LSLOvalue < 0) {
        LOG_ERR("Could not read LS LO GPIO: %d", LSLOvalue);
        gpio_pin_interrupt_configure(gpio0_dr_ls, LIM_SW_LO_PIN,
                                     GPIO_INT_DISABLE);
        gpio_remove_callback(gpio0_dr_ls, &ls_lo_gpio_cb);
        goto fail;
    }
    drv_dig_inputs_set_bit(BIT(LIM_SW_LO_BIT), LSLOvalue != 0);

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

        if (set_error_flags) {
            adi_to_amc_faults((uint32_t)OD_APP_SHADOW.x212B_GENERAL_ERROR_FLAGS,
                               (uint32_t)OD_APP_SHADOW.x212C_GDRV_ERROR_FLAGS);
            set_error_flags = false;
        }

        if ((0x00 != OD_RAM.x6800_machineType) && (false == machineConfigured)) {
            configureMachine(OD_RAM.x6800_machineType);
            machineConfigured = true;
        }

        k_msleep(1);
    }

    return 0;

fail:
    return err;
}
