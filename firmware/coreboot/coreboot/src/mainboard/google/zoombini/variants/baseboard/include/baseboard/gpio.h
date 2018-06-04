/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2017 Google Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 */

#ifndef __BASEBOARD_GPIO_H__
#define __BASEBOARD_GPIO_H__

#include <soc/gpe.h>
#include <soc/gpio.h>

/* Memory configuration board straps */
#define GPIO_MEM_CONFIG_0	GPP_D3
#define GPIO_MEM_CONFIG_1	GPP_D21
#define GPIO_MEM_CONFIG_2	GPP_D22
#define GPIO_MEM_CONFIG_3	GPP_D0

/* EC in RW */
#define GPIO_EC_IN_RW		GPP_A8

/* BIOS Flash Write Protect */
#define GPIO_PCH_WP		GPP_H12

/* EC wake is LAN_WAKE# which is a special DeepSX wake pin */
#define GPE_EC_WAKE		GPE0_LAN_WAK

/* eSPI virtual wire reporting */
#define EC_SCI_GPI		GPE0_ESPI
#endif
