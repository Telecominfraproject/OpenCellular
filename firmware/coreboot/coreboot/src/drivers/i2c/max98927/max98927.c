/*
 * This file is part of the coreboot project.
 *
 * Copyright 2017 Intel Corporation.
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

#include <arch/acpi.h>
#include <arch/acpi_device.h>
#include <arch/acpigen.h>
#include <console/console.h>
#include <device/i2c_simple.h>
#include <device/device.h>
#include <device/path.h>
#include <stdint.h>
#include <string.h>
#include "chip.h"

#define MAX98927_ACPI_NAME	"MAXI"
#define MAX98927_ACPI_HID	"MX98927"

static void max98927_fill_ssdt(struct device *dev)
{
	struct drivers_i2c_max98927_config *config = dev->chip_info;
	const char *scope = acpi_device_scope(dev);
	struct acpi_i2c i2c = {
		.address = dev->path.i2c.device,
		.mode_10bit = dev->path.i2c.mode_10bit,
		.speed = config->bus_speed ? : I2C_SPEED_FAST,
		.resource = scope,
	};
	struct acpi_dp *dp;

	if (!dev->enabled || !scope)
		return;

	/* Device */
	acpigen_write_scope(scope);
	acpigen_write_device(acpi_device_name(dev));
	acpigen_write_name_string("_HID", MAX98927_ACPI_HID);
	acpigen_write_name_integer("_UID", config->uid);
	if (config->desc)
		acpigen_write_name_string("_DDN", config->desc);
	acpigen_write_STA(ACPI_STATUS_DEVICE_ALL_ON);

	/* Resources */
	acpigen_write_name("_CRS");
	acpigen_write_resourcetemplate_header();
	acpi_device_write_i2c(&i2c);
	acpigen_write_resourcetemplate_footer();

	/* Device Properties */
	dp = acpi_dp_new_table("_DSD");

	acpi_dp_add_integer(dp, "interleave_mode", config->interleave_mode);
	acpi_dp_add_integer(dp, "vmon-slot-no", config->vmon_slot_no);
	acpi_dp_add_integer(dp, "imon-slot-no", config->imon_slot_no);

	acpi_dp_write(dp);

	acpigen_pop_len(); /* Device */
	acpigen_pop_len(); /* Scope */

	printk(BIOS_INFO, "%s: %s address 0%xh\n", acpi_device_path(dev),
			dev->chip_ops->name, dev->path.i2c.device);
}

static const char *max98927_acpi_name(const struct device *dev)
{
	struct drivers_i2c_max98927_config *config = dev->chip_info;

	if (config->name)
		return config->name;

	return MAX98927_ACPI_NAME;
}

static struct device_operations max98927_ops = {
	.read_resources		  = DEVICE_NOOP,
	.set_resources		  = DEVICE_NOOP,
	.enable_resources	  = DEVICE_NOOP,
	.acpi_name		  = &max98927_acpi_name,
	.acpi_fill_ssdt_generator = &max98927_fill_ssdt,
};

static void max98927_enable(struct device *dev)
{
	dev->ops = &max98927_ops;
}

struct chip_operations drivers_i2c_max98927_ops = {
	CHIP_NAME("Maxim MAX98927 Codec")
	.enable_dev = &max98927_enable
};
