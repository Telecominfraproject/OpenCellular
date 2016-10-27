/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * EC software sync routines for vboot
 */

#include "2sysincludes.h"
#include "2common.h"
#include "sysincludes.h"
#include "utility.h"
#include "vb2_common.h"
#include "vboot_api.h"
#include "vboot_common.h"
#include "vboot_display.h"
#include "vboot_kernel.h"
#include "vboot_nvstorage.h"

static void request_recovery(VbNvContext *vnc, uint32_t recovery_request)
{
	VB2_DEBUG("request_recovery(%d)\n", (int)recovery_request);

	VbNvSet(vnc, VBNV_RECOVERY_REQUEST, recovery_request);
}

/**
 * Wrapper around VbExEcProtect() which sets recovery reason on error.
 */
static VbError_t EcProtect(int devidx, enum VbSelectFirmware_t select,
			   VbNvContext *vnc)
{
	int rv = VbExEcProtect(devidx, select);

	if (rv == VBERROR_EC_REBOOT_TO_RO_REQUIRED) {
		VBDEBUG(("VbExEcProtect() needs reboot\n"));
	} else if (rv != VBERROR_SUCCESS) {
		VBDEBUG(("VbExEcProtect() returned %d\n", rv));
		request_recovery(vnc, VBNV_RECOVERY_EC_PROTECT);
	}
	return rv;
}

