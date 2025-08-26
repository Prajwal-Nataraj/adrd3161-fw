/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TMC9660_PARAMS_H_
#define TMC9660_PARAMS_H_

/* Read all RWE parameters from TMC9660 and save them to microcontroller flash */
int tmc_params_load(const struct device *dev);

/* Write all RWE parameters to TMC9660 using values from microcontroller flash */
int tmc_params_save(const struct device *dev);

#endif /* TMC9660_PARAMS_H_ */
