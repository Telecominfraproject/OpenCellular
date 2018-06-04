/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2012 Google Inc.
 * Copyright (C) 2015 Intel Corp.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of
 * the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

Scope (\_SB.PCI0.I2C1)
{
	Device (ETSA)
	{
		Name (_HID, "ELAN0001")
		Name (_DDN, "Elan Touchscreen ")
		Name (_UID, 5)
		Name (ISTP, 0) /* TouchScreen */

		Method(_CRS, 0x0, NotSerialized)
		{
			Name(BUF0,ResourceTemplate ()
			{
				I2CSerialBus(
					0x10,                     /* SlaveAddress */
					ControllerInitiated,      /* SlaveMode */
					400000,                   /* ConnectionSpeed */
					AddressingMode7Bit,       /* AddressingMode */
					"\\_SB.PCI0.I2C1",        /* ResourceSource */
				)
				Interrupt (ResourceConsumer, Level, ActiveLow)
				{
					BOARD_TOUCH_IRQ
				}

			} )
			Return (BUF0)
		}

		Method (_STA)
		{
			If (LEqual (\S1EN, 1)) {
				Return (0xF)
			} Else {
				Return (0x0)
			}
		}

		Name (_PRW, Package() { BOARD_TOUCHSCREEN_WAKE_GPIO, 0x3 })

		/* Allow device to power off in S0 */
		Name (_S0W, 4)
	}
}