static VbError_t EcUpdateImage(int devidx, VbCommonParams *cparams,
			       enum VbSelectFirmware_t select,
			       int *need_update, int in_rw, VbNvContext *vnc)
{
	VbSharedDataHeader *shared =
		(VbSharedDataHeader *)cparams->shared_data_blob;
	int rv;
	int hash_size;
	int ec_hash_size;
	const uint8_t *hash = NULL;
	const uint8_t *expected = NULL;
	const uint8_t *ec_hash = NULL;
	int expected_size;
	int i;
	int rw_request = select != VB_SELECT_FIRMWARE_READONLY;

	*need_update = 0;
	VBDEBUG(("EcUpdateImage() - "
		 "Check for %s update\n", rw_request ? "RW" : "RO"));

	/* Get current EC hash. */
	rv = VbExEcHashImage(devidx, select, &ec_hash, &ec_hash_size);
	if (rv) {
		VBDEBUG(("EcUpdateImage() - "
			 "VbExEcHashImage() returned %d\n", rv));
		request_recovery(vnc, VBNV_RECOVERY_EC_HASH_FAILED);
		return VBERROR_EC_REBOOT_TO_RO_REQUIRED;
	}
	VBDEBUG(("EC-%s hash: ", rw_request ? "RW" : "RO"));
	for (i = 0; i < ec_hash_size; i++)
		VBDEBUG(("%02x",ec_hash[i]));
	VBDEBUG(("\n"));

	/* Get expected EC hash. */
	rv = VbExEcGetExpectedImageHash(devidx, select, &hash, &hash_size);
	if (rv) {
		VBDEBUG(("EcUpdateImage() - "
			 "VbExEcGetExpectedImageHash() returned %d\n", rv));
		request_recovery(vnc, VBNV_RECOVERY_EC_EXPECTED_HASH);
		return VBERROR_EC_REBOOT_TO_RO_REQUIRED;
	}
	if (ec_hash_size != hash_size) {
		VBDEBUG(("EcUpdateImage() - "
			 "EC uses %d-byte hash, but AP-RW contains %d bytes\n",
			 ec_hash_size, hash_size));
		request_recovery(vnc, VBNV_RECOVERY_EC_HASH_SIZE);
		return VBERROR_EC_REBOOT_TO_RO_REQUIRED;
	}

	VBDEBUG(("Expected hash: "));
	for (i = 0; i < hash_size; i++)
		VBDEBUG(("%02x", hash[i]));
	VBDEBUG(("\n"));
	*need_update = vb2_safe_memcmp(ec_hash, hash, hash_size);

	if (!*need_update)
		return VBERROR_SUCCESS;

	/* Get expected EC image */
	rv = VbExEcGetExpectedImage(devidx, select, &expected, &expected_size);
	if (rv) {
		VBDEBUG(("EcUpdateImage() - "
			 "VbExEcGetExpectedImage() returned %d\n", rv));
		request_recovery(vnc, VBNV_RECOVERY_EC_EXPECTED_IMAGE);
		return VBERROR_EC_REBOOT_TO_RO_REQUIRED;
	}
	VBDEBUG(("EcUpdateImage() - image len = %d\n", expected_size));

	if (in_rw && rw_request) {
		/*
		 * Check if BIOS should also load VGA Option ROM when
		 * rebooting to save another reboot if possible.
		 */
		if ((shared->flags & VBSD_EC_SLOW_UPDATE) &&
		    (shared->flags & VBSD_OPROM_MATTERS) &&
		    !(shared->flags & VBSD_OPROM_LOADED)) {
			VBDEBUG(("EcUpdateImage() - Reboot to "
				 "load VGA Option ROM\n"));
			VbNvSet(vnc, VBNV_OPROM_NEEDED, 1);
		}

		/*
		 * EC is running the wrong RW image.  Reboot the EC to
		 * RO so we can update it on the next boot.
		 */
		VBDEBUG(("EcUpdateImage() - "
			 "in RW, need to update RW, so reboot\n"));
		return VBERROR_EC_REBOOT_TO_RO_REQUIRED;
	}

	VBDEBUG(("EcUpdateImage() updating EC-%s...\n",
		 rw_request ? "RW" : "RO"));

	if (shared->flags & VBSD_EC_SLOW_UPDATE) {
		VBDEBUG(("EcUpdateImage() - EC is slow. Show WAIT screen.\n"));

		/* Ensure the VGA Option ROM is loaded */
		if ((shared->flags & VBSD_OPROM_MATTERS) &&
		    !(shared->flags & VBSD_OPROM_LOADED)) {
			VBDEBUG(("EcUpdateImage() - Reboot to "
				 "load VGA Option ROM\n"));
			VbNvSet(vnc, VBNV_OPROM_NEEDED, 1);
			return VBERROR_VGA_OPROM_MISMATCH;
		}

		VbDisplayScreen(cparams, VB_SCREEN_WAIT, 0, vnc);
	}

	rv = VbExEcUpdateImage(devidx, select, expected, expected_size);
	if (rv != VBERROR_SUCCESS) {
		VBDEBUG(("EcUpdateImage() - "
			 "VbExEcUpdateImage() returned %d\n", rv));

		/*
		 * The EC may know it needs a reboot.  It may need to
		 * unprotect the region before updating, or may need to
		 * reboot after updating.  Either way, it's not an error
		 * requiring recovery mode.
		 *
		 * If we fail for any other reason, trigger recovery
		 * mode.
		 */
		if (rv != VBERROR_EC_REBOOT_TO_RO_REQUIRED)
			request_recovery(vnc, VBNV_RECOVERY_EC_UPDATE);

		return VBERROR_EC_REBOOT_TO_RO_REQUIRED;
	}

	/* Verify the EC was updated properly */
	rv = VbExEcHashImage(devidx, select, &ec_hash, &ec_hash_size);
	if (rv) {
		VBDEBUG(("EcUpdateImage() - "
			 "VbExEcHashImage() returned %d\n", rv));
		request_recovery(vnc, VBNV_RECOVERY_EC_HASH_FAILED);
		return VBERROR_EC_REBOOT_TO_RO_REQUIRED;
	}
	if (hash_size != ec_hash_size) {
		VBDEBUG(("EcUpdateImage() - "
			 "VbExEcHashImage() says size %d, not %d\n",
			 ec_hash_size, hash_size));
		request_recovery(vnc, VBNV_RECOVERY_EC_HASH_SIZE);
		return VBERROR_EC_REBOOT_TO_RO_REQUIRED;
	}
	VBDEBUG(("Updated EC-%s hash: ", rw_request ? "RW" : "RO"));
	for (i = 0; i < ec_hash_size; i++)
		VBDEBUG(("%02x",ec_hash[i]));
	VBDEBUG(("\n"));

	if (vb2_safe_memcmp(ec_hash, hash, hash_size)){
		VBDEBUG(("EcUpdateImage() - "
			 "Failed to update EC-%s\n", rw_request ?
			 "RW" : "RO"));
		request_recovery(vnc, VBNV_RECOVERY_EC_UPDATE);
		return VBERROR_EC_REBOOT_TO_RO_REQUIRED;
	}

	return VBERROR_SUCCESS;
}

