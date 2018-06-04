/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2018 Siemens AG
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

#include <device/i2c_bus.h>
#include <device/device.h>
#include <console/console.h>
#include "pca9538.h"
#include "chip.h"

/* This function can be used from outside the chip driver to read input. */
uint8_t pca9538_read_input(void)
{
	struct device *dev = pca9538_get_dev();

	if (!dev)
		return 0;
	else
		return (uint8_t)(i2c_dev_readb_at(dev, INPUT_REG));
}

/* This function can be used from outside the chip driver to set output. */
void pca9538_set_output(uint8_t val)
{
	struct device *dev = pca9538_get_dev();

	if (dev)
		i2c_dev_writeb_at(dev, OUTPUT_REG, val);
}

static void pca9538_init(struct device *dev)
{
	struct drivers_i2c_pca9538_config *config = dev->chip_info;

	if (!config)
		return;
	/* Set up registers as requested in devicetree. */
	i2c_dev_writeb_at(dev, INPUT_INVERT_REG, config->invert);
	i2c_dev_writeb_at(dev, OUTPUT_REG, config->out_val);
	i2c_dev_writeb_at(dev, IO_CONFIG_REG, config->in_out);
}

static struct device_operations pca9538_ops = {
	.read_resources		= DEVICE_NOOP,
	.set_resources		= DEVICE_NOOP,
	.enable_resources	= DEVICE_NOOP,
	.init			= pca9538_init,
	.final			= DEVICE_NOOP
};

static void pca9538_enable(struct device *dev)
{
	dev->ops = &pca9538_ops;
}

struct chip_operations drivers_i2c_pca9538_ops = {
	CHIP_NAME("PCA9538")
	.enable_dev = pca9538_enable
};
