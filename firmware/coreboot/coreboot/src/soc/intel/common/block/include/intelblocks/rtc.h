/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2017 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef SOC_INTEL_COMMON_BLOCK_RTC_H
#define SOC_INTEL_COMMON_BLOCK_RTC_H

void enable_rtc_upper_bank(void);

/* Expect return rtc failed bootlean in case of coin removal */
int soc_get_rtc_failed(void);

void rtc_init(void);

#endif	/* SOC_INTEL_COMMON_BLOCK_RTC_H */
