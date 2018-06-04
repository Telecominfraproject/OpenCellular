/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2015-2016 secunet Security Networks AG
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

#ifndef _EC_RODA_IT8518_CHIP_H
#define _EC_RODA_IT8518_CHIP_H

#include <device/device.h>

struct chip_operations;
extern struct chip_operations ec_roda_it8518_ops;

struct ec_roda_it8518_config {
	u8 cpuhot_limit;	/* temperature in °C which asserts PROCHOT# */
};

#endif /* _EC_RODA_IT8518_CHIP_H */
