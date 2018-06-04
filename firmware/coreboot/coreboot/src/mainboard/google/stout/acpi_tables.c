/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2007-2009 coresystems GmbH
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

#include <types.h>
#include <string.h>
#include <cbmem.h>
#include <console/console.h>
#include <arch/acpi.h>
#include <arch/ioapic.h>
#include <arch/acpigen.h>
#include <arch/smp/mpspec.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <vendorcode/google/chromeos/gnvs.h>
#include <bootmode.h>
#include <ec/quanta/it8518/ec.h>
#include "ec.h"
#include "onboard.h"

#include <southbridge/intel/bd82x6x/pch.h>
#include <southbridge/intel/bd82x6x/nvs.h>
#include "thermal.h"

static void acpi_update_thermal_table(global_nvs_t *gnvs)
{
	/* EC handles all thermal and fan control on Stout. */
	gnvs->tcrt = CRITICAL_TEMPERATURE;
	gnvs->tpsv = PASSIVE_TEMPERATURE;
	gnvs->tmax = MAX_TEMPERATURE;
}

void acpi_create_gnvs(global_nvs_t *gnvs)
{
	/* Disable USB ports in S3 by default */
	gnvs->s3u0 = 0;
	gnvs->s3u1 = 0;

	/* Disable USB ports in S5 by default */
	gnvs->s5u0 = 0;
	gnvs->s5u1 = 0;


#if IS_ENABLED(CONFIG_CHROMEOS)
	gnvs->chromeos.vbt2 = get_recovery_mode_switch() ?
			ACTIVE_ECFW_RO : ACTIVE_ECFW_RW;
#endif

	acpi_update_thermal_table(gnvs);

	// the lid is open by default.
	gnvs->lids = 1;

	/* XHCI Mode */
	gnvs->xhci = XHCI_MODE;
}
