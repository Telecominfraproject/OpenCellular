/*
 * This file is part of the coreboot project.
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

#define DPTF_CPU_PASSIVE	90
#define DPTF_CPU_CRITICAL	100

#define DPTF_TSR0_SENSOR_ID	1
#define DPTF_TSR0_SENSOR_NAME	"TMP432_Internal"
#define DPTF_TSR0_PASSIVE	53
#define DPTF_TSR0_CRITICAL	80

#define DPTF_TSR1_SENSOR_ID	2
#define DPTF_TSR1_SENSOR_NAME	"TMP432_Power_top"
#define DPTF_TSR1_PASSIVE	80
#define DPTF_TSR1_CRITICAL	90

#define DPTF_TSR2_SENSOR_ID	3
#define DPTF_TSR2_SENSOR_NAME	"TMP432_CPU_bottom"
#define DPTF_TSR2_PASSIVE	53
#define DPTF_TSR2_CRITICAL	100

#define DPTF_ENABLE_CHARGER

/* Charger performance states, board-specific values from charger and EC */
Name (CHPS, Package () {
	Package () { 0, 0, 0, 0, 255, 0x7d0, "mA", 0 }, /* 2.0A */
	Package () { 0, 0, 0, 0, 24, 0x6a4, "mA", 0 },  /* 1.7A */
	Package () { 0, 0, 0, 0, 16, 0x578, "mA", 0 },  /* 1.4A */
	Package () { 0, 0, 0, 0, 8, 0x3e8, "mA", 0 },   /* 1.0A */
	Package () { 0, 0, 0, 0, 0, 0x258, "mA", 0 },   /* 0.6A */
})
