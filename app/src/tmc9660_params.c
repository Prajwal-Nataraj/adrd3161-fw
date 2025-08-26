/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <stdlib.h>

#ifdef CONFIG_SHELL_TMC_PARAMS
#include <zephyr/shell/shell.h>
#endif /* CONFIG_SHELL_TMC_PARAMS */

#include <app/drivers/misc/tmc9660/tmc9660.h>

#include "tmc9660_params.h"
#include "storage.h"

LOG_MODULE_REGISTER(tmc9660_params, LOG_LEVEL_INF);



#ifdef CONFIG_TMC_PARAMS_REFSW
#define FOREACH_TMC9660_REFSW_PARAM(DO) \
    DO(REFERENCE_SWITCH_ENABLE, 0) \
    DO(REFERENCE_SWITCH_POLARITY_AND_SWAP, 0) \
    DO(REFERENCE_SWITCH_LATCH_SETTINGS, 0) \
    DO(REFERENCE_SWITCH_SEARCH_MODE, 0) \
    DO(REFERENCE_SWITCH_SEARCH_SPEED, 0) \
    DO(REFERENCE_SWITCH_SPEED, 0)
#else
#define FOREACH_TMC9660_REFSW_PARAM(DO)
#endif

#ifdef CONFIG_TMC_PARAMS_ABN1
#define FOREACH_TMC9660_ABN1_PARAM(DO) \
        DO(ABN_1_STEPS, 40000) \
        DO(ABN_1_DIRECTION, 0) \
        DO(ABN_1_INIT_METHOD, 0) \
        DO(ABN_1_INIT_DELAY, 1000) \
        DO(ABN_1_INIT_VELOCITY, 5) \
        DO(ABN_1_N_CHANNEL_PHI_E_OFFSET, 0) \
        DO(ABN_1_N_CHANNEL_INVERTED, 0) \
        DO(ABN_1_N_CHANNEL_FILTERING, 0)
#else
#define FOREACH_TMC9660_ABN1_PARAM(DO)
#endif

#ifdef CONFIG_TMC_PARAMS_ABN2
#define FOREACH_TMC9660_ABN2_PARAM(DO) \
        DO(ABN_2_STEPS, 0) \
        DO(ABN_2_DIRECTION, 0) \
        DO(ABN_2_GEAR_RATIO, 0) \
        DO(ABN_2_ENABLE, 0)
#else
#define FOREACH_TMC9660_ABN2_PARAM(DO)
#endif

#ifdef CONFIG_TMC_PARAMS_SPI
#define FOREACH_TMC9660_SPI_PARAM(DO) \
        DO(SPI_ENCODE_CS_SETTLE_DELAY_TIME, 0) \
        DO(SPI_ENCODER_CS_IDLE_DELAY_TIME, 0) \
        DO(SPI_ENCODER_MAIN_TRANSFER_CMD_SIZE, 0) \
        DO(SPI_ENCODER_SECONDARY_TRANSFER_CMD_SIZE, 0) \
        DO(SPI_ENCODER_TRANSFER_DATA_3_0, 0) \
        DO(SPI_ENCODER_TRANSFER_DATA_7_4, 0) \
        DO(SPI_ENCODER_TRANSFER_DATA_11_8, 0) \
        DO(SPI_ENCODER_TRANSFER_DATA_15_12, 0) \
        DO(SPI_ENCODER_TRANSFER, 0) \
        DO(SPI_ENCODER_POSITION_COUNTER_MASK, 0) \
        DO(SPI_ENCODER_POSITION_COUNTER_SHIFT, 0) \
        DO(SPI_ENCODER_INITIALIZATION_METHOD, 0) \
        DO(SPI_ENCODER_DIRECTION, 0) \
        DO(SPI_ENCODER_OFFSET, 0) \
    DO(SPI_LUT_CORRECTION_ENABLE, 0)
#else
#define FOREACH_TMC9660_SPI_PARAM(DO)
#endif

