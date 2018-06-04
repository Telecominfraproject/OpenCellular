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

/* S1 support: bit 0, S2 Support: bit 1, etc. S0 & S5 assumed */
#if IS_ENABLED(CONFIG_HAVE_ACPI_RESUME)
Name (SSFG, 0x05)
#else
Name (SSFG, 0x01)
#endif

/* Supported sleep states: */
Name(\_S0, Package () {0x00, 0x00, 0x00, 0x00} )	/* (S0) - working state */

If (And(SSFG, 0x01)) {
	Name(\_S1, Package () {0x01, 0x01, 0x01, 0x01} )	/* (S1) - sleeping w/CPU context */
}
If (And(SSFG, 0x02)) {
	Name(\_S2, Package () {0x02, 0x02, 0x02, 0x02} )	/* (S2) - "light" Suspend to RAM */
}
If (And(SSFG, 0x04)) {
	Name(\_S3, Package () {0x05, 0x05, 0x05, 0x05} )	/* (S3) - Suspend to RAM */
}
If (And(SSFG, 0x08)) {
	Name(\_S4, Package () {0x06, 0x06, 0x06, 0x06} )	/* (S4) - Suspend to Disk */
}

Name(\_S5, Package () {0x07, 0x07, 0x07, 0x07} )	/* (S5) - Soft Off */
