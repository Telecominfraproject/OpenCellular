/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * High-level firmware API for loading and verifying rewritable firmware.
 * (Firmware portion)
 */

#include "sysincludes.h"

#include "region.h"
#include "gbb_access.h"
#include "gbb_header.h"
#include "load_firmware_fw.h"
#include "utility.h"
#include "vboot_api.h"
#include "vboot_common.h"
#include "vboot_nvstorage.h"

/*
 * Static variables for UpdateFirmwareBodyHash().  It's less than optimal to
 * have static variables in a library, but in UEFI the caller is deep inside a
 * different firmware stack and doesn't have a good way to pass the params
 * struct back to us.
 */
typedef struct VbLoadFirmwareInternal {
	DigestContext body_digest_context;
	uint32_t body_size_accum;
} VbLoadFirmwareInternal;

void VbUpdateFirmwareBodyHash(VbCommonParams *cparams, uint8_t *data,
			      uint32_t size)
{
	VbLoadFirmwareInternal *lfi =
		(VbLoadFirmwareInternal*)cparams->vboot_context;

	DigestUpdate(&lfi->body_digest_context, data, size);
	lfi->body_size_accum += size;
}

int LoadFirmware(VbCommonParams *cparams, VbSelectFirmwareParams *fparams,
                 VbNvContext *vnc)
{
	VbSharedDataHeader *shared =
		(VbSharedDataHeader *)cparams->shared_data_blob;
	GoogleBinaryBlockHeader *gbb = cparams->gbb;
	VbPublicKey *root_key = NULL;
	VbLoadFirmwareInternal *lfi;

	uint32_t try_b_count;
	uint32_t lowest_version = 0xFFFFFFFF;
	int good_index = -1;
	int is_dev;
	int index;
	int i;

	int retval = VBERROR_UNKNOWN;
	int recovery = VBNV_RECOVERY_RO_UNSPECIFIED;

	/* Clear output params in case we fail */
	shared->firmware_index = 0xFF;

	VBDEBUG(("LoadFirmware started...\n"));

	/* Must have a root key from the GBB */
	retval = VbGbbReadRootKey(cparams, &root_key);
	if (retval) {
		VBDEBUG(("No GBB\n"));
		retval = VBERROR_INVALID_GBB;
		goto LoadFirmwareExit;
	}

	/* Parse flags */
	is_dev = (shared->flags & VBSD_BOOT_DEV_SWITCH_ON ? 1 : 0);
	if (is_dev)
		shared->flags |= VBSD_LF_DEV_SWITCH_ON;

	/* Read try-b count and decrement if necessary */
	VbNvGet(vnc, VBNV_TRY_B_COUNT, &try_b_count);
	if (0 != try_b_count) {
		VbNvSet(vnc, VBNV_TRY_B_COUNT, try_b_count - 1);
		shared->flags |= VBSD_FWB_TRIED;
	}

	/* Allocate our internal data */
	lfi = (VbLoadFirmwareInternal *)
		VbExMalloc(sizeof(VbLoadFirmwareInternal));
	cparams->vboot_context = lfi;

	/* Loop over indices */
	for (i = 0; i < 2; i++) {
		VbKeyBlockHeader *key_block;
		uint32_t vblock_size;
		VbFirmwarePreambleHeader *preamble;
		RSAPublicKey *data_key;
		uint64_t key_version;
		uint32_t combined_version;
		uint8_t *body_digest;
		uint8_t *check_result;

		/* If try B count is non-zero try firmware B first */
		index = (try_b_count ? 1 - i : i);
		if (0 == index) {
			key_block = (VbKeyBlockHeader *)
				fparams->verification_block_A;
			vblock_size = fparams->verification_size_A;
			check_result = &shared->check_fw_a_result;
		} else {
			key_block = (VbKeyBlockHeader *)
				fparams->verification_block_B;
			vblock_size = fparams->verification_size_B;
			check_result = &shared->check_fw_b_result;
		}

		/*
		 * Check the key block flags against the current boot mode.  Do
		 * this before verifying the key block, since flags are faster
		 * to check than the RSA signature.
		 */
		if (!(key_block->key_block_flags &
		      (is_dev ? KEY_BLOCK_FLAG_DEVELOPER_1 :
		       KEY_BLOCK_FLAG_DEVELOPER_0))) {
			VBDEBUG(("Developer flag mismatch.\n"));
			*check_result = VBSD_LF_CHECK_DEV_MISMATCH;
			continue;
		}

		/* RW firmware never runs in recovery mode. */
		if (!(key_block->key_block_flags & KEY_BLOCK_FLAG_RECOVERY_0)) {
			VBDEBUG(("Recovery flag mismatch.\n"));
			*check_result = VBSD_LF_CHECK_REC_MISMATCH;
			continue;
		}

		/* Verify the key block */
		if ((0 != KeyBlockVerify(key_block, vblock_size,
					 root_key, 0))) {
			VBDEBUG(("Key block verification failed.\n"));
			*check_result = VBSD_LF_CHECK_VERIFY_KEYBLOCK;
			continue;
		}

		/* Check for rollback of key version. */
		key_version = key_block->data_key.key_version;
		if (!(gbb->flags & GBB_FLAG_DISABLE_FW_ROLLBACK_CHECK)) {
			if (key_version < (shared->fw_version_tpm >> 16)) {
				VBDEBUG(("Key rollback detected.\n"));
				*check_result = VBSD_LF_CHECK_KEY_ROLLBACK;
				continue;
			}
			if (key_version > 0xFFFF) {
				/*
				 * Key version is stored in 16 bits in the TPM,
				 * so key versions greater than 0xFFFF can't be
				 * stored properly.
				 */
				VBDEBUG(("Key version > 0xFFFF.\n"));
				*check_result = VBSD_LF_CHECK_KEY_ROLLBACK;
				continue;
			}
		}

		/* Get key for preamble/data verification from the key block. */
		data_key = PublicKeyToRSA(&key_block->data_key);
		if (!data_key) {
			VBDEBUG(("Unable to parse data key.\n"));
			*check_result = VBSD_LF_CHECK_DATA_KEY_PARSE;
			continue;
		}

		/* Verify the preamble, which follows the key block. */
		preamble = (VbFirmwarePreambleHeader *)
			((uint8_t *)key_block + key_block->key_block_size);
		if ((0 != VerifyFirmwarePreamble(
					preamble,
					vblock_size - key_block->key_block_size,
					data_key))) {
			VBDEBUG(("Preamble verfication failed.\n"));
			*check_result = VBSD_LF_CHECK_VERIFY_PREAMBLE;
			RSAPublicKeyFree(data_key);
			continue;
		}

		/* Check for rollback of firmware version. */
		combined_version = (uint32_t)((key_version << 16) |
				(preamble->firmware_version & 0xFFFF));
		if (combined_version < shared->fw_version_tpm &&
		    !(gbb->flags & GBB_FLAG_DISABLE_FW_ROLLBACK_CHECK)) {
			VBDEBUG(("Firmware version rollback detected.\n"));
			*check_result = VBSD_LF_CHECK_FW_ROLLBACK;
			RSAPublicKeyFree(data_key);
			continue;
		}

		/* Header for this firmware is valid */
		*check_result = VBSD_LF_CHECK_HEADER_VALID;

		/* Check for lowest key version from a valid header. */
		if (lowest_version > combined_version)
			lowest_version = combined_version;

		/*
		 * If we already have good firmware, no need to read another
		 * one; we only needed to look at the versions to check for
		 * rollback.
		 */
		if (-1 != good_index) {
			RSAPublicKeyFree(data_key);
			continue;
		}

		/* Handle preamble flag for using the RO normal/dev code path */
		VBDEBUG(("Preamble flags %#x\n", VbGetFirmwarePreambleFlags(preamble)));
		if (VbGetFirmwarePreambleFlags(preamble) &
		    VB_FIRMWARE_PREAMBLE_USE_RO_NORMAL) {

			/* Fail if calling firmware doesn't support RO normal */
			if (!(shared->flags & VBSD_BOOT_RO_NORMAL_SUPPORT)) {
				VBDEBUG(("No RO normal support.\n"));
				*check_result = VBSD_LF_CHECK_NO_RO_NORMAL;
				RSAPublicKeyFree(data_key);
				continue;
			}

			/* Use the RO normal code path */
			shared->flags |= VBSD_LF_USE_RO_NORMAL;

		} else {
			VbError_t rv;

			/* Read the firmware data */
			DigestInit(&lfi->body_digest_context,
				   data_key->algorithm);
			lfi->body_size_accum = 0;
			rv = VbExHashFirmwareBody(
					cparams,
					(index ? VB_SELECT_FIRMWARE_B :
					 VB_SELECT_FIRMWARE_A));
			if (VBERROR_SUCCESS != rv) {
				VBDEBUG(("VbExHashFirmwareBody() failed for "
					 "index %d\n", index));
				*check_result = VBSD_LF_CHECK_GET_FW_BODY;
				RSAPublicKeyFree(data_key);
				continue;
			}
			if (lfi->body_size_accum !=
			    preamble->body_signature.data_size) {
				VBDEBUG(("Hashed %d bytes but expected %d\n",
					 (int)lfi->body_size_accum,
					 (int)preamble->body_signature.data_size));
				*check_result = VBSD_LF_CHECK_HASH_WRONG_SIZE;
				RSAPublicKeyFree(data_key);
				continue;
			}

			/* Verify firmware data */
			body_digest = DigestFinal(&lfi->body_digest_context);
			if (0 != VerifyDigest(body_digest,
					      &preamble->body_signature,
					      data_key)) {
				VBDEBUG(("FW body verification failed.\n"));
				*check_result = VBSD_LF_CHECK_VERIFY_BODY;
				RSAPublicKeyFree(data_key);
				VbExFree(body_digest);
				continue;
			}
			VbExFree(body_digest);
		}

		/* Done with the data key, so can free it now */
		RSAPublicKeyFree(data_key);

		/* If we're still here, the firmware is valid. */
		VBDEBUG(("Firmware %d is valid.\n", index));
		*check_result = VBSD_LF_CHECK_VALID;
		if (-1 == good_index) {
			/* Save the key we actually used */
			if (0 != VbSharedDataSetKernelKey(
					shared, &preamble->kernel_subkey)) {
				/*
				 * The firmware signature was good, but the
				 * public key was bigger that the caller can
				 * handle.
				 */
				VBDEBUG(("Unable to save kernel subkey.\n"));
				continue;
			}

			/*
			 * Save the good index, now that we're sure we can
			 * actually use this firmware.  That's the one we'll
			 * boot.
			 */
			good_index = index;
			shared->firmware_index = (uint8_t)index;
			shared->fw_keyblock_flags = key_block->key_block_flags;

			/*
			 * If the good firmware's key version is the same as
			 * the tpm, then the TPM doesn't need updating; we can
			 * stop now.  Otherwise, we'll check all the other
			 * headers to see if they contain a newer key.
			 */
			if (combined_version == shared->fw_version_tpm)
				break;
		}
	}

	/* Free internal data */
	VbExFree(lfi);
	cparams->vboot_context = NULL;

	/* Handle finding good firmware */
	if (good_index >= 0) {

		/* Save versions we found */
		shared->fw_version_lowest = lowest_version;
		if (lowest_version > shared->fw_version_tpm)
			shared->fw_version_tpm = lowest_version;

		/* Success */
		VBDEBUG(("Will boot firmware index %d\n",
			 (int)shared->firmware_index));
		retval = VBERROR_SUCCESS;

	} else {
		uint8_t a = shared->check_fw_a_result;
		uint8_t b = shared->check_fw_b_result;
		uint8_t best_check;

		/* No good firmware, so go to recovery mode. */
		VBDEBUG(("Alas, no good firmware.\n"));
		recovery = VBNV_RECOVERY_RO_INVALID_RW;
		retval = VBERROR_LOAD_FIRMWARE;

		/*
		 * If the best check result fits in the range of recovery
		 * reasons, provide more detail on how far we got in
		 * validation.
		 */
		best_check = (a > b ? a : b) +
			VBNV_RECOVERY_RO_INVALID_RW_CHECK_MIN;
		if (best_check >= VBNV_RECOVERY_RO_INVALID_RW_CHECK_MIN &&
		    best_check <= VBNV_RECOVERY_RO_INVALID_RW_CHECK_MAX)
			recovery = best_check;
	}

 LoadFirmwareExit:
	VbExFree(root_key);

	/* Store recovery request, if any */
	VbNvSet(vnc, VBNV_RECOVERY_REQUEST, VBERROR_SUCCESS != retval ?
		recovery : VBNV_RECOVERY_NOT_REQUESTED);
	/* If the system does not support RO_NORMAL and LoadFirmware()
	 * encountered an error, update the shared recovery reason if
	 * recovery was not previously requested. */
	if (!(shared->flags & VBSD_BOOT_RO_NORMAL_SUPPORT) &&
	    VBNV_RECOVERY_NOT_REQUESTED == shared->recovery_reason &&
	    VBERROR_SUCCESS != retval) {
		VBDEBUG(("RO normal but we got an error.\n"));
		shared->recovery_reason = recovery;
	}

	return retval;
}