#ifdef CONFIG_TMC_PARAMS_HALL
#define FOREACH_TMC9660_HALL_PARAM(DO) \
        DO(HALL_SECTOR_OFFSET, 0) \
        DO(HALL_FILTER_LENGTH, 0) \
        DO(HALL_POSITION_0_OFFSET, 0) \
        DO(HALL_POSITION_60_OFFSET, 10922) \
        DO(HALL_POSITION_120_OFFSET, 21845) \
        DO(HALL_POSITION_180_OFFSET, -32768) \
        DO(HALL_POSITION_240_OFFSET, -21846) \
        DO(HALL_POSITION_300_OFFSET, -10923) \
        DO(HALL_INVERT_DIRECTION, 0) \
        DO(HALL_EXTRAPOLATION_ENABLE, 0) \
        DO(HALL_PHI_E_OFFSET, 0)
#else
#define FOREACH_TMC9660_HALL_PARAM(DO)
#endif

#ifdef CONFIG_TMC_PARAMS_STEPDIR
#define FOREACH_TMC9660_STEPDIR_PARAM(DO) \
        DO(STEP_DIR_STEP_DIVIDER_SHIFT, 0)
#else
#define FOREACH_TMC9660_STEPDIR_PARAM(DO)
#endif


#define FOREACH_TMC9660_RWE_PARAM(DO) \
        DO(MOTOR_TYPE, 2) \
        DO(MOTOR_POLE_PAIRS, 50) \
        DO(MOTOR_DIRECTION, 0) \
        DO(MOTOR_PWM_FREQUENCY, 50000) \
        DO(OUTPUT_VOLTAGE_LIMIT, 20000) \
        DO(MAX_TORQUE, 2500) \
        DO(MAX_FLUX, 2500) \
        DO(PWM_SWITCHING_SCHEME, 1) \
        DO(IDLE_MOTOR_PWM_BEHAVIOR, 1) \
        DO(ADC_SHUNT_TYPE, 4) \
        DO(CSA_GAIN_ADC_I0_TO_ADC_I2, 1) \
        DO(CSA_GAIN_ADC_I3, 1) \
        DO(CSA_FILTER_ADC_I0_TO_ADC_I2, 0) \
        DO(CSA_FILTER_ADC_I3, 0) \
        DO(CURRENT_SCALING_FACTOR, 390) \
        DO(PHASE_UX1_ADC_MAPPING, 0) \
        DO(PHASE_VX2_ADC_MAPPING, 1) \
        DO(PHASE_WY1_ADC_MAPPING, 2) \
        DO(PHASE_Y2_ADC_MAPPING, 3) \
        DO(ADC_I0_SCALE, 1024) \
        DO(ADC_I1_SCALE, 1024) \
        DO(ADC_I2_SCALE, 1024) \
        DO(ADC_I3_SCALE, 1024) \
        DO(ADC_I0_INVERTED, 1) \
        DO(ADC_I1_INVERTED, 0) \
        DO(ADC_I2_INVERTED, 1) \
        DO(ADC_I3_INVERTED, 0) \
        DO(ADC_I0_OFFSET, -1) \
        DO(ADC_I1_OFFSET, 11) \
        DO(ADC_I2_OFFSET, -26) \
        DO(ADC_I3_OFFSET, -12) \
        DO(OPENLOOP_CURRENT, 500) \
        DO(OPENLOOP_VOLTAGE, 1638) \
        DO(ACCELERATION_FF_GAIN, 8) \
        DO(ACCELERATION_FF_SHIFT, 4) \
        DO(RAMP_ENABLE, 1) \
        DO(DIRECT_VELOCITY_MODE, 1) \
        DO(RAMP_AMAX, 50000) \
        DO(RAMP_A1, 50000) \
        DO(RAMP_A2, 50000) \
        DO(RAMP_DMAX, 50000) \
        DO(RAMP_D1, 50000) \
        DO(RAMP_D2, 50000) \
        DO(RAMP_VMAX, 4375000 /* 1m/s */) \
        DO(RAMP_V1, 0) \
        DO(RAMP_V2, 0) \
        DO(RAMP_VSTART, 0) \
        DO(RAMP_VSTOP, 1) \
        DO(RAMP_TVMAX, 0) \
        DO(RAMP_TZEROWAIT, 0) \
        DO(ACCELERATION_FEEDFORWARD_ENABLE, 0) \
        DO(VELOCITY_FEEDFORWARD_ENABLE, 1) \
        FOREACH_TMC9660_HALL_PARAM(DO) \
        FOREACH_TMC9660_ABN1_PARAM(DO) \
        DO(TORQUE_P, 840) \
        DO(TORQUE_I, 2340) \
        DO(FLUX_P, 840) \
        DO(FLUX_I, 2340) \
        DO(SEPARATE_TORQUE_FLUX_PI_PARAMTERS, 0) \
        DO(CURRENT_NORM_P, 0) \
        DO(CURRENT_NORM_I, 1) \
        DO(VELOCITY_SENSOR_SELECTION, 0) \
        DO(VELOCITY_P, 300) \
        DO(VELOCITY_I, 0) \
        DO(VELOCITY_NORM_P, 1) \
        DO(VELOCITY_NORM_I, 1) \
        DO(VELOCITY_SCALING_FACTOR, 1) \
        DO(VELOCITY_LOOP_DOWNSAMPLING, 5) \
        DO(VELOCITY_REACHED_THRESHOLD, 100000) \
        DO(VELOCITY_METER_SWITCH_THRESHOLD, 2000) \
        DO(VELOCITY_METER_SWITCH_HYSTERESIS, 500) \
        DO(POSITION_SENSOR_SELECTION, 0) \
        DO(POSITION_SCALING_FACTOR, 1024) \
        DO(POSITION_P, 30) \
        DO(POSITION_I, 0) \
        DO(POSITION_NORM_P, 0) \
        DO(POSITION_NORM_I, 1) \
        DO(STOP_ON_POSITION_DEVIATION, 0) \
        DO(POSITION_LOOP_DOWNSAMPLING, 0) \
        DO(POSITION_LIMIT_LOW, -2147483648) \
        DO(POSITION_LIMIT_HIGH, 2147483647) \
        DO(POSITION_REACHED_THRESHOLD, 0) \
        DO(EVENT_STOP_SETTINGS, 0) \
        FOREACH_TMC9660_REFSW_PARAM(DO) \
        FOREACH_TMC9660_ABN2_PARAM(DO) \
        FOREACH_TMC9660_SPI_PARAM(DO) \
        FOREACH_TMC9660_STEPDIR_PARAM(DO) \
        DO(BRAKE_CHOPPER_ENABLE, 0) \
        DO(BRAKE_CHOPPER_VOLTAGE_LIMIT, 260) \
        DO(BRAKE_CHOPPER_HYSTERESIS, 5) \
        DO(RELEASE_BRAKE, 0) \
        DO(BRAKE_RELEASING_DUTY_CYCLE, 75) \
        DO(BRAKE_HOLDING_DUTY_CYCLE, 11) \
        DO(BRAKE_RELEASING_DURATION, 80) \
        DO(INVERT_BRAKE_OUTPUT, 0) \
        DO(THERMAL_WINDING_TIME_CONSTANT_1, 3000) \
        DO(IIT_LIMIT_1, -1) \
        DO(THERMAL_WINDING_TIME_CONSTANT_2, 6000) \
        DO(IIT_LIMIT_2, -1) \
        DO(PWM_L_OUTPUT_POLARITY, 0) \
        DO(PWM_H_OUTPUT_POLARITY, 0) \
        DO(BREAK_BEFORE_MAKE_TIME_LOW_UVW, 0) \
        DO(BREAK_BEFORE_MAKE_TIME_HIGH_UVW, 0) \
        DO(BREAK_BEFORE_MAKE_TIME_LOW_Y2, 0) \
        DO(BREAK_BEFORE_MAKE_TIME_HIGH_Y2, 0) \
        DO(USE_ADAPTIVE_DRIVE_TIME_UVW, 1) \
        DO(USE_ADAPTIVE_DRIVE_TIME_Y2, 1) \
        DO(DRIVE_TIME_SINK_UVW, 40) \
        DO(DRIVE_TIME_SOURCE_UVW, 40) \
        DO(DRIVE_TIME_SINK_Y2, 40) \
        DO(DRIVE_TIME_SOURCE_Y2, 40) \
        /*DO(UVW_SINK_CURRENT, 10) \
        DO(UVW_SOURCE_CURRENT, 5) \
        DO(Y2_SINK_CURRENT, 10) \
        DO(Y2_SOURCE_CURRENT, 5)*/ \
        DO(BOOTSTRAP_CURRENT_LIMIT, 7) \
        DO(UNDERVOLTAGE_PROTECTION_SUPPLY_LEVEL, 0) \
        DO(UNDERVOLTAGE_PROTECTION_VDRV_ENABLE, 1) \
        DO(UNDERVOLTAGE_PROTECTION_BST_UVW_ENABLE, 0 /*XXX 1*/) \
        DO(UNDERVOLTAGE_PROTECTION_BST_Y2_ENABLE, 0 /*XXX 1*/) \
        DO(OVERCURRENT_PROTECTION_UVW_LOW_SIDE_ENABLE, 1) \
        DO(OVERCURRENT_PROTECTION_UVW_HIGH_SIDE_ENABLE, 1) \
        DO(OVERCURRENT_PROTECTION_Y2_LOW_SIDE_ENABLE, 1) \
        DO(OVERCURRENT_PROTECTION_Y2_HIGH_SIDE_ENABLE, 1) \
        DO(OVERCURRENT_PROTECTION_UVW_LOW_SIDE_THRESHOLD, 1) \
        DO(OVERCURRENT_PROTECTION_UVW_HIGH_SIDE_THRESHOLD, 2) \
        DO(OVERCURRENT_PROTECTION_Y2_LOW_SIDE_THRESHOLD, 1) \
        DO(OVERCURRENT_PROTECTION_Y2_HIGH_SIDE_THRESHOLD, 2) \
        DO(OVERCURRENT_PROTECTION_UVW_LOW_SIDE_BLANKING, 2) \
        DO(OVERCURRENT_PROTECTION_UVW_HIGH_SIDE_BLANKING, 2) \
        DO(OVERCURRENT_PROTECTION_Y2_LOW_SIDE_BLANKING, 2) \
        DO(OVERCURRENT_PROTECTION_Y2_HIGH_SIDE_BLANKING, 2) \
        DO(OVERCURRENT_PROTECTION_UVW_LOW_SIDE_DEGLITCH, 6) \
        DO(OVERCURRENT_PROTECTION_UVW_HIGH_SIDE_DEGLITCH, 6) \
        DO(OVERCURRENT_PROTECTION_Y2_LOW_SIDE_DEGLITCH, 6) \
        DO(OVERCURRENT_PROTECTION_Y2_HIGH_SIDE_DEGLITCH, 6) \
        DO(OVERCURRENT_PROTECTION_UVW_LOW_SIDE_USE_VDS, 0) \
        DO(OVERCURRENT_PROTECTION_Y2_LOW_SIDE_USE_VDS, 0) \
        DO(VGS_SHORT_ON_PROTECTION_UVW_LOW_SIDE_ENABLE, 1) \
        DO(VGS_SHORT_OFF_PROTECTION_UVW_LOW_SIDE_ENABLE, 1) \
        DO(VGS_SHORT_ON_PROTECTION_UVW_HIGH_SIDE_ENABLE, 1) \
        DO(VGS_SHORT_OFF_PROTECTION_UVW_HIGH_SIDE_ENABLE, 1) \
        DO(VGS_SHORT_ON_PROTECTION_Y2_LOW_SIDE_ENABLE, 1) \
        DO(VGS_SHORT_OFF_PROTECTION_Y2_LOW_SIDE_ENABLE, 1) \
        DO(VGS_SHORT_ON_PROTECTION_Y2_HIGH_SIDE_ENABLE, 1) \
        DO(VGS_SHORT_OFF_PROTECTION_Y2_HIGH_SIDE_ENABLE, 1) \
        DO(VGS_SHORT_PROTECTION_UVW_BLANKING, 1) \
        DO(VGS_SHORT_PROTECTION_Y2_BLANKING, 1) \
        DO(VGS_SHORT_PROTECTION_UVW_DEGLITCH, 1) \
        DO(VGS_SHORT_PROTECTION_Y2_DEGLITCH, 1) \
        DO(GDRV_RETRY_BEHAVIOUR, 0) \
        DO(DRIVE_FAULT_BEHAVIOUR, 0) \
        DO(FAULT_HANDLER_NUMBER_OF_RETRIES, 5) \
        DO(SUPPLY_OVERVOLTAGE_WARNING_THRESHOLD, 710) \
        DO(SUPPLY_UNDERVOLTAGE_WARNING_THRESHOLD, 78) \
        DO(EXTERNAL_TEMPERATURE_SHUTDOWN_THRESHOLD, 65535) \
        DO(EXTERNAL_TEMPERATURE_WARNING_THRESHOLD, 65535) \
        DO(CHIP_TEMPERATURE_SHUTDOWN_THRESHOLD, 65535) \
        DO(CHIP_TEMPERATURE_WARNING_THRESHOLD, 65535) \
        DO(FIELDWEAKENING_I, 0) \
        DO(FIELDWEAKENING_VOLTAGE_THRESHOLD, 32767) \
        DO(TARGET_TORQUE_BIQUAD_FILTER_ENABLE, 0) \
        DO(TARGET_TORQUE_BIQUAD_FILTER_ACOEFF_1, 0) \
        DO(TARGET_TORQUE_BIQUAD_FILTER_ACOEFF_2, 0) \
        DO(TARGET_TORQUE_BIQUAD_FILTER_BCOEFF_0, 1048576) \
        DO(TARGET_TORQUE_BIQUAD_FILTER_BCOEFF_1, 0) \
        DO(TARGET_TORQUE_BIQUAD_FILTER_BCOEFF_2, 0) \
        DO(ACTUAL_VELOCITY_BIQUAD_FILTER_ENABLE, 1) \
        DO(ACTUAL_VELOCITY_BIQUAD_FILTER_ACOEFF_1, 1038090) \
        DO(ACTUAL_VELOCITY_BIQUAD_FILTER_ACOEFF_2, 0) \
        DO(ACTUAL_VELOCITY_BIQUAD_FILTER_BCOEFF_0, 3491) \
        DO(ACTUAL_VELOCITY_BIQUAD_FILTER_BCOEFF_1, 3491) \
        DO(ACTUAL_VELOCITY_BIQUAD_FILTER_BCOEFF_2, 3491)

