/*
 * This file is part of the coreboot project.
 *
 * Copyright 2013 Google Inc.
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

#include <bootblock_common.h>
#include <console/console.h>
#include <program_loading.h>

/* The qemu part of all this is very, very non-hardware like.
 * So it gets its own bootblock.
 */
void main(void)
{
	if (IS_ENABLED(CONFIG_BOOTBLOCK_CONSOLE)) {
		console_init();
	}

	run_romstage();
}
