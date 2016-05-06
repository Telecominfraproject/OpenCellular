/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "bdb_api.h"
#include "bdb.h"

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

__attribute__((weak))
int vbe_read_nvm(enum nvm_type type, uint8_t *buf, uint32_t size)
{
	return BDB_ERROR_NOT_IMPLEMENTED;
}

__attribute__((weak))
int vbe_write_nvm(enum nvm_type type, void *buf, uint32_t size)
{
	return BDB_ERROR_NOT_IMPLEMENTED;
}
