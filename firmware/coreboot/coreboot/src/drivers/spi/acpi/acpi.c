/*
 * This file is part of the coreboot project.
 *
 * Copyright 2017 Google Inc.
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

#include <arch/acpi_device.h>
#include <arch/acpigen.h>
#include <console/console.h>
#include <device/device.h>
#include <device/path.h>
#include <device/spi.h>
#include <spi-generic.h>
#include <stdint.h>
#include <string.h>
#include "chip.h"

static int spi_acpi_get_bus(const struct device *dev)
{
	struct device *spi_dev;
	struct device_operations *ops;

	if (!dev->bus || !dev->bus->dev)
		return -1;

	spi_dev = dev->bus->dev;
	ops = spi_dev->ops;

	if (ops && ops->ops_spi_bus &&
	    ops->ops_spi_bus->dev_to_bus)
		return ops->ops_spi_bus->dev_to_bus(spi_dev);

	return -1;
}

static bool spi_acpi_add_gpios_to_crs(struct drivers_spi_acpi_config *config)
{
	/*
	 * Return false if:
	 * 1. Request to explicitly disable export of GPIOs in CRS, or
	 * 2. Both reset and enable GPIOs are not provided.
	 */
	if (config->disable_gpio_export_in_crs ||
	    ((config->reset_gpio.pin_count == 0) &&
	     (config->enable_gpio.pin_count == 0)))
		return false;

	return true;
}

static int spi_acpi_write_gpio(struct acpi_gpio *gpio, int *curr_index)
{
	int ret = -1;

	if (gpio->pin_count == 0)
		return ret;

	acpi_device_write_gpio(gpio);
	ret = *curr_index;
	(*curr_index)++;

	return ret;
}

static void spi_acpi_fill_ssdt_generator(struct device *dev)
{
	struct drivers_spi_acpi_config *config = dev->chip_info;
	const char *scope = acpi_device_scope(dev);
	const char *path = acpi_device_path(dev);
	struct acpi_spi spi = {
		.device_select = dev->path.spi.cs,
		.speed = config->speed ? : 1 * MHz,
		.resource = scope,
		.device_select_polarity = SPI_POLARITY_LOW,
		.wire_mode = SPI_4_WIRE_MODE,
		.data_bit_length = 8,
		.clock_phase = SPI_CLOCK_PHASE_FIRST,
		.clock_polarity = SPI_POLARITY_LOW,
	};
	int curr_index = 0;
	int irq_gpio_index = -1;
	int reset_gpio_index = -1;
	int enable_gpio_index = -1;

	if (!dev->enabled || !scope)
		return;

	if (spi_acpi_get_bus(dev) == -1) {
		printk(BIOS_ERR, "%s: ERROR: Cannot get bus for device.\n",
			dev_path(dev));
		return;
	}

	if (!config->hid) {
		printk(BIOS_ERR, "%s: ERROR: HID required.\n", dev_path(dev));
		return;
	}

	/* Device */
	acpigen_write_scope(scope);
	acpigen_write_device(acpi_device_name(dev));
	acpigen_write_name_string("_HID", config->hid);
	if (config->cid)
		acpigen_write_name_string("_CID", config->cid);
	acpigen_write_name_integer("_UID", config->uid);
	if (config->desc)
		acpigen_write_name_string("_DDN", config->desc);

	/* Resources */
	acpigen_write_name("_CRS");
	acpigen_write_resourcetemplate_header();
	acpi_device_write_spi(&spi);

	/* Use either Interrupt() or GpioInt() */
	if (config->irq_gpio.pin_count)
		irq_gpio_index = spi_acpi_write_gpio(&config->irq_gpio,
						     &curr_index);
	else
		acpi_device_write_interrupt(&config->irq);

	/* Add enable/reset GPIOs if needed */
	if (spi_acpi_add_gpios_to_crs(config)) {
		reset_gpio_index = spi_acpi_write_gpio(&config->reset_gpio,
						       &curr_index);
		enable_gpio_index = spi_acpi_write_gpio(&config->enable_gpio,
							&curr_index);
	}
	acpigen_write_resourcetemplate_footer();

	/* Wake capabilities */
	if (config->wake) {
		acpigen_write_name_integer("_S0W", 4);
		acpigen_write_PRW(config->wake, 3);
	};

	/* Write device properties if needed */
	if (config->compat_string || irq_gpio_index >= 0 ||
	    reset_gpio_index >= 0 || enable_gpio_index >= 0) {
		struct acpi_dp *dsd = acpi_dp_new_table("_DSD");
		if (config->compat_string)
			acpi_dp_add_string(dsd, "compatible",
					   config->compat_string);
		if (irq_gpio_index >= 0)
			acpi_dp_add_gpio(dsd, "irq-gpios", path,
					 irq_gpio_index, 0,
					 config->irq_gpio.polarity);
		if (reset_gpio_index >= 0)
			acpi_dp_add_gpio(dsd, "reset-gpios", path,
					 reset_gpio_index, 0,
					 config->reset_gpio.polarity);
		if (enable_gpio_index >= 0)
			acpi_dp_add_gpio(dsd, "enable-gpios", path,
					 enable_gpio_index, 0,
					 config->enable_gpio.polarity);
		acpi_dp_write(dsd);
	}

	/* Power Resource */
	if (config->has_power_resource) {
		const struct acpi_power_res_params power_res_params = {
			&config->reset_gpio,
			config->reset_delay_ms,
			config->reset_off_delay_ms,
			&config->enable_gpio,
			config->enable_delay_ms,
			config->enable_off_delay_ms,
			&config->stop_gpio,
			config->stop_delay_ms,
			config->stop_off_delay_ms
		};
		acpi_device_add_power_res(&power_res_params);
	}

	acpigen_pop_len(); /* Device */
	acpigen_pop_len(); /* Scope */

	printk(BIOS_INFO, "%s: %s at %s\n", path,
	       config->desc ? : dev->chip_ops->name, dev_path(dev));
}

static const char *spi_acpi_name(const struct device *dev)
{
	struct drivers_spi_acpi_config *config = dev->chip_info;
	static char name[5];

	if (config->name)
		return config->name;

	snprintf(name, sizeof(name), "S%03.3X", spi_acpi_get_bus(dev));
	name[4] = '\0';
	return name;
}

static struct device_operations spi_acpi_ops = {
	.read_resources		  = DEVICE_NOOP,
	.set_resources		  = DEVICE_NOOP,
	.enable_resources	  = DEVICE_NOOP,
	.acpi_name		  = &spi_acpi_name,
	.acpi_fill_ssdt_generator = &spi_acpi_fill_ssdt_generator,
};

static void spi_acpi_enable(struct device *dev)
{
	dev->ops = &spi_acpi_ops;
}

struct chip_operations drivers_spi_acpi_ops = {
	CHIP_NAME("SPI Device")
	.enable_dev = &spi_acpi_enable
};
