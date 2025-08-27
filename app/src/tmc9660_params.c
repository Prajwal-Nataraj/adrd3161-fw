/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define OD_DEFINITION
#include <301/CO_ODinterface.h>
#undef OD_DEFINITION

#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>

#ifdef CONFIG_SHELL_TMC_PARAMS
#include <zephyr/shell/shell.h>
#endif /* CONFIG_SHELL_TMC_PARAMS */

#include <app/drivers/misc/tmc9660/tmc9660.h>

#include "OD.h"
#include "tmc9660_params.h"

LOG_MODULE_REGISTER(tmc9660_params, LOG_LEVEL_DBG);

extern struct device *g_tmc9660;

ODR_t tmc_extension_write(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten) {
	__ASSERT(count == 4, "TMC param OD write invalid. Expected size 4, got %d", count);

	int tmc_param_id = stream->index - 0x2000;
	int ret = tmc9660_set_param_retry(g_tmc9660, tmc_param_id, *(int32_t *)buf, 3);
	LOG_DBG("OD write TMC param %d value %d returned %d", tmc_param_id, *(int32_t *)buf, ret);
	*countWritten = 4;

	if(ret == 0) return ODR_OK;
	if(ret == -EINVAL) return ODR_INVALID_VALUE;
	if(ret == -EIO) return ODR_HW;
	if(ret == -EAGAIN) return ODR_HW; // If 3 retries isn't enough, report hardware error
	return ODR_UNSUPP_ACCESS; // Catch-all for other errors
}

ODR_t tmc_extension_read(OD_stream_t *stream, void *buf, OD_size_t count, OD_size_t *countRead) {
	__ASSERT(count == 4, "TMC param OD read invalid. Expected size 4, got %d", count);

	int tmc_param_id = stream->index - 0x2000;
	int ret = tmc9660_get_param_retry(g_tmc9660, tmc_param_id, buf, 3);
	LOG_DBG("OD read TMC param %d value %d returned %d", tmc_param_id, *(int32_t *)buf, ret);
	*countRead = 4;
	
	if(ret == 0) return ODR_OK;
	if(ret == -EINVAL) return ODR_INVALID_VALUE;
	if(ret == -EIO) return ODR_HW;
	if(ret == -EAGAIN) return ODR_HW; // If 3 retries isn't enough, report hardware error
	return ODR_UNSUPP_ACCESS; // Catch-all for other errors
}

OD_extension_t tmc_params_extensions[OD_CNT_TMC_PARAMS] = { 0 };

void tmc_params_init(void) {
	/* 
	 * Iterate through all CANopen OD entries, mapping 0x2000 - 0x2200 to TMC parameters 0x000 - 0x200.
	 * Set OD extensions with proper read/write handlers based on whether OD entry allows read/write.
	 */

	int ext_idx = 0;
	for(int i = 0; i < OD->size; i++) {
		OD_entry_t *entry = &OD->list[i];
		const OD_obj_var_t *var = (const OD_obj_var_t *) entry->odObject;
		OD_extension_t *extension = &tmc_params_extensions[ext_idx];

		if(entry->index < 0x2000 || entry->index > 0x2200) continue;

		if((var->attribute & ODA_SDO_R) == ODA_SDO_R) {
			extension->read = tmc_extension_read;
		}

		if((var->attribute & ODA_SDO_W) == ODA_SDO_W) {
			extension->write = tmc_extension_write;
		}

		OD_extension_init(entry, extension);
		ext_idx++;
	}
}

#ifdef CONFIG_SHELL_TMC_PARAMS

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
	/* FIXME
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

    return 0; */

	return -1;
}

int cmd_tmc_rwe_load(struct shell *sh, int argc, char **argv) {
	/* FIXME
    tmc_params_load(g_tmc9660);
    cmd_tmc_rwe_show(sh, argc, argv);
    return 0;
    */
	return -1;
}

int cmd_tmc_rwe_save(struct shell *sh, int argc, char **argv) {
	/* FIXME
    tmc_params_save(g_tmc9660);
    cmd_tmc_rwe_show(sh, argc, argv);
    return 0;
    */
	return -1;
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
	int ret = tmc9660_set_param_retry(g_tmc9660, COMMUTATION_MODE, 0, 3); // Stop commutation, see errata 3
	if(ret < 0) {
		shell_print(sh, "Could not stop commutation. NOT entering bootloader!");
		return ret;
	}

	ret = tmc9660_tmcl_command(g_tmc9660, 242, 0x81, 0x92, 0xa3b4c5d6, NULL);
	if(ret != -5) {
		shell_print(sh, "Expected status -5, got %d", ret);
		return -1;
	}

	shell_print(sh, "Entered bootloader! Motor control will not work, expect application errors");
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_tmc,
    SHELL_CMD_ARG(gap, NULL, "Get parameter", cmd_tmc_gap, 2, 0),
    SHELL_CMD_ARG(sap, NULL, "Set parameter", cmd_tmc_sap, 3, 0),
    SHELL_CMD_ARG(flags, NULL, "Show current flags", cmd_tmc_flags, 0, 0),
    // SHELL_CMD_ARG(rwe_show, NULL, "Show RWE parameters", cmd_tmc_rwe_show, 0, 1),
    // SHELL_CMD(rwe_save, NULL, "Save all current parameters to flash (run this after tuning)", cmd_tmc_rwe_save),
    // SHELL_CMD(rwe_load, NULL, "Load all RWE parameters from flash", cmd_tmc_rwe_load),
    SHELL_CMD(boot, NULL, "Exit parameter mode to bootloader", cmd_tmc_boot),
    SHELL_SUBCMD_SET_END
);
SHELL_CMD_REGISTER(tmc, &sub_tmc, "TMC9660 commands", NULL);

#endif /* CONFIG_SHELL_TMC_PARAMS */

