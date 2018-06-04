/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2015 Matt DeVillier <matt.devillier@gmail.com>
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

#ifndef SUPERIO_NUVOTON_NCT6779D_H
#define SUPERIO_NUVOTON_NCT6779D_H

/* Logical Device Numbers (LDN). */
#define NCT6779D_PP			0x01 /* Parallel port */
#define NCT6779D_SP1		0x02 /* Com1 */
#define NCT6779D_SP2		0x03 /* Com2 & IR */
#define NCT6779D_KBC		0x05 /* PS/2 keyboard and mouse */
#define NCT6779D_CIR		0x06 /* Consumer IR */
#define NCT6779D_GPIO678_V	0x07 /* GPIO 6/7/8 */
#define NCT6779D_WDT1_GPIO01_V	0x08 /* WDT1, GPIO 0/1 */
#define NCT6779D_GPIO12345678_V	0x09 /* GPIO 1/2/3/4/5/6/7/8 */
#define NCT6779D_ACPI		0x0A /* ACPI */
#define NCT6779D_HWM_FPLED	0x0B /* Hardware monitor & front LED */
#define NCT6779D_WDT1		0x0D /* Watchdog timer 1 */
#define NCT6779D_CIRWKUP	0x0E /* CIR wakeup */
#define NCT6779D_GPIO_PP_OD	0x0F /* GPIO Push-Pull/Open drain select */
#define NCT6779D_PRT80		0x14 /* Port 80 UART */
#define NCT6779D_DSLP		0x16 /* Deep sleep */


/* virtual LDN for GPIO */

#define NCT6779D_GPIOBASE	((0 << 8) | NCT6779D_WDT1_GPIO01_V)

#define NCT6779D_GPIO0		((1 << 8) | NCT6779D_WDT1_GPIO01_V)
#define NCT6779D_GPIO1		((1 << 8) | NCT6779D_GPIO12345678_V)
#define NCT6779D_GPIO2		((2 << 8) | NCT6779D_GPIO12345678_V)
#define NCT6779D_GPIO3		((3 << 8) | NCT6779D_GPIO12345678_V)
#define NCT6779D_GPIO4		((4 << 8) | NCT6779D_GPIO12345678_V)
#define NCT6779D_GPIO5		((5 << 8) | NCT6779D_GPIO12345678_V)
#define NCT6779D_GPIO6		((6 << 8) | NCT6779D_GPIO12345678_V)
#define NCT6779D_GPIO7		((7 << 8) | NCT6779D_GPIO12345678_V)
#define NCT6779D_GPIO8		((0 << 8) | NCT6779D_GPIO12345678_V)

#endif /* SUPERIO_NUVOTON_NCT6779D_H */