#define PARAM_ID_LIST_ENTRY(PARAM, DEFAULT) PARAM, // Use enum value from tmc9660_params.h
#define PARAM_NAME_WITH_NULL(PARAM, DEFAULT) #PARAM "\0"
#define PARAM_DEFAULT_VALUE(PARAM, DEFAULT) DEFAULT,

static uint16_t tmc9660_rwe_param_ids[] = {
    FOREACH_TMC9660_RWE_PARAM(PARAM_ID_LIST_ENTRY)
};
static size_t tmc9660_rwe_param_count = ARRAY_SIZE(tmc9660_rwe_param_ids);

#ifdef CONFIG_TMC_RWE_PARAM_NAMES
static char tmc9660_rwe_param_names[] = FOREACH_TMC9660_RWE_PARAM(PARAM_NAME_WITH_NULL);
#endif /* CONFIG_TMC_RWE_PARAM_NAMES */

 // Buffer used as an intermediary writable location when moving data from TMC to flash
static int32_t tmc9660_rwe_param_rambuf[ARRAY_SIZE(tmc9660_rwe_param_ids)] = {
	FOREACH_TMC9660_RWE_PARAM(PARAM_DEFAULT_VALUE)
};

int tmc_params_save(const struct device *dev) {
    int ret = 0;
    for(int i = 0; i < tmc9660_rwe_param_count; i++) {
        if(tmc9660_get_param(dev, tmc9660_rwe_param_ids[i], &tmc9660_rwe_param_rambuf[i]))
            ret = -1;
    }

	// Commit to flash
	ret = storage_put(TMC9660_PARAMS_STORAGE, tmc9660_rwe_param_rambuf, sizeof(tmc9660_rwe_param_rambuf));
	if(ret) {
		LOG_ERR("Could not save TMC9660 RWE params to flash: %d", ret);
	}

    return ret;
}

