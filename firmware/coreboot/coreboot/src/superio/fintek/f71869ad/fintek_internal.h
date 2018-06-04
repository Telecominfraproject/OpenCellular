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

#ifndef SUPERIO_FINTEK_F71869AD_INTERNAL_H
#define SUPERIO_FINTEK_F71869AD_INTERNAL_H

#include <arch/io.h>
#include <device/pnp.h>

void f71869ad_multifunc_init(struct device *dev);
void f71869ad_hwm_init(struct device *dev);

#endif /* SUPERIO_FINTEK_F71869AD_INTERNAL_H */
