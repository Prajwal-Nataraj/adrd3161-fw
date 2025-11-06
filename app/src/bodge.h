/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BODGE_H_
#define BODGE_H_

#include <wrap_max32_sys.h>

static inline void calibrate_oscillator()
{
	MXC_SYS_ClockCalibrate(MXC_SYS_CLOCK_IPO);
}

#endif /* BODGE_H_ */

