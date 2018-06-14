/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Functions for loading a kernel from disk.
 * (Firmware portion)
 */

#include "sysincludes.h"
#include "2sysincludes.h"

#include "2common.h"
#include "2misc.h"
#include "2nvstorage.h"
#include "2rsa.h"
#include "2sha.h"
#include "cgptlib.h"
#include "cgptlib_internal.h"
#include "gbb_access.h"
#include "gbb_header.h"
#include "gpt_misc.h"
#include "load_kernel_fw.h"
#include "rollback_index.h"
#include "utility.h"
#include "vb2_common.h"
#include "vboot_api.h"
#include "vboot_common.h"
#include "vboot_kernel.h"

#define LOWEST_TPM_VERSION 0xffffffff

enum vboot_mode {
	kBootRecovery = 0,  /* Recovery firmware, any dev switch position */
	kBootNormal = 1,    /* Normal boot - kernel must be verified */
	kBootDev = 2        /* Developer boot - self-signed kernel ok */
};

/**
 * Return the boot mode based on the parameters.
 *
 * @param params	Load kernel parameters
 * @return The current boot mode.
 */
static enum vboot_mode get_kernel_boot_mode(struct vb2_context *ctx)
{
	if (ctx->flags & VB2_CONTEXT_RECOVERY_MODE)
		return kBootRecovery;

	if (ctx->flags & VB2_CONTEXT_DEVELOPER_MODE)
		return kBootDev;

	return kBootNormal;
};

/**
 * Check if the parameters require an officially signed OS.
 *
 * @param params	Load kernel parameters
 * @return 1 if official OS required; 0 if self-signed kernels are ok
 */
static int require_official_os(struct vb2_context *ctx,
			       const LoadKernelParams *params)
{
	/* Normal and recovery modes always require official OS */
	if (get_kernel_boot_mode(ctx) != kBootDev)
		return 1;

	/* FWMP can require developer mode to use official OS */
	if (params->fwmp &&
	    (params->fwmp->flags & FWMP_DEV_ENABLE_OFFICIAL_ONLY))
		return 1;

	/* Developer can request official OS via nvstorage */
	return vb2_nv_get(ctx, VB2_NV_DEV_BOOT_SIGNED_ONLY);
}

/**
 * Return a pointer to the keyblock inside a vblock.
 *
 * Must only be called during or after vb2_verify_kernel_vblock().
 *
 * @param kbuf		Buffer containing vblock
 * @return The keyblock pointer.
 */
static struct vb2_keyblock *get_keyblock(uint8_t *kbuf)
{
	return (struct vb2_keyblock *)kbuf;
}

/**
 * Return a pointer to the kernel preamble inside a vblock.
 *
 * Must only be called during or after vb2_verify_kernel_vblock().
 *
 * @param kbuf		Buffer containing vblock
 * @return The kernel preamble pointer.
 */
static struct vb2_kernel_preamble *get_preamble(uint8_t *kbuf)
{
	return (struct vb2_kernel_preamble *)
			(kbuf + get_keyblock(kbuf)->keyblock_size);
}

/**
 * Return the offset of the kernel body from the start of the vblock.
 *
 * Must only be called during or after vb2_verify_kernel_vblock().
 *
 * @param kbuf		Buffer containing vblock
 * @return The offset of the kernel body from the vblock start, in bytes.
 */
static uint32_t get_body_offset(uint8_t *kbuf)
{
	return (get_keyblock(kbuf)->keyblock_size +
		get_preamble(kbuf)->preamble_size);
}

/**
 * Verify a kernel vblock.
 *
 * @param kbuf		Buffer containing the vblock
 * @param kbuf_size	Size of the buffer in bytes
 * @param kernel_subkey	Packed kernel subkey to use in validating keyblock
 * @param params	Load kernel parameters
 * @param min_version	Minimum kernel version
 * @param shpart	Destination for verification results
 * @param wb		Work buffer.  Must be at least
 *			VB2_VERIFY_KERNEL_PREAMBLE_WORKBUF_BYTES bytes.
 * @return VB2_SUCCESS, or non-zero error code.
 */
