/*
 * This file is part of the coreboot project.
 *
 * Copyright 2014 Rockchip Inc.
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

#ifndef __COREBOOT_SRC_SOC_ROCKCHIP_COMMON_INCLUDE_SOC_PWM_H
#define __COREBOOT_SRC_SOC_ROCKCHIP_COMMON_INCLUDE_SOC_PWM_H

void pwm_init(u32 id, u32 period_ns, u32 duty_ns);

#endif  /* ! __COREBOOT_SRC_SOC_ROCKCHIP_COMMON_INCLUDE_SOC_PWM_H */
