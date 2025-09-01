/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BODGE_H_
#define BODGE_H_

#include <fcr_regs.h>
#include <mxc_sys.h>
#include <mxc_delay.h>

// 2025-09-01 bodge: this will be part of msdk / hal_adi soon
static inline int MXC_SYS_ClockCalibrate(mxc_sys_system_clock_t clock)
{
    if (clock != MXC_SYS_CLOCK_IPO) {
        return E_BAD_PARAM; // Only the IPO supports calibration
    }

    int err = E_NO_ERROR;
    // The following section implements section 4.2.1.1 of the MAX32662 UG Rev 1
    if ((err = MXC_SYS_ClockSourceEnable(MXC_SYS_CLOCK_ERTCO)))
        return err;
    MXC_SETFIELD(MXC_FCR->autocal2, MXC_F_FCR_AUTOCAL2_ACDIV, 3051 << MXC_F_FCR_AUTOCAL2_ACDIV_POS);
    MXC_SETFIELD(MXC_FCR->autocal1, MXC_F_FCR_AUTOCAL1_INITTRIM,
                 0x40 << MXC_F_FCR_AUTOCAL1_INITTRIM_POS);
    MXC_FCR->autocal0 |= 0x7;
    MXC_Delay(MXC_DELAY_MSEC(10)); // Wait 10ms for calibration to complete
    // Calculated trim is loaded to MXC_FCR->hirc96mactmrout and is used by the hardware as long as
    // MXC_FCR->autocal0.acen is set to 1
    MXC_FCR->autocal0 &= ~MXC_F_FCR_AUTOCAL0_ACRUN; // Stop the calibration
    MXC_FCR->autocal0 |= MXC_F_FCR_AUTOCAL0_ACEN; // Enable use of calibration value
    return E_NO_ERROR;
}

static inline void calibrate_oscillator()
{
	MXC_SYS_ClockCalibrate(MXC_SYS_CLOCK_IPO);
}

#endif /* BODGE_H_ */

