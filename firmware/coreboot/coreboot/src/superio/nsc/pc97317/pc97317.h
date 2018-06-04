/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2000 AG Electronics Ltd.
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

#ifndef SUPERIO_NSC_PC97317_H
#define SUPERIO_NSC_PC97317_H

#define PC97317_KBCK 0x00 /* Keyboard */
#define PC97317_KBCM 0x01 /* Mouse */
#define PC97317_RTC  0x02 /* Real-Time Clock */
#define PC97317_FDC  0x03 /* Floppy */
#define PC97317_PP   0x04 /* Parallel port */
#define PC97317_SP2  0x05 /* Com2 */
#define PC97317_SP1  0x06 /* Com1 */
#define PC97317_GPIO 0x07
#define PC97317_PM   0x08 /* Power Management */

#include <arch/io.h>
#include <stdint.h>

void pc97317_enable_serial(pnp_devfn_t dev, u16 iobase);

#endif /* SUPERIO_NSC_PC97317_H */
