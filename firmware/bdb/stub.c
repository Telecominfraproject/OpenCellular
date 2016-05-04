/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "bdb_api.h"

__attribute__((weak))
uint32_t vbe_get_vboot_register(enum vboot_register type)
{
	return 0;
}

__attribute__((weak))
void vbe_set_vboot_register(enum vboot_register type, uint32_t val)
{
	return;
}

__attribute__((weak))
void vbe_reset(void)
{
	return;
}