int tmc_params_load(const struct device *dev) {
    int ret = 0;

	/*ret = storage_get(TMC9660_PARAMS_STORAGE, tmc9660_rwe_param_rambuf, sizeof(tmc9660_rwe_param_rambuf));
	if(ret) {
		LOG_ERR("Could not get TMC9660 RWE params from flash: %d", ret);
	}*/ // BODGEEEEE

    #ifdef CONFIG_TMC_RWE_PARAM_NAMES
    char *name_ptr = tmc9660_rwe_param_names;
    #endif
    for(int i = 0; i < tmc9660_rwe_param_count; i++) {
        int ret = tmc9660_set_param_retry(dev, tmc9660_rwe_param_ids[i], tmc9660_rwe_param_rambuf[i], 3);

        if(ret) {
#ifdef CONFIG_TMC_RWE_PARAM_NAMES
            LOG_ERR("Could not set parameter %s = %d", name_ptr, tmc9660_rwe_param_rambuf[i]);
#else
            LOG_ERR("Could not set parameter %d = %d", tmc9660_rwe_param_ids[i], tmc9660_rwe_param_rambuf[i]);
#endif      
            ret = -1;
        }

#ifdef CONFIG_TMC_RWE_PARAM_NAMES
        while(*name_ptr++); // Advance to next string
#endif
    }

    return ret;
}

