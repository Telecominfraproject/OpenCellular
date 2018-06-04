/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2016 Google Inc.
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

#include <arch/io.h>
#include <cpu/x86/smm.h>
#include <ec/google/chromeec/smm.h>
#include <gpio.h>
#include <soc/smm.h>
#include "ec.h"
#include "gpio.h"

void mainboard_smi_espi_handler(void)
{
	chromeec_smi_process_events();
}

static void mainboard_gpio_smi_sleep(u8 slp_typ)
{
	/* Power down the rails on any sleep type */
	gpio_set(EN_PP3300_DX_CAM, 0);

	/* Assert TOUSHCREEN_STOP_L */
	gpio_set(GPP_E11, 0);

	/* Turn off touchscreen power */
	gpio_set(GPP_C22, 0);
}

void mainboard_smi_sleep(u8 slp_typ)
{
	chromeec_smi_sleep(slp_typ, MAINBOARD_EC_S3_WAKE_EVENTS,
			   MAINBOARD_EC_S5_WAKE_EVENTS);
	chromeec_smi_device_event_sleep(slp_typ,
					MAINBOARD_EC_S3_DEVICE_EVENTS, 0);
	mainboard_gpio_smi_sleep(slp_typ);
}

int mainboard_smi_apmc(u8 apmc)
{
	chromeec_smi_apmc(apmc, MAINBOARD_EC_SCI_EVENTS,
			  MAINBOARD_EC_SMI_EVENTS);
	return 0;
}
