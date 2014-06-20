/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Non-volatile storage routines.
 */
#include "sysincludes.h"

#include "crc8.h"
#include "utility.h"
#include "vboot_common.h"
#include "vboot_nvstorage.h"
#include "rollback_index.h"

/* These are the fields of the nvram that we want to back up. */
static const VbNvParam backup_params[] = {
	VBNV_KERNEL_FIELD,
	VBNV_LOCALIZATION_INDEX,
	VBNV_DEV_BOOT_USB,
	VBNV_DEV_BOOT_LEGACY,
	VBNV_DEV_BOOT_SIGNED_ONLY,
};

/* We can't back things up if there isn't enough storage. */
BUILD_ASSERT(VBNV_BLOCK_SIZE <= BACKUP_NV_SIZE);

int RestoreNvFromBackup(VbNvContext *vnc)
{
	VbNvContext bvnc;
	uint32_t value;
	int i;

	VBDEBUG(("TPM: %s()\n", __func__));

	if (TPM_SUCCESS != RollbackBackupRead(bvnc.raw))
		return 1;

	VbNvSetup(&bvnc);
	if (bvnc.regenerate_crc) {
		VBDEBUG(("TPM: Oops, backup is no good.\n"));
		return 1;
	}

	for (i = 0; i < ARRAY_SIZE(backup_params); i++) {
		VbNvGet(&bvnc, backup_params[i], &value);
		VbNvSet(vnc, backup_params[i], value);
	}

	/* VbNvTeardown(&bvnc); is not needed. We're done with it. */
	return 0;
}

int SaveNvToBackup(VbNvContext *vnc)
{
	VbNvContext bvnc;
	uint32_t value;
	int i;

	VBDEBUG(("TPM: %s()\n", __func__));

	/* Read it first. No point in writing the same data. */
	if (TPM_SUCCESS != RollbackBackupRead(bvnc.raw))
		return 1;

	VbNvSetup(&bvnc);
	VBDEBUG(("TPM: existing backup is %s\n",
		 bvnc.regenerate_crc ? "bad" : "good"));

	for (i = 0; i < ARRAY_SIZE(backup_params); i++) {
		VbNvGet(vnc, backup_params[i], &value);
		VbNvSet(&bvnc, backup_params[i], value);
	}

	VbNvTeardown(&bvnc);

	if (!bvnc.raw_changed) {
		VBDEBUG(("TPM: Nothing's changed, not writing backup\n"));
		/* Clear the request flag, since we're happy. */
		VbNvSet(vnc, VBNV_BACKUP_NVRAM_REQUEST, 0);
		return 0;
	}

	if (TPM_SUCCESS == RollbackBackupWrite(bvnc.raw)) {
		/* Clear the request flag if we wrote successfully too */
		VbNvSet(vnc, VBNV_BACKUP_NVRAM_REQUEST, 0);
		return 0;
	}

	VBDEBUG(("TPM: Sorry, couldn't write backup.\n"));
	return 1;
}