int vb2_verify_kernel_vblock(struct vb2_context *ctx,
			     uint8_t *kbuf,
			     uint32_t kbuf_size,
			     const struct vb2_packed_key *kernel_subkey,
			     const LoadKernelParams *params,
			     uint32_t min_version,
			     VbSharedDataKernelPart *shpart,
			     struct vb2_workbuf *wb)
{
	/* Unpack kernel subkey */
	struct vb2_public_key kernel_subkey2;
	if (VB2_SUCCESS != vb2_unpack_key(&kernel_subkey2, kernel_subkey)) {
		VB2_DEBUG("Unable to unpack kernel subkey\n");
		return VB2_ERROR_VBLOCK_KERNEL_SUBKEY;
	}

	/* Verify the key block. */
	int keyblock_valid = 1;  /* Assume valid */
	struct vb2_keyblock *keyblock = get_keyblock(kbuf);
	if (VB2_SUCCESS != vb2_verify_keyblock(keyblock, kbuf_size,
					       &kernel_subkey2, wb)) {
		VB2_DEBUG("Verifying key block signature failed.\n");
		shpart->check_result = VBSD_LKP_CHECK_KEY_BLOCK_SIG;
		keyblock_valid = 0;

		/* Check if we must have an officially signed kernel */
		if (require_official_os(ctx, params)) {
			VB2_DEBUG("Self-signed kernels not enabled.\n");
			shpart->check_result = VBSD_LKP_CHECK_SELF_SIGNED;
			return VB2_ERROR_VBLOCK_SELF_SIGNED;
		}

		/* Otherwise, allow the kernel if the key block hash is valid */
		if (VB2_SUCCESS !=
		    vb2_verify_keyblock_hash(keyblock, kbuf_size, wb)) {
			VB2_DEBUG("Verifying key block hash failed.\n");
			shpart->check_result = VBSD_LKP_CHECK_KEY_BLOCK_HASH;
			return VB2_ERROR_VBLOCK_KEYBLOCK_HASH;
		}
	}

	/* Check the key block flags against boot flags. */
	if (!(keyblock->keyblock_flags &
	      ((ctx->flags & VB2_CONTEXT_DEVELOPER_MODE) ?
	       KEY_BLOCK_FLAG_DEVELOPER_1 : KEY_BLOCK_FLAG_DEVELOPER_0))) {
		VB2_DEBUG("Key block developer flag mismatch.\n");
		shpart->check_result = VBSD_LKP_CHECK_DEV_MISMATCH;
		keyblock_valid = 0;
	}
	if (!(keyblock->keyblock_flags &
	      ((ctx->flags & VB2_CONTEXT_RECOVERY_MODE) ?
	       KEY_BLOCK_FLAG_RECOVERY_1 : KEY_BLOCK_FLAG_RECOVERY_0))) {
		VB2_DEBUG("Key block recovery flag mismatch.\n");
		shpart->check_result = VBSD_LKP_CHECK_REC_MISMATCH;
		keyblock_valid = 0;
	}

	/* Check for rollback of key version except in recovery mode. */
	enum vboot_mode boot_mode = get_kernel_boot_mode(ctx);
	uint32_t key_version = keyblock->data_key.key_version;
	if (kBootRecovery != boot_mode) {
		if (key_version < (min_version >> 16)) {
			VB2_DEBUG("Key version too old.\n");
			shpart->check_result = VBSD_LKP_CHECK_KEY_ROLLBACK;
			keyblock_valid = 0;
		}
		if (key_version > 0xFFFF) {
			/*
			 * Key version is stored in 16 bits in the TPM, so key
			 * versions greater than 0xFFFF can't be stored
			 * properly.
			 */
			VB2_DEBUG("Key version > 0xFFFF.\n");
			shpart->check_result = VBSD_LKP_CHECK_KEY_ROLLBACK;
			keyblock_valid = 0;
		}
	}

	/* If not in developer mode, key block required to be valid. */
	if (kBootDev != boot_mode && !keyblock_valid) {
		VB2_DEBUG("Key block is invalid.\n");
		return VB2_ERROR_VBLOCK_KEYBLOCK;
	}

