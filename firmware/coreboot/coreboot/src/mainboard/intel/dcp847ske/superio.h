/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2017 Tobias Diedrich <ranma+coreboot@tdiedrich.de>
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

#ifndef DCP847SKE_SUPERIO_H
#define DCP847SKE_SUPERIO_H

#include <arch/io.h>

#define NUVOTON_PORT 0x4e
#define HWM_PORT     0x0a30
#define GPIO_PORT    0x0a80

#define SUPERIO_BANK(x) (0x0700 | x)
#define SUPERIO_INITVAL(reg, data) ((reg << 8) | (data))
#define HWM_BANK(x)     (0x4e00 | x)
#define HWM_INITVAL SUPERIO_INITVAL

#define SUPERIO_UNLOCK do { \
	outb(0x87, NUVOTON_PORT); \
	outb(0x87, NUVOTON_PORT); \
} while (0)

#define SUPERIO_LOCK do { \
	outb(0xaa, NUVOTON_PORT); \
} while (0)

#define SUPERIO_WRITE(reg, data) do { \
	outb((reg), NUVOTON_PORT); \
	outb((data), NUVOTON_PORT + 1); \
} while (0)

#define SUPERIO_WRITE_INITVAL(val) SUPERIO_WRITE((val) >> 8, (val) & 0xff)

#define HWM_WRITE(reg, data) do { \
	outb((reg), HWM_PORT + 5); \
	outb((data), HWM_PORT + 6); \
} while (0)

#define HWM_WRITE_INITVAL(val) HWM_WRITE((val) >> 8, (val) & 0xff)

#endif /* DCP847SKE_SUPERIO_H */
