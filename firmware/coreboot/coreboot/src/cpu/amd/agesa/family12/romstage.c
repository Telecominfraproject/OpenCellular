/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2011 Advanced Micro Devices, Inc.
 * Copyright (C) 2017 Kyösti Mälkki
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

#include <northbridge/amd/agesa/state_machine.h>

#include "sb_cimx.h"
#include "SbPlatform.h"
#include "platform_cfg.h"

void platform_once(struct sysinfo *cb)
{
	gpioEarlyInit();

	sb_poweron_init();

	board_BeforeAgesa(cb);
}