	/* If in developer mode and using key hash, check it */
	if ((kBootDev == boot_mode) &&
	    params->fwmp && (params->fwmp->flags & FWMP_DEV_USE_KEY_HASH)) {
		struct vb2_packed_key *key = &keyblock->data_key;
		uint8_t *buf = ((uint8_t *)key) + key->key_offset;
		uint32_t buflen = key->key_size;
		uint8_t digest[VB2_SHA256_DIGEST_SIZE];

		VB2_DEBUG("Checking developer key hash.\n");
		vb2_digest_buffer(buf, buflen, VB2_HASH_SHA256,
				  digest, sizeof(digest));
		if (0 != vb2_safe_memcmp(digest, params->fwmp->dev_key_hash,
					 VB2_SHA256_DIGEST_SIZE)) {
			int i;

			VB2_DEBUG("Wrong developer key hash.\n");
			VB2_DEBUG("Want: ");
			for (i = 0; i < VB2_SHA256_DIGEST_SIZE; i++)
				VB2_DEBUG("%02x",
					  params->fwmp->dev_key_hash[i]);
			VB2_DEBUG("\nGot:  ");
			for (i = 0; i < VB2_SHA256_DIGEST_SIZE; i++)
				VB2_DEBUG("%02x", digest[i]);
			VB2_DEBUG("\n");

			return VB2_ERROR_VBLOCK_DEV_KEY_HASH;
		}
	}

	/* Get key for preamble verification from the key block. */
	struct vb2_public_key data_key;
	if (VB2_SUCCESS != vb2_unpack_key(&data_key, &keyblock->data_key)) {
		VB2_DEBUG("Unable to unpack kernel data key\n");
		shpart->check_result = VBSD_LKP_CHECK_DATA_KEY_PARSE;
		return VB2_ERROR_UNKNOWN;
	}

	/* Verify the preamble, which follows the key block */
	struct vb2_kernel_preamble *preamble = get_preamble(kbuf);
	if (VB2_SUCCESS !=
	    vb2_verify_kernel_preamble(preamble,
				       kbuf_size - keyblock->keyblock_size,
				       &data_key,
				       wb)) {
		VB2_DEBUG("Preamble verification failed.\n");
		shpart->check_result = VBSD_LKP_CHECK_VERIFY_PREAMBLE;
		return VB2_ERROR_UNKNOWN;
	}

	/*
	 * If the key block is valid and we're not in recovery mode, check for
	 * rollback of the kernel version.
	 */
	uint32_t combined_version = (key_version << 16) |
			(preamble->kernel_version & 0xFFFF);
	shpart->combined_version = combined_version;
	if (keyblock_valid && kBootRecovery != boot_mode) {
		if (combined_version < min_version) {
			VB2_DEBUG("Kernel version too low.\n");
			shpart->check_result = VBSD_LKP_CHECK_KERNEL_ROLLBACK;
			/*
			 * If not in developer mode, kernel version
			 * must be valid.
			 */
			if (kBootDev != boot_mode)
				return VB2_ERROR_UNKNOWN;
		}
	}

	VB2_DEBUG("Kernel preamble is good.\n");
	shpart->check_result = VBSD_LKP_CHECK_PREAMBLE_VALID;
	if (keyblock_valid)
		shpart->flags |= VBSD_LKP_FLAG_KEY_BLOCK_VALID;

	return VB2_SUCCESS;
}

enum vb2_load_partition_flags {
	/* Only check the vblock to */
	VB2_LOAD_PARTITION_VBLOCK_ONLY = (1 << 0),
};

#define KBUF_SIZE 65536  /* Bytes to read at start of kernel partition */

/* Minimum context work buffer size needed for vb2_load_partition() */
#define VB2_LOAD_PARTITION_WORKBUF_BYTES	\
	(VB2_VERIFY_KERNEL_PREAMBLE_WORKBUF_BYTES + KBUF_SIZE)

/**
 * Load and verify a partition from the stream.
 *
 * @param ctx		Vboot context
 * @param stream	Stream to load kernel from
 * @param kernel_subkey	Key to use to verify vblock
 * @param flags		Flags (one or more of vb2_load_partition_flags)
 * @param params	Load-kernel parameters
 * @param min_version	Minimum kernel version from TPM
 * @param shpart	Destination for verification results
 * @return VB2_SUCCESS, or non-zero error code.
 */