#ifdef CONFIG_SHELL_TMC_PARAMS

extern struct device *g_tmc9660;

int cmd_tmc_sap(struct shell *sh, int argc, char **argv) {
    int param_id = atoi(argv[1]);
    int param_val = atoi(argv[2]);
    return tmc9660_set_param(g_tmc9660, param_id, param_val);
}

int cmd_tmc_gap(struct shell *sh, int argc, char **argv) {
    int param_id = atoi(argv[1]);
    int val = 0;
    int ret = tmc9660_get_param(g_tmc9660, param_id, &val);
    if(ret == 0)
        shell_print(sh, "%d", val);
    else
        shell_error(sh, "Error %d", ret);
    return ret;
}

int cmd_tmc_rwe_show(struct shell *sh, int argc, char **argv) {
	int ret = 0;

	ret = storage_get(TMC9660_PARAMS_STORAGE, tmc9660_rwe_param_rambuf, sizeof(tmc9660_rwe_param_rambuf));
	if(ret) {
		LOG_ERR("Could not get TMC9660 RWE params from flash: %d", ret);
	}

    #ifdef CONFIG_TMC_RWE_PARAM_NAMES
    char *name_ptr = tmc9660_rwe_param_names;
    #endif
    for(int i = 0; i < tmc9660_rwe_param_count; i++) {
        int param_id = tmc9660_rwe_param_ids[i];
        int flash_value = tmc9660_rwe_param_rambuf[i];
        int tmc_value = 0xcafebabe;
        int ret = tmc9660_get_param(g_tmc9660, param_id, &tmc_value);

        if(ret) {
#ifdef CONFIG_TMC_RWE_PARAM_NAMES
            shell_warn(sh, "Error %d reading %s", ret, name_ptr);
#else
            shell_warn(sh, "Error %d reading %d", ret, param_id);
#endif
        }

        if(flash_value == tmc_value) {
#ifdef CONFIG_TMC_RWE_PARAM_NAMES
            shell_print(sh, "%s = %d", name_ptr, tmc_value);
#else
            shell_print(sh, "%d = %d", param_id, tmc_value);
#endif
        } else {
#ifdef CONFIG_TMC_RWE_PARAM_NAMES
            shell_warn(sh, "%s = %d (saved: %d)", name_ptr, tmc_value, flash_value);
#else
            shell_warn(sh, "%d = %d (saved: %d)", param_id, tmc_value, flash_value);
#endif
        }

#ifdef CONFIG_TMC_RWE_PARAM_NAMES
        while(*name_ptr++); // Advance to next string
#endif
    }

    return 0;
}

