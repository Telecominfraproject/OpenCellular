/*
 * Copyright (c) 2015-2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <board_arm_def.h>
#include <console.h>
#include <debug.h>
#include <errno.h>
#include <norflash.h>
#include <platform.h>
#include <stdint.h>

/*
 * ARM common implementation for error handler
 */
void plat_error_handler(int err)
{
	int ret;

	switch (err) {
	case -ENOENT:
	case -EAUTH:
		/* Image load or authentication error. Erase the ToC */
		INFO("Erasing FIP ToC from flash...\n");
		nor_unlock(PLAT_ARM_FIP_BASE);
		ret = nor_word_program(PLAT_ARM_FIP_BASE, 0);
		if (ret != 0) {
			ERROR("Cannot erase ToC\n");
		} else {
			INFO("Done\n");
		}
		break;
	default:
		/* Unexpected error */
		break;
	}

	(void)console_flush();

	/* Loop until the watchdog resets the system */
	for (;;)
		wfi();
}