int vb2_load_partition(struct vb2_context *ctx,
		       VbExStream_t stream,
		       const struct vb2_packed_key *kernel_subkey,
		       uint32_t flags,
		       LoadKernelParams *params,
		       uint32_t min_version,
		       VbSharedDataKernelPart *shpart)
{
	struct vb2_workbuf wblocal;
	vb2_workbuf_from_ctx(ctx, &wblocal);

	/* Allocate kernel header buffer in workbuf */
	uint8_t *kbuf = vb2_workbuf_alloc(&wblocal, KBUF_SIZE);
	if (!kbuf)
		return VB2_ERROR_LOAD_PARTITION_WORKBUF;


	if (VbExStreamRead(stream, KBUF_SIZE, kbuf)) {
		VB2_DEBUG("Unable to read start of partition.\n");
		shpart->check_result = VBSD_LKP_CHECK_READ_START;
		return VB2_ERROR_LOAD_PARTITION_READ_VBLOCK;
	}

	if (VB2_SUCCESS !=
	    vb2_verify_kernel_vblock(ctx, kbuf, KBUF_SIZE, kernel_subkey,
				     params, min_version, shpart, &wblocal)) {
		return VB2_ERROR_LOAD_PARTITION_VERIFY_VBLOCK;
	}

	if (flags & VB2_LOAD_PARTITION_VBLOCK_ONLY)
		return VB2_SUCCESS;

	struct vb2_keyblock *keyblock = get_keyblock(kbuf);
	struct vb2_kernel_preamble *preamble = get_preamble(kbuf);

	/*
	 * Make sure the kernel starts at or before what we already read into
	 * kbuf.
	 *
	 * We could deal with a larger offset by reading and discarding the
	 * data in between the vblock and the kernel data.
	 */
	uint32_t body_offset = get_body_offset(kbuf);
	if (body_offset > KBUF_SIZE) {
		shpart->check_result = VBSD_LKP_CHECK_BODY_OFFSET;
		VB2_DEBUG("Kernel body offset is %u > 64KB.\n", body_offset);
		return VB2_ERROR_LOAD_PARTITION_BODY_OFFSET;
	}

	uint8_t *kernbuf = params->kernel_buffer;
	uint32_t kernbuf_size = params->kernel_buffer_size;
	if (!kernbuf) {
		/* Get kernel load address and size from the header. */
		kernbuf = (uint8_t *)((long)preamble->body_load_address);
		kernbuf_size = preamble->body_signature.data_size;
	} else if (preamble->body_signature.data_size > kernbuf_size) {
		VB2_DEBUG("Kernel body doesn't fit in memory.\n");
		shpart->check_result = VBSD_LKP_CHECK_BODY_EXCEEDS_MEM;
		return 	VB2_ERROR_LOAD_PARTITION_BODY_SIZE;
	}

	uint32_t body_toread = preamble->body_signature.data_size;
	uint8_t *body_readptr = kernbuf;

	/*
	 * If we've already read part of the kernel, copy that to the beginning
	 * of the kernel buffer.
	 */
	uint32_t body_copied = KBUF_SIZE - body_offset;
	if (body_copied > body_toread)
		body_copied = body_toread;  /* Don't over-copy tiny kernel */
	memcpy(body_readptr, kbuf + body_offset, body_copied);
	body_toread -= body_copied;
	body_readptr += body_copied;

	/* Read the kernel data */
	if (body_toread && VbExStreamRead(stream, body_toread, body_readptr)) {
		VB2_DEBUG("Unable to read kernel data.\n");
		shpart->check_result = VBSD_LKP_CHECK_READ_DATA;
		return VB2_ERROR_LOAD_PARTITION_READ_BODY;
	}

	/* Get key for preamble/data verification from the key block. */
	struct vb2_public_key data_key;
	if (VB2_SUCCESS != vb2_unpack_key(&data_key, &keyblock->data_key)) {
		VB2_DEBUG("Unable to unpack kernel data key\n");
		shpart->check_result = VBSD_LKP_CHECK_DATA_KEY_PARSE;
		return VB2_ERROR_LOAD_PARTITION_DATA_KEY;
	}

	/* Verify kernel data */
	if (VB2_SUCCESS != vb2_verify_data(kernbuf, kernbuf_size,
					   &preamble->body_signature,
					   &data_key, &wblocal)) {
		VB2_DEBUG("Kernel data verification failed.\n");
		shpart->check_result = VBSD_LKP_CHECK_VERIFY_DATA;
		return VB2_ERROR_LOAD_PARTITION_VERIFY_BODY;
	}

	/* If we're still here, the kernel is valid */
	VB2_DEBUG("Partition is good.\n");
	shpart->check_result = VBSD_LKP_CHECK_KERNEL_GOOD;

	/* Save kernel data back to parameters */
	params->bootloader_address = preamble->bootloader_address;
	params->bootloader_size = preamble->bootloader_size;
	params->flags = vb2_kernel_get_flags(preamble);
	if (!params->kernel_buffer) {
		params->kernel_buffer = kernbuf;
		params->kernel_buffer_size = kernbuf_size;
	}

	return VB2_SUCCESS;
}

