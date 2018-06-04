/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2014 Edward O'Callaghan <eocallaghan@alterapraxis.com>
 * Copyright (C) 2014 Felix Held <felix-coreboot@felixheld.de>
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

#ifndef SUPERIO_NUVOTON_COMMON_ROMSTAGE_H
#define SUPERIO_NUVOTON_COMMON_ROMSTAGE_H

#include <arch/io.h>
#include <stdint.h>

void nuvoton_pnp_enter_conf_state(pnp_devfn_t dev);
void nuvoton_pnp_exit_conf_state(pnp_devfn_t dev);
void nuvoton_enable_serial(pnp_devfn_t dev, u16 iobase);

#endif /* SUPERIO_NUVOTON_COMMON_ROMSTAGE_H */
