/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2007 Advanced Micro Devices, Inc.
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

#ifndef NORTHBRIDGE_AMD_LX_H
#define NORTHBRIDGE_AMD_LX_H

#include <cpu/amd/lxdef.h>

/* northbridge.c */
int sizeram(void);

/* northbridgeinit.c */
void northbridge_init_early(void);

/* pll_reset.c */
unsigned int GeodeLinkSpeed(void);
#endif
