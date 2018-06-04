/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2018 Intel Corporation.
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

Scope (\_SB.PCI0.CIO2)
{
        /* Define two endpoints for CIO2 device where endpoint of port0
	is connected to CAM0 and endpoint of port1 is connected to CAM1.
	variant of this poppy baseboard can define their own endpoints (max 2)
	with custom remote-endpoint */

	Name (EP00, Package() {
		ToUUID ("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
		Package () {
			Package () { "endpoint", 0 },
			Package () { "clock-lanes", 0 },
			Package () { "data-lanes", Package () { 1, 2, 3, 4 } },
			Package () { "remote-endpoint",
				Package() { \_SB.PCI0.I2C2.CAM0, 0, 0 }
			},
		}
	})

	Name (EP10, Package() {
		ToUUID ("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
		Package () {
			Package () { "endpoint", 0 },
			Package () { "clock-lanes", 0 },
			Package () { "data-lanes", Package () { 1, 2 } },
			Package () { "remote-endpoint",
				Package() { \_SB.PCI0.I2C4.CAM1, 0, 0 }
			},
		}
	})
}
