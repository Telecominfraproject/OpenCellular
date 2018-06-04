/*
 * This file is part of the coreboot project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <cpu/cpu.h>
#include <device/device.h>

static void qemu_cpu_init(struct device *dev)
{
}

static struct device_operations cpu_dev_ops = {
	.init = qemu_cpu_init,
};

static const struct cpu_device_id cpu_table[] = {
	{ 0, 0 },
};

static const struct cpu_driver driver __cpu_driver = {
	.ops      = &cpu_dev_ops,
	.id_table = cpu_table,
};

struct chip_operations cpu_power8_qemu_ops = {
	CHIP_NAME("QEMU POWER8 CPU")
};
