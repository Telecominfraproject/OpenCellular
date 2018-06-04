/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2014 Edward O'Callaghan <eocallaghan@alterapraxis.com>
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

#ifndef SUPERIO_FINTEK_F81216H_H
#define SUPERIO_FINTEK_F81216H_H

/* Logical Device Numbers (LDN). */
#define F81216H_SP1    0x00	/* UART1 (+CIR mode) */
#define F81216H_SP2    0x01	/* UART2 */
#define F81216H_SP3    0x02	/* UART3 */
#define F81216H_SP4    0x03	/* UART4 */
#define F81216H_WDT    0x08	/* WDT   */

/**
 * The PNP config entry key is parameterised
 * by two bits on this Super I/O with 0x77 as
 * the default key.
 * See page 17 of data sheet for details.
 */
typedef enum {
	MODE_6767 = 0x67,
	MODE_7777 = 0x77,
	MODE_8787 = 0x87,
	MODE_A0A0 = 0xA0,
} mode_key;

void f81216h_enable_serial(pnp_devfn_t dev, u16 iobase, mode_key k);

#endif /* SUPERIO_FINTEK_F81216H_H */
