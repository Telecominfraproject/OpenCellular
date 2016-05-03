/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef VBOOT_REFERENCE_FIRMWARE_BDB_VBOOT_REGISTER_H
#define VBOOT_REFERENCE_FIRMWARE_BDB_VBOOT_REGISTER_H

enum vboot_register {
	/* Register cleared after every reset */
	VBOOT_REGISTER,
	/* Register cleared after cold reset (persists after warm reset) */
	VBOOT_REGISTER_PERSIST,
};

/* Bit fields for VBOOT_REGISTER_PERSISTENT */
#define VBOOT_REGISTER_RECOVERY_REQUEST		(1 << 0)
#define VBOOT_REGISTER_TRY_SECONDARY_BDB	(1 << 1)
#define VBOOT_REGISTER_FAILED_RW_PRIMARY	(1 << 2)
#define VBOOT_REGISTER_FAILED_RW_SECONDARY	(1 << 3)

#endif