VbError_t LoadKernel(struct vb2_context *ctx, LoadKernelParams *params)
{
	struct vb2_shared_data *sd = vb2_get_sd(ctx);
	VbSharedDataHeader *shared = sd->vbsd;
	VbSharedDataKernelCall *shcall = NULL;
	struct vb2_packed_key *recovery_key = NULL;
	int found_partitions = 0;
	uint32_t lowest_version = LOWEST_TPM_VERSION;

	VbError_t retval = VBERROR_UNKNOWN;
	int recovery = VB2_RECOVERY_LK_UNSPECIFIED;

	/* Clear output params in case we fail */
	params->partition_number = 0;
	params->bootloader_address = 0;
	params->bootloader_size = 0;
	params->flags = 0;

	/*
	 * Set up tracking for this call.  This wraps around if called many
	 * times, so we need to initialize the call entry each time.
	 */
	shcall = shared->lk_calls +
			(shared->lk_call_count & (VBSD_MAX_KERNEL_CALLS - 1));
	memset(shcall, 0, sizeof(*shcall));
	shcall->boot_flags = (uint32_t)params->boot_flags;
	shcall->boot_mode = get_kernel_boot_mode(ctx);
	shcall->sector_size = (uint32_t)params->bytes_per_lba;
	shcall->sector_count = params->streaming_lba_count;
	shared->lk_call_count++;

	/* Choose key to verify kernel */
	struct vb2_packed_key *kernel_subkey;
	if (kBootRecovery == shcall->boot_mode) {
		/* Use the recovery key to verify the kernel */
		retval = VbGbbReadRecoveryKey(ctx,
					      (VbPublicKey **)&recovery_key);
		if (VBERROR_SUCCESS != retval)
			goto load_kernel_exit;
		kernel_subkey = recovery_key;
	} else {
		/* Use the kernel subkey passed from firmware verification */
		kernel_subkey = (struct vb2_packed_key *)&shared->kernel_subkey;
	}

	/* Read GPT data */
	GptData gpt;
	gpt.sector_bytes = (uint32_t)params->bytes_per_lba;
	gpt.streaming_drive_sectors = params->streaming_lba_count;
	gpt.gpt_drive_sectors = params->gpt_lba_count;
	gpt.flags = params->boot_flags & BOOT_FLAG_EXTERNAL_GPT
			? GPT_FLAG_EXTERNAL : 0;
	if (0 != AllocAndReadGptData(params->disk_handle, &gpt)) {
		VB2_DEBUG("Unable to read GPT data\n");
		shcall->check_result = VBSD_LKC_CHECK_GPT_READ_ERROR;
		goto gpt_done;
	}

	/* Initialize GPT library */
	if (GPT_SUCCESS != GptInit(&gpt)) {
		VB2_DEBUG("Error parsing GPT\n");
		shcall->check_result = VBSD_LKC_CHECK_GPT_PARSE_ERROR;
		goto gpt_done;
	}

	/* Loop over candidate kernel partitions */
	uint64_t part_start, part_size;
	while (GPT_SUCCESS ==
	       GptNextKernelEntry(&gpt, &part_start, &part_size)) {

		VB2_DEBUG("Found kernel entry at %"
			  PRIu64 " size %" PRIu64 "\n",
			  part_start, part_size);

		/*
		 * Set up tracking for this partition.  This wraps around if
		 * called many times, so initialize the partition entry each
		 * time.
		 */
		VbSharedDataKernelPart *shpart =
				shcall->parts + (shcall->kernel_parts_found
				& (VBSD_MAX_KERNEL_PARTS - 1));
		memset(shpart, 0, sizeof(VbSharedDataKernelPart));
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
		VbExStream_t stream = NULL;
		if (VbExStreamOpen(params->disk_handle,
				   part_start, part_size, &stream)) {
			VB2_DEBUG("Partition error getting stream.\n");
			shpart->check_result = VBSD_LKP_CHECK_TOO_SMALL;
			VB2_DEBUG("Marking kernel as invalid.\n");
			GptUpdateKernelEntry(&gpt, GPT_UPDATE_ENTRY_BAD);
			continue;
		}

		uint32_t lpflags = 0;
		if (params->partition_number > 0) {
			/*
			 * If we already have a good kernel, we only needed to
			 * look at the vblock versions to check for rollback.
			 */
			lpflags |= VB2_LOAD_PARTITION_VBLOCK_ONLY;
		}

		int rv = vb2_load_partition(ctx,
					    stream,
					    kernel_subkey,
					    lpflags,
					    params,
					    shared->kernel_version_tpm,
					    shpart);
		VbExStreamClose(stream);

		if (rv != VB2_SUCCESS) {
			VB2_DEBUG("Marking kernel as invalid.\n");
			GptUpdateKernelEntry(&gpt, GPT_UPDATE_ENTRY_BAD);
			continue;
		}

		int keyblock_valid = (shpart->flags &
				      VBSD_LKP_FLAG_KEY_BLOCK_VALID);
		if (keyblock_valid) {
			shared->flags |= VBSD_KERNEL_KEY_VERIFIED;
			/* Track lowest version from a valid header. */
			if (lowest_version > shpart->combined_version)
				lowest_version = shpart->combined_version;
		}
		VB2_DEBUG("Key block valid: %d\n", keyblock_valid);
		VB2_DEBUG("Combined version: %u\n", shpart->combined_version);

		/*
		 * If we're only looking at headers, we're done with this
		 * partition.
		 */
		if (lpflags & VB2_LOAD_PARTITION_VBLOCK_ONLY)
			continue;

		/*
		 * Otherwise, we found a partition we like.
		 *
		 * TODO: GPT partitions start at 1, but cgptlib starts them at
		 * 0.  Adjust here, until cgptlib is fixed.
		 */
		params->partition_number = gpt.current_kernel + 1;

		/*
		 * TODO: GetCurrentKernelUniqueGuid() should take a destination
		 * size, or the dest should be a struct, so we know it's big
		 * enough.
		 */
		GetCurrentKernelUniqueGuid(&gpt, &params->partition_guid);

		/* Update GPT to note this is the kernel we're trying.
		 * But not when we assume that the boot process may
		 * not complete for valid reasons (eg. early shutdown).
		 */
		if (!(shared->flags & VBSD_NOFAIL_BOOT))
			GptUpdateKernelEntry(&gpt, GPT_UPDATE_ENTRY_TRY);

		/*
		 * If we're in recovery mode or we're about to boot a
		 * non-officially-signed kernel, there's no rollback
		 * protection, so we can stop at the first valid kernel.
		 */
		if (kBootRecovery == shcall->boot_mode || !keyblock_valid) {
			VB2_DEBUG("In recovery mode or dev-signed kernel\n");
			break;
		}

		/*
		 * Otherwise, we do care about the key index in the TPM.  If
		 * the good partition's key version is the same as the tpm,
		 * then the TPM doesn't need updating; we can stop now.
		 * Otherwise, we'll check all the other headers to see if they
		 * contain a newer key.
		 */
		if (shpart->combined_version == shared->kernel_version_tpm) {
			VB2_DEBUG("Same kernel version\n");
			break;
		}
	} /* while(GptNextKernelEntry) */

gpt_done:
	/* Write and free GPT data */
	WriteAndFreeGptData(params->disk_handle, &gpt);

	/* Handle finding a good partition */
	if (params->partition_number > 0) {
		VB2_DEBUG("Good partition %d\n", params->partition_number);
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
		recovery = VB2_RECOVERY_RW_INVALID_OS;
		retval = VBERROR_INVALID_KERNEL_FOUND;
	} else {
		shcall->check_result = VBSD_LKC_CHECK_NO_PARTITIONS;
		recovery = VB2_RECOVERY_RW_NO_OS;
		retval = VBERROR_NO_KERNEL_FOUND;
	}

load_kernel_exit:
	/* Store recovery request, if any */
	vb2_nv_set(ctx, VB2_NV_RECOVERY_REQUEST,
		   VBERROR_SUCCESS != retval ?
		   recovery : VB2_RECOVERY_NOT_REQUESTED);

	free(recovery_key);

	shcall->return_code = (uint8_t)retval;
	return retval;
}
