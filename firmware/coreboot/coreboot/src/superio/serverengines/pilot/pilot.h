/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2009 University of Heidelberg
 * Written by Mondrian Nuessle <nuessle@uni-heidelberg.de> for Univ. Heidelberg
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

#ifndef SUPERIO_SERVERENGINES_PILOT_PILOT_H
#define SUPERIO_SERVERENGINES_PILOT_PILOT_H

/* PILOT Super I/O is only based on LPC observation done on factory system. */

#define PILOT_LD1 0x01 /* Logical device 1 */
#define PILOT_SP1 0x02 /* Com1 */
#define PILOT_LD4 0x04 /* Logical device 4 */
#define PILOT_LD5 0x05 /* Logical device 5 */
#define PILOT_LD7 0x07 /* Logical device 7 */

/* should not expose these however early_init needs love */
void pnp_enter_ext_func_mode(pnp_devfn_t dev);
void pnp_exit_ext_func_mode(pnp_devfn_t dev);

void pilot_early_init(pnp_devfn_t dev);

void pilot_enable_serial(pnp_devfn_t dev, u16 iobase);
void pilot_disable_serial(pnp_devfn_t dev);

#endif /* SUPERIO_SERVERENGINES_PILOT_PILOT_H */
