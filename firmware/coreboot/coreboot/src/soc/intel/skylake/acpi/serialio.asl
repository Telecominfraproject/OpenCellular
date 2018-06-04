/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2015 Google Inc.
 * Copyright (C) 2015 Intel Corporation.
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

/* Intel Serial IO Devices */

Device (I2C0)
{
	Name (_ADR, 0x00150000)
	Name (_DDN, "Serial IO I2C Controller 0")
}

Device (I2C1)
{
	Name (_ADR, 0x00150001)
	Name (_DDN, "Serial IO I2C Controller 1")
}

Device (I2C2)
{
	Name (_ADR, 0x00150002)
	Name (_DDN, "Serial IO I2C Controller 2")
}

Device (I2C3)
{
	Name (_ADR, 0x00150003)
	Name (_DDN, "Serial IO I2C Controller 3")
}

Device (I2C4)
{
	Name (_ADR, 0x00190002)
	Name (_DDN, "Serial IO I2C Controller 4")
}

Device (I2C5)
{
	Name (_ADR, 0x00190001)
	Name (_DDN, "Serial IO I2C Controller 5")
}

Device (SPI0)
{
	Name (_ADR, 0x001E0002)
	Name (_DDN, "Serial IO SPI Controller 0")
}

Device (SPI1)
{
	Name (_ADR, 0x001E0003)
	Name (_DDN, "Serial IO SPI Controller 1")
}

Device (UAR0)
{
	Name (_ADR, 0x001E0000)
	Name (_DDN, "Serial IO UART Controller 0")
}

Device (UAR1)
{
	Name (_ADR, 0x001E0001)
	Name (_DDN, "Serial IO UART Controller 1")
}

Device (UAR2)
{
	Name (_ADR, 0x00190000)
	Name (_DDN, "Serial IO UART Controller 2")
}
