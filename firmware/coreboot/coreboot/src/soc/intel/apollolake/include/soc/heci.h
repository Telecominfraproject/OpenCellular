/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2016 Intel Corp.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _SOC_APOLLOLAKE_HECI_H_
#define _SOC_APOLLOLAKE_HECI_H_

enum sec_status {
	SEC_STATE_RESET = 0,
	SEC_STATE_INIT,
	SEC_STATE_RECOVERY,
	SEC_STATE_UNKNOWN0,
	SEC_STATE_UNKNOWN1,
	SEC_STATE_NORMAL,
	SEC_STATE_DISABLE_WAIT,
	SEC_STATE_TRANSITION,
	SEC_STATE_INVALID_CPU
};

#define REG_SEC_FW_STS0					0x40
#define MASK_SEC_FIRMWARE_COMPLETE			(1 << 9)
#define MASK_SEC_STATUS					0xf

/* Read Firmware Status register */
uint32_t heci_fw_sts(void);
/* Returns true if CSE is in normal status */
bool heci_cse_normal(void);
/* Returns true if CSE is done with whatever it was doing */
bool heci_cse_done(void);

#endif