int cmd_tmc_rwe_load(struct shell *sh, int argc, char **argv) {
    tmc_params_load(g_tmc9660);
    cmd_tmc_rwe_show(sh, argc, argv);
    return 0;
}

int cmd_tmc_rwe_save(struct shell *sh, int argc, char **argv) {
    tmc_params_save(g_tmc9660);
    cmd_tmc_rwe_show(sh, argc, argv);
    return 0;
}

int cmd_tmc_flags(struct shell *sh, int argc, char **argv) {
	int flags = 0;
	tmc9660_get_param(g_tmc9660, GENERAL_STATUS_FLAGS, &flags);

	if(flags & 1) shell_print(sh, "REGULATION_STOPPED");
	if(flags & 2) shell_print(sh, "REGULATION_TORQUE");
	if(flags & 4) shell_print(sh, "REGULATION_VELOCITY");
	if(flags & 8) shell_print(sh, "REGULATION_POSITION");
	if(flags & 16) shell_print(sh, "CONFIG_STORED");
	if(flags & 32) shell_print(sh, "CONFIG_LOADED");
	if(flags & 64) shell_print(sh, "CONFIG_READ_ONLY");
	if(flags & 128) shell_print(sh, "TMCL_SCRIPT_READ_ONLY");
	if(flags & 256) shell_print(sh, "BRAKE_CHOPPER_ACTIVE");
	if(flags & 512) shell_print(sh, "POSITION_REACHED");
	if(flags & 1024) shell_print(sh, "VELOCITY_REACHED");
	if(flags & 2048) shell_print(sh, "ADC_OFFSET_CALIBRATED");
	if(flags & 4096) shell_print(sh, "RAMPER_LATCHED");
	if(flags & 8192) shell_print(sh, "RAMPER_EVENT_STOP_SWITCH");
	if(flags & 16384) shell_print(sh, "RAMPER_EVENT_STOP_DEVIATION");
	if(flags & 32768) shell_print(sh, "RAMPER_VELOCITY_REACHED");
	if(flags & 65536) shell_print(sh, "RAMPER_POSITION_REACHED");
	if(flags & 131072) shell_print(sh, "RAMPER_SECOND_MOVE");
	if(flags & 262144) shell_print(sh, "IIT_1_ACTIVE");
	if(flags & 524288) shell_print(sh, "IIT_2_ACTIVE");
	if(flags & 1048576) shell_print(sh, "REFSEARCH_FINISHED");
	if(flags & 2097152) shell_print(sh, "Y2_USED_FOR_BRAKING");
	if(flags & 4194304) shell_print(sh, "FLASH_STIMULUS_AVAILABLE");
	if(flags & 8388608) shell_print(sh, "STEPDIR_INPUT_AVAILABLE");
	if(flags & 16777216) shell_print(sh, "RIGHT_REF_SWITCH_AVAILABLE");
	if(flags & 33554432) shell_print(sh, "HOME_REF_SWITCH_AVAILABLE");
	if(flags & 67108864) shell_print(sh, "LEFT_REF_SWITCH_AVAILABLE");
	if(flags & 134217728) shell_print(sh, "ABN2_FEEDBACK_AVAILABLE");
	if(flags & 268435456) shell_print(sh, "HALL_FEEDBACK_AVAILABLE");
	if(flags & 536870912) shell_print(sh, "ABN1_FEEDBACK_AVAILABLE");
	if(flags & 1073741824) shell_print(sh, "SPI_FLASH_AVAILABLE");
	if(flags & 2147483648) shell_print(sh, "I2C_EEPROM_AVAILABLE");

	return 0;
}

