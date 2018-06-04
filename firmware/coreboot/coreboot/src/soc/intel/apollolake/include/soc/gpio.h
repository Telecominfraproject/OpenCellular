/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2017 Intel Corp.
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

#ifndef _SOC_APL_GPIO_H_
#define _SOC_APL_GPIO_H_

#if IS_ENABLED(CONFIG_SOC_INTEL_GLK)
#include <soc/gpio_glk.h>
#else
#include <soc/gpio_apl.h>
#endif
#include <intelblocks/gpio.h>/* intelblocks/gpio.h depends on definitions in
				soc/gpio_glk.h and soc/gpio_apl.h */
#endif
