/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2005 Digital Design Corporation
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

#ifndef SUPERIO_SMSC_LPC47N217_LPC47N217_H
#define SUPERIO_SMSC_LPC47N217_LPC47N217_H

#include <arch/io.h>
#include <stdint.h>

/*
 * These are arbitrary, but must match declarations in the mainboard
 * devicetree.cb file. Values chosen to match SMSC LPC47B37x.
 */
#define LPC47N217_PP               3   /* Parallel Port */
#define LPC47N217_SP1              4   /* Com1 */
#define LPC47N217_SP2              5   /* Com2 */

#define LPC47N217_MAX_CONFIG_REGISTER	0x39

void lpc47n217_enable_serial(pnp_devfn_t dev, u16 iobase);

#endif