VbError_t VbEcSoftwareSync(int devidx, VbCommonParams *cparams,
			   VbNvContext *vnc)
{
	VbSharedDataHeader *shared =
		(VbSharedDataHeader *)cparams->shared_data_blob;
	enum VbSelectFirmware_t select_rw =
		shared->firmware_index ? VB_SELECT_FIRMWARE_B :
		VB_SELECT_FIRMWARE_A;
	enum VbSelectFirmware_t select_ro = VB_SELECT_FIRMWARE_READONLY;
	int in_rw = 0;
	int ro_try_count = 2;
	int num_tries = 0;
	uint32_t try_ro_sync, recovery_request;
	int rv, updated_rw, updated_ro;

	VBDEBUG(("VbEcSoftwareSync(devidx=%d)\n", devidx));

	/* Determine whether the EC is in RO or RW */
	rv = VbExEcRunningRW(devidx, &in_rw);

	if (shared->recovery_reason) {
		/* Recovery mode; just verify the EC is in RO code */
		if (rv == VBERROR_SUCCESS && in_rw == 1) {
			/*
			 * EC is definitely in RW firmware.  We want it in
			 * read-only code, so preserve the current recovery
			 * reason and reboot.
			 *
			 * We don't reboot on error or unknown EC code, because
			 * we could end up in an endless reboot loop.  If we
			 * had some way to track that we'd already rebooted for
			 * this reason, we could retry only once.
			 */
			VBDEBUG(("VbEcSoftwareSync() - "
				 "want recovery but got EC-RW\n"));
			request_recovery(vnc, shared->recovery_reason);
			return VBERROR_EC_REBOOT_TO_RO_REQUIRED;
		}

		VBDEBUG(("VbEcSoftwareSync() in recovery; EC-RO\n"));
		return VBERROR_SUCCESS;
	}

	/*
	 * Not in recovery.  If we couldn't determine where the EC was,
	 * reboot to recovery.
	 */
	if (rv != VBERROR_SUCCESS) {
		VBDEBUG(("VbEcSoftwareSync() - "
			 "VbExEcRunningRW() returned %d\n", rv));
		request_recovery(vnc, VBNV_RECOVERY_EC_UNKNOWN_IMAGE);
		return VBERROR_EC_REBOOT_TO_RO_REQUIRED;
	}

	/* If AP is read-only normal, EC should be in its RO code also. */
	if (shared->flags & VBSD_LF_USE_RO_NORMAL) {
		/* If EC is in RW code, request reboot back to RO */
		if (in_rw == 1) {
			VBDEBUG(("VbEcSoftwareSync() - "
				 "want RO-normal but got EC-RW\n"));
			return VBERROR_EC_REBOOT_TO_RO_REQUIRED;
		}

		/* Protect the RW flash and stay in EC-RO */
		rv = EcProtect(devidx, select_rw, vnc);
		if (rv != VBERROR_SUCCESS)
			return rv;

		rv = VbExEcDisableJump(devidx);
		if (rv != VBERROR_SUCCESS) {
			VBDEBUG(("VbEcSoftwareSync() - "
				 "VbExEcDisableJump() returned %d\n", rv));
			request_recovery(vnc, VBNV_RECOVERY_EC_SOFTWARE_SYNC);
			return VBERROR_EC_REBOOT_TO_RO_REQUIRED;
		}

		VBDEBUG(("VbEcSoftwareSync() in RO-Normal; EC-RO\n"));
		return VBERROR_SUCCESS;
	}

	VBDEBUG(("VbEcSoftwareSync() check for RW update.\n"));

	/* Update the RW Image. */
	rv = EcUpdateImage(devidx, cparams, select_rw, &updated_rw, in_rw, vnc);

	if (rv != VBERROR_SUCCESS) {
		VBDEBUG(("VbEcSoftwareSync() - "
			 "EcUpdateImage() returned %d\n", rv));
		return rv;
	}

	/* Tell EC to jump to its RW image */
	if (!in_rw) {
		VBDEBUG(("VbEcSoftwareSync() jumping to EC-RW\n"));
		rv = VbExEcJumpToRW(devidx);
		if (rv != VBERROR_SUCCESS) {
			VBDEBUG(("VbEcSoftwareSync() - "
				 "VbExEcJumpToRW() returned %x\n", rv));

			/*
			 * If the EC booted RO-normal and a previous AP boot
			 * has called VbExEcStayInRO(), we need to reboot the EC
			 * to unlock the ability to jump to the RW firmware.
			 *
			 * All other errors trigger recovery mode.
			 */
			if (rv != VBERROR_EC_REBOOT_TO_RO_REQUIRED)
				request_recovery(vnc, VBNV_RECOVERY_EC_JUMP_RW);

			return VBERROR_EC_REBOOT_TO_RO_REQUIRED;
		}
	}

	VbNvGet(vnc, VBNV_TRY_RO_SYNC, &try_ro_sync);

	if (!devidx && try_ro_sync &&
	    !(shared->flags & VBSD_BOOT_FIRMWARE_WP_ENABLED)) {
		/* Reset RO Software Sync NV flag */
		VbNvSet(vnc, VBNV_TRY_RO_SYNC, 0);

		VbNvGet(vnc, VBNV_RECOVERY_REQUEST, &recovery_request);

		/* Update the RO Image. */
		while (num_tries < ro_try_count) {
			VBDEBUG(("VbEcSoftwareSync() RO Software Sync\n"));

			/* Get expected EC-RO Image. */
			rv = EcUpdateImage(devidx, cparams, select_ro,
					   &updated_ro, in_rw, vnc);
			if (rv == VBERROR_SUCCESS) {
				/*
				 * If the RO update had failed, reset the
				 * recovery request.
				 */
				if (num_tries)
					request_recovery(vnc, recovery_request);
				break;
			} else
				VBDEBUG(("VbEcSoftwareSync() - "
					 "EcUpdateImage() returned %d\n", rv));

			num_tries++;
		}
	}
	if (rv != VBERROR_SUCCESS)
		return rv;

	/* Protect RO flash */
	rv = EcProtect(devidx, select_ro, vnc);
	if (rv != VBERROR_SUCCESS)
		return rv;

	/* Protect RW flash */
	rv = EcProtect(devidx, select_rw, vnc);
	if (rv != VBERROR_SUCCESS)
		return rv;

	rv = VbExEcDisableJump(devidx);
	if (rv != VBERROR_SUCCESS) {
		VBDEBUG(("VbEcSoftwareSync() - "
			"VbExEcDisableJump() returned %d\n", rv));
		request_recovery(vnc, VBNV_RECOVERY_EC_SOFTWARE_SYNC);
		return VBERROR_EC_REBOOT_TO_RO_REQUIRED;
	}

	/*
	 * Reboot to unload VGA Option ROM if:
	 * - RW update was done
	 * - the system is NOT in developer mode
	 * - the system has slow EC update flag set
	 * - the VGA Option ROM was needed and loaded
	 */
	if (updated_rw &&
	    !(shared->flags & VBSD_BOOT_DEV_SWITCH_ON) &&
	    (shared->flags & VBSD_EC_SLOW_UPDATE) &&
	    (shared->flags & VBSD_OPROM_MATTERS) &&
	    (shared->flags & VBSD_OPROM_LOADED)) {
		VBDEBUG(("VbEcSoftwareSync() - Reboot to "
			 "unload VGA Option ROM\n"));
		VbNvSet(vnc, VBNV_OPROM_NEEDED, 0);
		return VBERROR_VGA_OPROM_MISMATCH;
	}


	return rv;
}
