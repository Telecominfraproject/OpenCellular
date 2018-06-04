/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2014 Felix Held <felix-coreboot@felixheld.de>
 * Copyright (C) 2015 - 2016 Raptor Engineering, LLC
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

#ifndef SUPERIO_WINBOND_W83667HG_A
#define SUPERIO_WINBOND_W83667HG_A

/* Pinmux configuration defines */
#define W83667HG_SPI_PINMUX_OFFSET		0x2a

#define W83667HG_SPI_PINMUX_GPIO4_SERIAL_B_MASK	(1 << 2)
#define W83667HG_SPI_PINMUX_GPIO4		(0 << 2)
#define W83667HG_SPI_PINMUX_SERIAL_B		(1 << 2)

/* Logical Device Numbers (LDN). */
#define W83667HG_A_FDC		0x00
#define W83667HG_A_PP		0x01
#define W83667HG_A_SP1		0x02 /* Com1 */
#define W83667HG_A_SP2		0x03 /* Com2 */
#define W83667HG_A_KBC		0x05
#define W83667HG_A_SPI		0x06
#define W83667HG_A_GPIO6789_V	0x07
#define W83667HG_A_WDT1		0x08
#define W83667HG_A_GPIO2345_V	0x09
#define W83667HG_A_ACPI		0x0A
#define W83667HG_A_HWM_TSI	0x0B /* HW monitor/SB-TSI/deep S5 */
#define W83667HG_A_PECI		0x0C
#define W83667HG_A_VID_BUSSEL	0x0D /* VID and BUSSEL */
#define W83667HG_A_GPIO_PP_OD	0x0F /* GPIO Push-Pull/Open drain select */

/* Virtual LDN for GPIO and SPI */
#define W83667HG_A_SPI1			((1 << 8) | W83667HG_A_SPI)
#define W83667HG_A_GPIO1		((1 << 8) | W83667HG_A_WDT1)
#define W83667HG_A_GPIO2		((0 << 8) | W83667HG_A_GPIO2345_V)
#define W83667HG_A_GPIO3		((1 << 8) | W83667HG_A_GPIO2345_V)
#define W83667HG_A_GPIO4		((2 << 8) | W83667HG_A_GPIO2345_V)
#define W83667HG_A_GPIO5		((3 << 8) | W83667HG_A_GPIO2345_V)
#define W83667HG_A_GPIO6		((1 << 8) | W83667HG_A_GPIO6789_V)
#define W83667HG_A_GPIO7		((2 << 8) | W83667HG_A_GPIO6789_V)
#define W83667HG_A_GPIO8		((3 << 8) | W83667HG_A_GPIO6789_V)
#define W83667HG_A_GPIO9		((4 << 8) | W83667HG_A_GPIO6789_V)

#endif /* SUPERIO_WINBOND_W83667HG_A */
