/*
 * This file is part of the bayou project.
 *
 * Copyright (C) 2008 Advanced Micro Devices, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef SELF_H_
#define SELF_H_

#include <libpayload.h>

struct self_segment {
	u32 type;
	u32 offset;
	u64 load_addr;
	u32 len;
	u32 mem_len;
};

struct self {
	struct larstat stat;
	void *fptr;
};

#define SELF_TYPE_CODE   0x45444F43
#define SELF_TYPE_DATA   0x41544144
#define SELF_TYPE_BSS    0x20535342
#define SELF_TYPE_PARAMS 0x41524150
#define SELF_TYPE_ENTRY  0x52544E45

#endif
