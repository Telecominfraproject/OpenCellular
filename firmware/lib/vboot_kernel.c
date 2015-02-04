/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Functions for loading a kernel from disk.
 * (Firmware portion)
 */

#include "sysincludes.h"

#include "cgptlib.h"
#include "cgptlib_internal.h"
#include "region.h"
#include "gbb_access.h"
#include "gbb_header.h"
#include "gpt_misc.h"
#include "load_kernel_fw.h"
#include "utility.h"
#include "vboot_api.h"
#include "vboot_common.h"
#include "vboot_kernel.h"

#define KBUF_SIZE 65536  /* Bytes to read at start of kernel partition */
#define LOWEST_TPM_VERSION 0xffffffff

typedef enum BootMode {
	kBootRecovery = 0,  /* Recovery firmware, any dev switch position */
	kBootNormal = 1,    /* Normal boot - kernel must be verified */
	kBootDev = 2        /* Developer boot - self-signed kernel ok */
} BootMode;

VbError_t LoadKernel(LoadKernelParams *params, VbCommonParams *cparams)
{
	VbSharedDataHeader *shared =
		(VbSharedDataHeader *)params->shared_data_blob;
	VbSharedDataKernelCall *shcall = NULL;
	VbNvContext* vnc = params->nv_context;
	VbPublicKey* kernel_subkey = NULL;
	int free_kernel_subkey = 0;
	GptData gpt;
	uint64_t part_start, part_size;
	uint64_t blba;
	uint64_t kbuf_sectors;
	uint8_t* kbuf = NULL;
	int found_partitions = 0;
	int good_partition = -1;
	int good_partition_key_block_valid = 0;
	uint32_t lowest_version = LOWEST_TPM_VERSION;
	int rec_switch, dev_switch;
	BootMode boot_mode;
	uint32_t require_official_os = 0;
	uint32_t body_toread;
	uint8_t *body_readptr;

	VbError_t retval = VBERROR_UNKNOWN;
	int recovery = VBNV_RECOVERY_LK_UNSPECIFIED;

	/* Sanity Checks */
	if (!params->bytes_per_lba ||
	    !params->streaming_lba_count) {
		VBDEBUG(("LoadKernel() called with invalid params\n"));
		retval = VBERROR_INVALID_PARAMETER;
		goto LoadKernelExit;
	}

	/* Clear output params in case we fail */
	params->partition_number = 0;
	params->bootloader_address = 0;
	params->bootloader_size = 0;
	params->flags = 0;

	/* Calculate switch positions and boot mode */
	rec_switch = (BOOT_FLAG_RECOVERY & params->boot_flags ? 1 : 0);
	dev_switch = (BOOT_FLAG_DEVELOPER & params->boot_flags ? 1 : 0);
	if (rec_switch) {
		boot_mode = kBootRecovery;
	} else if (dev_switch) {
		boot_mode = kBootDev;
		VbNvGet(vnc, VBNV_DEV_BOOT_SIGNED_ONLY, &require_official_os);
	} else {
		boot_mode = kBootNormal;
	}

	/*
	 * Set up tracking for this call.  This wraps around if called many
	 * times, so we need to initialize the call entry each time.
	 */
	shcall = shared->lk_calls + (shared->lk_call_count
				     & (VBSD_MAX_KERNEL_CALLS - 1));
	Memset(shcall, 0, sizeof(VbSharedDataKernelCall));
	shcall->boot_flags = (uint32_t)params->boot_flags;
	shcall->boot_mode = boot_mode;
	shcall->sector_size = (uint32_t)params->bytes_per_lba;
	shcall->sector_count = params->streaming_lba_count;
	shared->lk_call_count++;

	/* Initialization */
	blba = params->bytes_per_lba;
	kbuf_sectors = KBUF_SIZE / blba;
	if (0 == kbuf_sectors) {
		VBDEBUG(("LoadKernel() called with sector size > KBUF_SIZE\n"));
		retval = VBERROR_INVALID_PARAMETER;
		goto LoadKernelExit;
	}

	if (kBootRecovery == boot_mode) {
		/* Use the recovery key to verify the kernel */
		retval = VbGbbReadRecoveryKey(cparams, &kernel_subkey);
		if (VBERROR_SUCCESS != retval)
			goto LoadKernelExit;
		free_kernel_subkey = 1;
	} else {
		/* Use the kernel subkey passed from LoadFirmware(). */
		kernel_subkey = &shared->kernel_subkey;
	}

	/* Read GPT data */
	gpt.sector_bytes = (uint32_t)blba;
	gpt.streaming_drive_sectors = params->streaming_lba_count;
	gpt.gpt_drive_sectors = params->gpt_lba_count;
	gpt.flags = params->boot_flags & BOOT_FLAG_EXTERNAL_GPT
			? GPT_FLAG_EXTERNAL : 0;
	if (0 != AllocAndReadGptData(params->disk_handle, &gpt)) {
		VBDEBUG(("Unable to read GPT data\n"));
		shcall->check_result = VBSD_LKC_CHECK_GPT_READ_ERROR;
		goto bad_gpt;
	}

	/* Initialize GPT library */
	if (GPT_SUCCESS != GptInit(&gpt)) {
		VBDEBUG(("Error parsing GPT\n"));
		shcall->check_result = VBSD_LKC_CHECK_GPT_PARSE_ERROR;
		goto bad_gpt;
	}

	/* Allocate kernel header buffers */
	kbuf = (uint8_t*)VbExMalloc(KBUF_SIZE);
	if (!kbuf)
		goto bad_gpt;

        /* Loop over candidate kernel partitions */
        while (GPT_SUCCESS ==
	       GptNextKernelEntry(&gpt, &part_start, &part_size)) {
		VbSharedDataKernelPart *shpart = NULL;
		VbKeyBlockHeader *key_block;
		VbKernelPreambleHeader *preamble;
		RSAPublicKey *data_key = NULL;
		VbExStream_t stream = NULL;
		uint64_t key_version;
		uint32_t combined_version;
		uint64_t body_offset;
		int key_block_valid = 1;

		VBDEBUG(("Found kernel entry at %" PRIu64 " size %" PRIu64 "\n",
			 part_start, part_size));

		/*
		 * Set up tracking for this partition.  This wraps around if
		 * called many times, so initialize the partition entry each
		 * time.
		 */
		shpart = shcall->parts + (shcall->kernel_parts_found
					  & (VBSD_MAX_KERNEL_PARTS - 1));
		Memset(shpart, 0, sizeof(VbSharedDataKernelPart));
		shpart->sector_start = part_start;
		shpart->sector_count = part_size;
		/*
		 * TODO: GPT partitions start at 1, but cgptlib starts them at
		 * 0.  Adjust here, until cgptlib is fixed.
		 */
		shpart->gpt_index = (uint8_t)(gpt.current_kernel + 1);
		shcall->kernel_parts_found++;

		/* Found at least one kernel partition. */
		found_partitions++;

		/* Set up the stream */
		if (VbExStreamOpen(params->disk_handle,
				   part_start, part_size, &stream)) {
			VBDEBUG(("Partition error getting stream.\n"));
			shpart->check_result = VBSD_LKP_CHECK_TOO_SMALL;
			goto bad_kernel;
		}

		if (0 != VbExStreamRead(stream, KBUF_SIZE, kbuf)) {
			VBDEBUG(("Unable to read start of partition.\n"));
			shpart->check_result = VBSD_LKP_CHECK_READ_START;
			goto bad_kernel;
		}

		/* Verify the key block. */
		key_block = (VbKeyBlockHeader*)kbuf;
		if (0 != KeyBlockVerify(key_block, KBUF_SIZE,
					kernel_subkey, 0)) {
			VBDEBUG(("Verifying key block signature failed.\n"));
			shpart->check_result = VBSD_LKP_CHECK_KEY_BLOCK_SIG;
			key_block_valid = 0;

			/* If not in developer mode, this kernel is bad. */
			if (kBootDev != boot_mode)
				goto bad_kernel;

			/*
			 * In developer mode, we can explictly disallow
			 * self-signed kernels
			 */
			if (require_official_os) {
				VBDEBUG(("Self-signed kernels not enabled.\n"));
				shpart->check_result =
					VBSD_LKP_CHECK_SELF_SIGNED;
				goto bad_kernel;
			}

			/*
			 * Allow the kernel if the SHA-512 hash of the key
			 * block is valid.
			 */
			if (0 != KeyBlockVerify(key_block, KBUF_SIZE,
						kernel_subkey, 1)) {
				VBDEBUG(("Verifying key block hash failed.\n"));
				shpart->check_result =
					VBSD_LKP_CHECK_KEY_BLOCK_HASH;
				goto bad_kernel;
			}
		}

		/* Check the key block flags against the current boot mode. */
		if (!(key_block->key_block_flags &
		      (dev_switch ? KEY_BLOCK_FLAG_DEVELOPER_1 :
		       KEY_BLOCK_FLAG_DEVELOPER_0))) {
			VBDEBUG(("Key block developer flag mismatch.\n"));
			shpart->check_result = VBSD_LKP_CHECK_DEV_MISMATCH;
			key_block_valid = 0;
		}
		if (!(key_block->key_block_flags &
		      (rec_switch ? KEY_BLOCK_FLAG_RECOVERY_1 :
		       KEY_BLOCK_FLAG_RECOVERY_0))) {
			VBDEBUG(("Key block recovery flag mismatch.\n"));
			shpart->check_result = VBSD_LKP_CHECK_REC_MISMATCH;
			key_block_valid = 0;
		}

		/* Check for rollback of key version except in recovery mode. */
		key_version = key_block->data_key.key_version;
		if (kBootRecovery != boot_mode) {
			if (key_version < (shared->kernel_version_tpm >> 16)) {
				VBDEBUG(("Key version too old.\n"));
				shpart->check_result =
					VBSD_LKP_CHECK_KEY_ROLLBACK;
				key_block_valid = 0;
			}
			if (key_version > 0xFFFF) {
				/*
				 * Key version is stored in 16 bits in the TPM,
				 * so key versions greater than 0xFFFF can't be
				 * stored properly.
				 */
				VBDEBUG(("Key version > 0xFFFF.\n"));
				shpart->check_result =
					VBSD_LKP_CHECK_KEY_ROLLBACK;
				key_block_valid = 0;
			}
		}

		/* If not in developer mode, key block required to be valid. */
		if (kBootDev != boot_mode && !key_block_valid) {
			VBDEBUG(("Key block is invalid.\n"));
			goto bad_kernel;
		}

		/* Get key for preamble/data verification from the key block. */
		data_key = PublicKeyToRSA(&key_block->data_key);
		if (!data_key) {
			VBDEBUG(("Data key bad.\n"));
			shpart->check_result = VBSD_LKP_CHECK_DATA_KEY_PARSE;
			goto bad_kernel;
		}

		/* Verify the preamble, which follows the key block */
		preamble = (VbKernelPreambleHeader *)
			(kbuf + key_block->key_block_size);
		if ((0 != VerifyKernelPreamble(
					preamble,
					KBUF_SIZE - key_block->key_block_size,
					data_key))) {
			VBDEBUG(("Preamble verification failed.\n"));
			shpart->check_result = VBSD_LKP_CHECK_VERIFY_PREAMBLE;
			goto bad_kernel;
		}

		/*
		 * If the key block is valid and we're not in recovery mode,
		 * check for rollback of the kernel version.
		 */
		combined_version = (uint32_t)(
				(key_version << 16) |
				(preamble->kernel_version & 0xFFFF));
		shpart->combined_version = combined_version;
		if (key_block_valid && kBootRecovery != boot_mode) {
			if (combined_version < shared->kernel_version_tpm) {
				VBDEBUG(("Kernel version too low.\n"));
				shpart->check_result =
					VBSD_LKP_CHECK_KERNEL_ROLLBACK;
				/*
				 * If not in developer mode, kernel version
				 * must be valid.
				 */
				if (kBootDev != boot_mode)
					goto bad_kernel;
			}
		}

		VBDEBUG(("Kernel preamble is good.\n"));
		shpart->check_result = VBSD_LKP_CHECK_PREAMBLE_VALID;

		/* Check for lowest version from a valid header. */
		if (key_block_valid && lowest_version > combined_version)
			lowest_version = combined_version;
		else {
			VBDEBUG(("Key block valid: %d\n", key_block_valid));
			VBDEBUG(("Combined version: %u\n",
				 (unsigned) combined_version));
		}

		/*
		 * If we already have a good kernel, no need to read another
		 * one; we only needed to look at the versions to check for
		 * rollback.  So skip to the next kernel preamble.
		 */
		if (-1 != good_partition) {
			VbExStreamClose(stream);
			stream = NULL;
			continue;
		}

		body_offset = key_block->key_block_size +
			preamble->preamble_size;

		/*
		 * Make sure the kernel starts at or before what we already
		 * read into kbuf.
		 *
		 * We could deal with a larger offset by reading and discarding
		 * the data in between the vblock and the kernel data.
		 */
		if (body_offset > KBUF_SIZE) {
			shpart->check_result = VBSD_LKP_CHECK_BODY_OFFSET;
			VBDEBUG(("Kernel body offset is %d > 64KB.\n",
				 (int)body_offset));
			goto bad_kernel;
		}

		if (!params->kernel_buffer) {
			/* Get kernel load address and size from the header. */
			params->kernel_buffer =
				(void *)((long)preamble->body_load_address);
			params->kernel_buffer_size =
				preamble->body_signature.data_size;
		} else if (preamble->body_signature.data_size >
			   params->kernel_buffer_size) {
			VBDEBUG(("Kernel body doesn't fit in memory.\n"));
			shpart->check_result = VBSD_LKP_CHECK_BODY_EXCEEDS_MEM;
			goto bad_kernel;
		}

		/*
		 * Body signature data size is 64 bit and toread is 32 bit so
		 * this could technically cause us to read less data.  That's
		 * fine, because a 4 GB kernel is implausible, and if we did
		 * have one that big, we'd simply read too little data and fail
		 * to verify it.
		 */
		body_toread = preamble->body_signature.data_size;
		body_readptr = params->kernel_buffer;

		/*
		 * If we've already read part of the kernel, copy that to the
		 * beginning of the kernel buffer.
		 */
		if (body_offset < KBUF_SIZE) {
			uint32_t body_copied = KBUF_SIZE - body_offset;

			/* If the kernel is tiny, don't over-copy */
			if (body_copied > body_toread)
				body_copied = body_toread;

			Memcpy(body_readptr, kbuf + body_offset, body_copied);
			body_toread -= body_copied;
			body_readptr += body_copied;
		}

		/* Read the kernel data */
		if (body_toread &&
		    0 != VbExStreamRead(stream, body_toread, body_readptr)) {
			VBDEBUG(("Unable to read kernel data.\n"));
			shpart->check_result = VBSD_LKP_CHECK_READ_DATA;
			goto bad_kernel;
		}

		/* Close the stream; we're done with it */
		VbExStreamClose(stream);
		stream = NULL;

		/* Verify kernel data */
		if (0 != VerifyData((const uint8_t *)params->kernel_buffer,
				    params->kernel_buffer_size,
				    &preamble->body_signature, data_key)) {
			VBDEBUG(("Kernel data verification failed.\n"));
			shpart->check_result = VBSD_LKP_CHECK_VERIFY_DATA;
			goto bad_kernel;
		}

		/* Done with the kernel signing key, so can free it now */
		RSAPublicKeyFree(data_key);
		data_key = NULL;

		/*
		 * If we're still here, the kernel is valid.  Save the first
		 * good partition we find; that's the one we'll boot.
		 */
		VBDEBUG(("Partition is good.\n"));
		shpart->check_result = VBSD_LKP_CHECK_KERNEL_GOOD;
		if (key_block_valid)
			shpart->flags |= VBSD_LKP_FLAG_KEY_BLOCK_VALID;

		good_partition_key_block_valid = key_block_valid;
		/*
		 * TODO: GPT partitions start at 1, but cgptlib starts them at
		 * 0.  Adjust here, until cgptlib is fixed.
		 */
		good_partition = gpt.current_kernel + 1;
		params->partition_number = gpt.current_kernel + 1;
		GetCurrentKernelUniqueGuid(&gpt, &params->partition_guid);
		/*
		 * TODO: GetCurrentKernelUniqueGuid() should take a destination
		 * size, or the dest should be a struct, so we know it's big
		 * enough.
		 */
		params->bootloader_address = preamble->bootloader_address;
		params->bootloader_size = preamble->bootloader_size;
		if (VbKernelHasFlags(preamble) == VBOOT_SUCCESS)
			params->flags = preamble->flags;

		/* Update GPT to note this is the kernel we're trying */
		GptUpdateKernelEntry(&gpt, GPT_UPDATE_ENTRY_TRY);

		/*
		 * If we're in recovery mode or we're about to boot a
		 * dev-signed kernel, there's no rollback protection, so we can
		 * stop at the first valid kernel.
		 */
		if (kBootRecovery == boot_mode || !key_block_valid) {
			VBDEBUG(("In recovery mode or dev-signed kernel\n"));
			break;
		}

		/*
		 * Otherwise, we do care about the key index in the TPM.  If
		 * the good partition's key version is the same as the tpm,
		 * then the TPM doesn't need updating; we can stop now.
		 * Otherwise, we'll check all the other headers to see if they
		 * contain a newer key.
		 */
		if (combined_version == shared->kernel_version_tpm) {
			VBDEBUG(("Same kernel version\n"));
			break;
		}

		/* Continue, so that we skip the error handling code below */
		continue;

	bad_kernel:
		/* Handle errors parsing this kernel */
		if (NULL != stream)
			VbExStreamClose(stream);
		if (NULL != data_key)
			RSAPublicKeyFree(data_key);

		VBDEBUG(("Marking kernel as invalid.\n"));
		GptUpdateKernelEntry(&gpt, GPT_UPDATE_ENTRY_BAD);


        } /* while(GptNextKernelEntry) */

 bad_gpt:

	/* Free kernel buffer */
	if (kbuf)
		VbExFree(kbuf);

	/* Write and free GPT data */
	WriteAndFreeGptData(params->disk_handle, &gpt);

	/* Handle finding a good partition */
	if (good_partition >= 0) {
		VBDEBUG(("Good_partition >= 0\n"));
		shcall->check_result = VBSD_LKC_CHECK_GOOD_PARTITION;
		shared->kernel_version_lowest = lowest_version;
		/*
		 * Sanity check - only store a new TPM version if we found one.
		 * If lowest_version is still at its initial value, we didn't
		 * find one; for example, we're in developer mode and just
		 * didn't look.
		 */
		if (lowest_version != LOWEST_TPM_VERSION &&
		    lowest_version > shared->kernel_version_tpm)
			shared->kernel_version_tpm = lowest_version;

		/* Success! */
		retval = VBERROR_SUCCESS;
	} else if (found_partitions > 0) {
		shcall->check_result = VBSD_LKC_CHECK_INVALID_PARTITIONS;
		recovery = VBNV_RECOVERY_RW_INVALID_OS;
		retval = VBERROR_INVALID_KERNEL_FOUND;
	} else {
		shcall->check_result = VBSD_LKC_CHECK_NO_PARTITIONS;
		recovery = VBNV_RECOVERY_RW_NO_OS;
		retval = VBERROR_NO_KERNEL_FOUND;
	}

 LoadKernelExit:

	/* Store recovery request, if any */
	VbNvSet(vnc, VBNV_RECOVERY_REQUEST, VBERROR_SUCCESS != retval ?
		recovery : VBNV_RECOVERY_NOT_REQUESTED);

	/*
	 * If LoadKernel() was called with bad parameters, shcall may not be
	 * initialized.
	 */
	if (shcall)
		shcall->return_code = (uint8_t)retval;

	/* Save whether the good partition's key block was fully verified */
	if (good_partition_key_block_valid)
		shared->flags |= VBSD_KERNEL_KEY_VERIFIED;

	/* Store how much shared data we used, if any */
	params->shared_data_size = shared->data_used;

	if (free_kernel_subkey)
		VbExFree(kernel_subkey);

	return retval;
}
