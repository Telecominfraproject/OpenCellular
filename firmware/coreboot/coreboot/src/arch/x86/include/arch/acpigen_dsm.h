/*
 * This file is part of the coreboot project.
 *
 * Copyright 2016 Google Inc.
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

#ifndef __ARCH_ACPIGEN_DSM_H__
#define __ARCH_ACPIGEN_DSM_H__

#include <stdint.h>

struct dsm_i2c_hid_config {
	uint8_t hid_desc_reg_offset;
};

void acpigen_write_dsm_i2c_hid(struct dsm_i2c_hid_config *config);

#endif /* __ARCH_ACPIGEN_DSM_H__ */