int cmd_tmc_boot(struct shell *sh, int argc, char **argv) {
    int ret = tmc9660_tmcl_command(g_tmc9660, 242, 0x81, 0x92, 0xa3b4c5d6, NULL);

    shell_print(sh, "BOOT returned %d", ret);

    return ret;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_tmc,
    SHELL_CMD_ARG(gap, NULL, "Get parameter", cmd_tmc_gap, 2, 0),
    SHELL_CMD_ARG(sap, NULL, "Set parameter", cmd_tmc_sap, 3, 0),
    SHELL_CMD_ARG(flags, NULL, "Show current flags", cmd_tmc_flags, 0, 0),
    SHELL_CMD_ARG(rwe_show, NULL, "Show RWE parameters", cmd_tmc_rwe_show, 0, 1),
    SHELL_CMD(rwe_save, NULL, "Save all current parameters to flash (run this after tuning)", cmd_tmc_rwe_save),
    SHELL_CMD(rwe_load, NULL, "Load all RWE parameters from flash", cmd_tmc_rwe_load),
    SHELL_CMD(boot, NULL, "Exit parameter mode to bootloader", cmd_tmc_boot),
    SHELL_SUBCMD_SET_END
);
SHELL_CMD_REGISTER(tmc, &sub_tmc, "TMC9660 commands", NULL);

#endif /* CONFIG_SHELL_TMC_PARAMS */
