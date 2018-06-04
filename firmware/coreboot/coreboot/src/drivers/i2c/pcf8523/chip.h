/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2016 Siemens AG
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

#include "pcf8523.h"

struct drivers_i2c_pcf8523_config {
	unsigned char cap_sel;		/* Internal capacitor selection */
	unsigned char second_int_en;	/* Enable IRQ for seconds */
	unsigned char alarm_int_en;	/* Enable IRQ for alarm */
	unsigned char correction_int_en;/* Enable IRQ for corrections */
	unsigned char wdt_int_en;	/* Enable IRQ for watchdog */
	unsigned char tmrA_int_en;	/* Enable IRQ for timer A */
	unsigned char tmrB_int_en;	/* Enable IRQ for timer B */
	unsigned char power_mode;	/* Set up power mode */
	unsigned char bat_switch_int_en;/* Enable IRQ for battery switch */
	unsigned char bat_low_int_en;	/* Enable IRQ for low battery */
	unsigned char offset_mode;	/* Set up mode how to handle offset */
	unsigned char offset_val;	/* Value for offset adjustment */
	unsigned char tmrA_mode;	/* Operation mode of timer A */
	unsigned char tmrA_int_mode;	/* IRQ mode for timer A */
	unsigned char tmrB_mode;	/* Operation mode for timer B */
	unsigned char tmrB_int_mode;	/* IRQ mode for timer B */
	unsigned char cof_selection;	/* Set up "clock out" frequency */
	unsigned char tmrA_prescaler;	/* Prescaler for timer A */
	unsigned char tmrB_prescaler;	/* Prescaler for timer B */
	unsigned char tmrB_pulse_cfg;	/* Pulse width config for timer B */
	unsigned char set_user_date;	/* Use user date from device tree */
	unsigned char user_year;	/* User year to set */
	unsigned char user_month;	/* User month to set */
	unsigned char user_day;		/* User day to set */
	unsigned char user_weekday;	/* User weekday to set */
};
