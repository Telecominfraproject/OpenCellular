/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * High-level firmware wrapper API - entry points for kernel selection
 */

#include "sysincludes.h"

#include "2sysincludes.h"
#include "2common.h"
#include "2misc.h"
#include "2nvstorage.h"
#include "2rsa.h"
#include "ec_sync.h"
#include "gbb_access.h"
#include "gbb_header.h"
#include "load_kernel_fw.h"
#include "region.h"
#include "rollback_index.h"
#include "utility.h"
#include "vb2_common.h"
#include "vboot_api.h"
#include "vboot_common.h"
#include "vboot_kernel.h"
#include "vboot_nvstorage.h"

/* Global variables */
static VbNvContext vnc;
static struct RollbackSpaceFwmp fwmp;
static LoadKernelParams lkp;
static struct vb2_context ctx;
static uint8_t *unaligned_workbuf;

#ifdef CHROMEOS_ENVIRONMENT
/* Global variable accessors for unit tests */

struct RollbackSpaceFwmp *VbApiKernelGetFwmp(void)
{
	return &fwmp;
}

struct LoadKernelParams *VbApiKernelGetParams(void)
{
	return &lkp;
}

#endif

/**
 * Set recovery request (called from vboot_api_kernel.c functions only)
 */
static void VbSetRecoveryRequest(struct vb2_context *ctx,
				 uint32_t recovery_request)
{
	VB2_DEBUG("VbSetRecoveryRequest(%d)\n", (int)recovery_request);
	vb2_nv_set(ctx, VB2_NV_RECOVERY_REQUEST, recovery_request);
}

static void VbNvLoad(void)
{
	VbExNvStorageRead(vnc.raw);
	VbNvSetup(&vnc);
}

static void VbNvCommit(void)
{
	VbNvTeardown(&vnc);
	if (vnc.raw_changed)
		VbExNvStorageWrite(vnc.raw);
}

void vb2_nv_commit(struct vb2_context *ctx)
{
	/* Copy nvdata back to old vboot1 nv context if needed */
	if (ctx->flags & VB2_CONTEXT_NVDATA_CHANGED) {
		memcpy(vnc.raw, ctx->nvdata, VB2_NVDATA_SIZE);
		vnc.raw_changed = 1;
		ctx->flags &= ~VB2_CONTEXT_NVDATA_CHANGED;
	}

	VbNvCommit();
}

uint32_t vb2_get_fwmp_flags(void)
{
	return fwmp.flags;
}

/**
 * Attempt loading a kernel from the specified type(s) of disks.
 *
 * If successful, sets p->disk_handle to the disk for the kernel and returns
 * VBERROR_SUCCESS.
 *
 * @param ctx			Vboot context
 * @param cparams		Vboot common params
 * @param p			Parameters for loading kernel
 * @param get_info_flags	Flags to pass to VbExDiskGetInfo()
 * @return VBERROR_SUCCESS, VBERROR_NO_DISK_FOUND if no disks of the specified
 * type were found, or other non-zero VBERROR_ codes for other failures.
 */
uint32_t VbTryLoadKernel(struct vb2_context *ctx, VbCommonParams *cparams,
                         uint32_t get_info_flags)
{
	VbError_t retval = VBERROR_UNKNOWN;
	VbDiskInfo* disk_info = NULL;
	uint32_t disk_count = 0;
	uint32_t i;

	VB2_DEBUG("VbTryLoadKernel() start, get_info_flags=0x%x\n",
		  (unsigned)get_info_flags);

	lkp.fwmp = &fwmp;
	lkp.nv_context = &vnc;
	lkp.disk_handle = NULL;

	/* Find disks */
	if (VBERROR_SUCCESS != VbExDiskGetInfo(&disk_info, &disk_count,
					       get_info_flags))
		disk_count = 0;

	VB2_DEBUG("VbTryLoadKernel() found %d disks\n", (int)disk_count);
	if (0 == disk_count) {
		VbSetRecoveryRequest(ctx, VBNV_RECOVERY_RW_NO_DISK);
		return VBERROR_NO_DISK_FOUND;
	}

	/* Loop over disks */
	for (i = 0; i < disk_count; i++) {
		VB2_DEBUG("VbTryLoadKernel() trying disk %d\n", (int)i);
		/*
		 * Sanity-check what we can. FWIW, VbTryLoadKernel() is always
		 * called with only a single bit set in get_info_flags.
		 *
		 * Ensure 512-byte sectors and non-trivially sized disk (for
		 * cgptlib) and that we got a partition with only the flags we
		 * asked for.
		 */
		if (512 != disk_info[i].bytes_per_lba ||
		    16 > disk_info[i].lba_count ||
		    get_info_flags != (disk_info[i].flags &
				       ~VB_DISK_FLAG_EXTERNAL_GPT)) {
			VB2_DEBUG("  skipping: bytes_per_lba=%" PRIu64
				  " lba_count=%" PRIu64 " flags=0x%x\n",
				  disk_info[i].bytes_per_lba,
				  disk_info[i].lba_count,
				  disk_info[i].flags);
			continue;
		}
		lkp.disk_handle = disk_info[i].handle;
		lkp.bytes_per_lba = disk_info[i].bytes_per_lba;
		lkp.gpt_lba_count = disk_info[i].lba_count;
		lkp.streaming_lba_count = disk_info[i].streaming_lba_count
						?: lkp.gpt_lba_count;
		lkp.boot_flags |= disk_info[i].flags & VB_DISK_FLAG_EXTERNAL_GPT
				? BOOT_FLAG_EXTERNAL_GPT : 0;
		retval = LoadKernel(ctx, &lkp, cparams);
		VB2_DEBUG("VbTryLoadKernel() LoadKernel() = %d\n", retval);

		/*
		 * Stop now if we found a kernel.
		 *
		 * TODO: If recovery requested, should track the farthest we
		 * get, instead of just returning the value from the last disk
		 * attempted.
		 */
		if (VBERROR_SUCCESS == retval)
			break;
	}

	/* If we didn't find any good kernels, don't return a disk handle. */
	if (VBERROR_SUCCESS != retval) {
		VbSetRecoveryRequest(ctx, VBNV_RECOVERY_RW_NO_KERNEL);
		lkp.disk_handle = NULL;
	}

	VbExDiskFreeInfo(disk_info, lkp.disk_handle);

	/*
	 * Pass through return code.  Recovery reason (if any) has already been
	 * set by LoadKernel().
	 */
	return retval;
}

VbError_t VbBootNormal(struct vb2_context *ctx, VbCommonParams *cparams)
{
	VbSharedDataHeader *shared =
		(VbSharedDataHeader *)cparams->shared_data_blob;

	/* Boot from fixed disk only */
	VB2_DEBUG("Entering\n");

	VbError_t rv = VbTryLoadKernel(ctx, cparams, VB_DISK_FLAG_FIXED);

	VB2_DEBUG("Checking if TPM kernel version needs advancing\n");

	if ((1 == shared->firmware_index) && (shared->flags & VBSD_FWB_TRIED)) {
		/*
		 * Special cases for when we're trying a new firmware B.  These
		 * are needed because firmware updates also usually change the
		 * kernel key, which means that the B firmware can only boot a
		 * new kernel, and the old firmware in A can only boot the
		 * previous kernel.
		 *
		 * Don't advance the TPM if we're trying a new firmware B,
		 * because we don't yet know if the new kernel will
		 * successfully boot.  We still want to be able to fall back to
		 * the previous firmware+kernel if the new firmware+kernel
		 * fails.
		 *
		 * If we found only invalid kernels, reboot and try again.
		 * This allows us to fall back to the previous firmware+kernel
		 * instead of giving up and going to recovery mode right away.
		 * We'll still go to recovery mode if we run out of tries and
		 * the old firmware can't find a kernel it likes.
		 */
		if (rv == VBERROR_INVALID_KERNEL_FOUND) {
			VB2_DEBUG("Trying FW B; only found invalid kernels.\n");
			VbSetRecoveryRequest(ctx, VBNV_RECOVERY_NOT_REQUESTED);
		}

		return rv;
	}

	if ((shared->kernel_version_tpm > shared->kernel_version_tpm_start) &&
	    RollbackKernelWrite(shared->kernel_version_tpm)) {
		VB2_DEBUG("Error writing kernel versions to TPM.\n");
		VbSetRecoveryRequest(ctx, VBNV_RECOVERY_RW_TPM_W_ERROR);
		return VBERROR_TPM_WRITE_KERNEL;
	}

	return rv;
}

/* This function is also used by tests */
void VbApiKernelFree(VbCommonParams *cparams)
{
	/* VbSelectAndLoadKernel() always allocates this, tests don't */
	if (cparams->gbb) {
		free(cparams->gbb);
		cparams->gbb = NULL;
	}
	if (cparams->bmp) {
		free(cparams->bmp);
		cparams->bmp = NULL;
	}
}

static VbError_t vb2_kernel_setup(VbCommonParams *cparams,
				  VbSelectAndLoadKernelParams *kparams)
{
	VbSharedDataHeader *shared =
		(VbSharedDataHeader *)cparams->shared_data_blob;

	/* Start timer */
	shared->timer_vb_select_and_load_kernel_enter = VbExGetTimer();

	/*
	 * Set up vboot context.
	 *
	 * TODO: Propagate this up to higher API levels, and use more of the
	 * context fields (e.g. secdatak) and flags.
	 */
	memset(&ctx, 0, sizeof(ctx));

	VbNvLoad();
	memcpy(ctx.nvdata, vnc.raw, VB2_NVDATA_SIZE);

	if (shared->recovery_reason)
		ctx.flags |= VB2_CONTEXT_RECOVERY_MODE;
	if (shared->flags & VBSD_BOOT_DEV_SWITCH_ON)
		ctx.flags |= VB2_CONTEXT_DEVELOPER_MODE;

	ctx.workbuf_size = VB2_KERNEL_WORKBUF_RECOMMENDED_SIZE +
			VB2_WORKBUF_ALIGN;

	unaligned_workbuf = ctx.workbuf = malloc(ctx.workbuf_size);
	if (!unaligned_workbuf) {
		VB2_DEBUG("Can't allocate work buffer\n");
		VbSetRecoveryRequest(&ctx, VB2_RECOVERY_RW_SHARED_DATA);
		return VBERROR_INIT_SHARED_DATA;
	}

	if (VB2_SUCCESS != vb2_align(&ctx.workbuf, &ctx.workbuf_size,
				     VB2_WORKBUF_ALIGN,
				     VB2_KERNEL_WORKBUF_RECOMMENDED_SIZE)) {
		VB2_DEBUG("Can't align work buffer\n");
		VbSetRecoveryRequest(&ctx, VB2_RECOVERY_RW_SHARED_DATA);
		return VBERROR_INIT_SHARED_DATA;
	}

	if (VB2_SUCCESS != vb2_init_context(&ctx)) {
		VB2_DEBUG("Can't init vb2_context\n");
		free(unaligned_workbuf);
		VbSetRecoveryRequest(&ctx, VB2_RECOVERY_RW_SHARED_DATA);
		return VBERROR_INIT_SHARED_DATA;
	}

	struct vb2_shared_data *sd = vb2_get_sd(&ctx);
	sd->recovery_reason = shared->recovery_reason;

	/*
	 * If we're in recovery mode just to do memory retraining, all we
	 * need to do is reboot.
	 */
	if (shared->recovery_reason == VBNV_RECOVERY_TRAIN_AND_REBOOT) {
		VB2_DEBUG("Reboot after retraining in recovery.\n");
		return VBERROR_REBOOT_REQUIRED;
	}

	/* Fill in params for calls to LoadKernel() */
	memset(&lkp, 0, sizeof(lkp));
	lkp.kernel_buffer = kparams->kernel_buffer;
	lkp.kernel_buffer_size = kparams->kernel_buffer_size;

	/* Clear output params in case we fail */
	kparams->disk_handle = NULL;
	kparams->partition_number = 0;
	kparams->bootloader_address = 0;
	kparams->bootloader_size = 0;
	kparams->flags = 0;
	memset(kparams->partition_guid, 0, sizeof(kparams->partition_guid));

	/* Read GBB header, since we'll needs flags from it */
	cparams->bmp = NULL;
	cparams->gbb = malloc(sizeof(*cparams->gbb));
	uint32_t retval = VbGbbReadHeader_static(cparams, cparams->gbb);
	if (retval)
		return retval;

	/* Read kernel version from the TPM.  Ignore errors in recovery mode. */
	if (RollbackKernelRead(&shared->kernel_version_tpm)) {
		VB2_DEBUG("Unable to get kernel versions from TPM\n");
		if (!shared->recovery_reason) {
			VbSetRecoveryRequest(&ctx,
					     VBNV_RECOVERY_RW_TPM_R_ERROR);
			return VBERROR_TPM_READ_KERNEL;
		}
	}

	shared->kernel_version_tpm_start = shared->kernel_version_tpm;

	/* Read FWMP.  Ignore errors in recovery mode. */
	if (cparams->gbb->flags & GBB_FLAG_DISABLE_FWMP) {
		memset(&fwmp, 0, sizeof(fwmp));
	} else if (RollbackFwmpRead(&fwmp)) {
		VB2_DEBUG("Unable to get FWMP from TPM\n");
		if (!shared->recovery_reason) {
			VbSetRecoveryRequest(&ctx,
					     VBNV_RECOVERY_RW_TPM_R_ERROR);
			return VBERROR_TPM_READ_FWMP;
		}
	}

	return VBERROR_SUCCESS;
}

static VbError_t vb2_kernel_phase4(VbCommonParams *cparams,
				   VbSelectAndLoadKernelParams *kparams)
{
	VbSharedDataHeader *shared =
		(VbSharedDataHeader *)cparams->shared_data_blob;

	/* Save disk parameters */
	kparams->disk_handle = lkp.disk_handle;
	kparams->partition_number = lkp.partition_number;
	kparams->bootloader_address = lkp.bootloader_address;
	kparams->bootloader_size = lkp.bootloader_size;
	kparams->flags = lkp.flags;
	kparams->kernel_buffer = lkp.kernel_buffer;
	kparams->kernel_buffer_size = lkp.kernel_buffer_size;
	memcpy(kparams->partition_guid, lkp.partition_guid,
	       sizeof(kparams->partition_guid));

	/* Lock the kernel versions if not in recovery mode */
	if (!shared->recovery_reason &&
	    RollbackKernelLock(shared->recovery_reason)) {
		VB2_DEBUG("Error locking kernel versions.\n");
		VbSetRecoveryRequest(&ctx, VBNV_RECOVERY_RW_TPM_L_ERROR);
		return VBERROR_TPM_LOCK_KERNEL;
	}

	return VBERROR_SUCCESS;
}

static void vb2_kernel_cleanup(struct vb2_context *ctx, VbCommonParams *cparams)
{
	VbSharedDataHeader *shared =
			(VbSharedDataHeader *)cparams->shared_data_blob;

	/*
	 * Clean up vboot context.
	 *
	 * TODO: This should propagate up to higher levels
	 */

	/* Free buffers */
	free(unaligned_workbuf);
	VbApiKernelFree(cparams);

	vb2_nv_commit(ctx);

	/* Stop timer */
	shared->timer_vb_select_and_load_kernel_exit = VbExGetTimer();
}

VbError_t VbSelectAndLoadKernel(VbCommonParams *cparams,
                                VbSelectAndLoadKernelParams *kparams)
{
	VbSharedDataHeader *shared =
		(VbSharedDataHeader *)cparams->shared_data_blob;

	VbError_t retval = vb2_kernel_setup(cparams, kparams);
	if (retval)
		goto VbSelectAndLoadKernel_exit;

	/*
	 * Do EC software sync unless we're in recovery mode. This has UI but
	 * it's just a single non-interactive WAIT screen.
	 */
	if (shared->recovery_reason == VB2_RECOVERY_NOT_REQUESTED) {
		retval = ec_sync_all(&ctx, cparams);
		if (retval)
			goto VbSelectAndLoadKernel_exit;
	}

	/* Select boot path */
	if (shared->recovery_reason) {
		/* Recovery boot.  This has UI. */
		if (kparams->inflags & VB_SALK_INFLAGS_ENABLE_DETACHABLE_UI)
			retval = VbBootRecoveryMenu(&ctx, cparams);
		else
			retval = VbBootRecovery(&ctx, cparams);
		VbExEcEnteringMode(0, VB_EC_RECOVERY);
	} else if (shared->flags & VBSD_BOOT_DEV_SWITCH_ON) {
		/* Developer boot.  This has UI. */
		if (kparams->inflags & VB_SALK_INFLAGS_ENABLE_DETACHABLE_UI)
			retval = VbBootDeveloperMenu(&ctx, cparams);
		else
			retval = VbBootDeveloper(&ctx, cparams);
		VbExEcEnteringMode(0, VB_EC_DEVELOPER);
	} else {
		/* Normal boot */
		retval = VbBootNormal(&ctx, cparams);
		VbExEcEnteringMode(0, VB_EC_NORMAL);
	}

 VbSelectAndLoadKernel_exit:

	if (VBERROR_SUCCESS == retval)
		retval = vb2_kernel_phase4(cparams, kparams);

	vb2_kernel_cleanup(&ctx, cparams);

	/* Pass through return value from boot path */
	VB2_DEBUG("Returning %d\n", (int)retval);
	return retval;
}

VbError_t VbVerifyMemoryBootImage(VbCommonParams *cparams,
				  VbSelectAndLoadKernelParams *kparams,
				  void *boot_image,
				  size_t image_size)
{
	VbError_t retval;
	VbPublicKey* kernel_subkey = NULL;
	uint8_t *kbuf;
	VbKeyBlockHeader *key_block;
	VbSharedDataHeader *shared =
		(VbSharedDataHeader *)cparams->shared_data_blob;
	VbKernelPreambleHeader *preamble;
	uint64_t body_offset;
	int hash_only = 0;
	int dev_switch;
	uint32_t allow_fastboot_full_cap = 0;
	uint8_t *workbuf = NULL;
	struct vb2_workbuf wb;

	if ((boot_image == NULL) || (image_size == 0))
		return VBERROR_INVALID_PARAMETER;

	/* Clear output params in case we fail. */
	kparams->disk_handle = NULL;
	kparams->partition_number = 0;
	kparams->bootloader_address = 0;
	kparams->bootloader_size = 0;
	kparams->flags = 0;
	memset(kparams->partition_guid, 0, sizeof(kparams->partition_guid));

	kbuf = boot_image;

	/* Read GBB Header */
	cparams->bmp = NULL;
	cparams->gbb = malloc(sizeof(*cparams->gbb));
	retval = VbGbbReadHeader_static(cparams, cparams->gbb);
	if (VBERROR_SUCCESS != retval) {
		VB2_DEBUG("Gbb read header failed.\n");
		return retval;
	}

	/*
	 * We don't care verifying the image if:
	 * 1. dev-mode switch is on and
	 * 2a. GBB_FLAG_FORCE_DEV_BOOT_FASTBOOT_FULL_CAP is set, or
	 * 2b. DEV_BOOT_FASTBOOT_FULL_CAP flag is set in NvStorage
	 *
	 * Check only the integrity of the image.
	 */
	dev_switch = shared->flags & VBSD_BOOT_DEV_SWITCH_ON;

	VbNvLoad();
	VbNvGet(&vnc, VBNV_DEV_BOOT_FASTBOOT_FULL_CAP,
		&allow_fastboot_full_cap);

	if (0 == allow_fastboot_full_cap) {
		allow_fastboot_full_cap = !!(cparams->gbb->flags &
				GBB_FLAG_FORCE_DEV_BOOT_FASTBOOT_FULL_CAP);
	}

	if (dev_switch && allow_fastboot_full_cap) {
		VB2_DEBUG("Only performing integrity-check.\n");
		hash_only = 1;
	} else {
		/* Get recovery key. */
		retval = VbGbbReadRecoveryKey(cparams, &kernel_subkey);
		if (VBERROR_SUCCESS != retval) {
			VB2_DEBUG("Gbb Read Recovery key failed.\n");
			return retval;
		}
	}

	/* If we fail at any step, retval returned would be invalid kernel. */
	retval = VBERROR_INVALID_KERNEL_FOUND;

	/* Allocate work buffer */
	workbuf = (uint8_t *)malloc(VB2_KERNEL_WORKBUF_RECOMMENDED_SIZE);
	if (!workbuf)
		goto fail;
	vb2_workbuf_init(&wb, workbuf, VB2_KERNEL_WORKBUF_RECOMMENDED_SIZE);

	/* Verify the key block. */
	key_block = (VbKeyBlockHeader *)kbuf;
	struct vb2_keyblock *keyblock2 = (struct vb2_keyblock *)kbuf;
	int rv;
	if (hash_only) {
		rv = vb2_verify_keyblock_hash(keyblock2, image_size, &wb);
	} else {
		/* Unpack kernel subkey */
		struct vb2_public_key kernel_subkey2;
		if (VB2_SUCCESS !=
		    vb2_unpack_key(&kernel_subkey2,
				   (struct vb2_packed_key *)kernel_subkey)) {
			VB2_DEBUG("Unable to unpack kernel subkey\n");
			goto fail;
		}
		rv = vb2_verify_keyblock(keyblock2, image_size,
					 &kernel_subkey2, &wb);
	}

	if (VB2_SUCCESS != rv) {
		VB2_DEBUG("Verifying key block signature/hash failed.\n");
		goto fail;
	}

	/* Check the key block flags against the current boot mode. */
	if (!(key_block->key_block_flags &
	      (dev_switch ? KEY_BLOCK_FLAG_DEVELOPER_1 :
	       KEY_BLOCK_FLAG_DEVELOPER_0))) {
		VB2_DEBUG("Key block developer flag mismatch.\n");
		if (hash_only == 0)
			goto fail;
	}

	if (!(key_block->key_block_flags & KEY_BLOCK_FLAG_RECOVERY_1)) {
		VB2_DEBUG("Key block recovery flag mismatch.\n");
		if (hash_only == 0)
			goto fail;
	}

	/* Get key for preamble/data verification from the key block. */
	struct vb2_public_key data_key2;
	if (VB2_SUCCESS != vb2_unpack_key(&data_key2, &keyblock2->data_key)) {
		VB2_DEBUG("Unable to unpack kernel data key\n");
		goto fail;
	}

	/* Verify the preamble, which follows the key block */
	preamble = (VbKernelPreambleHeader *)(kbuf + key_block->key_block_size);
	struct vb2_kernel_preamble *preamble2 =
			(struct vb2_kernel_preamble *)
			(kbuf + key_block->key_block_size);

	if (VB2_SUCCESS != vb2_verify_kernel_preamble(
			preamble2,
			image_size - key_block->key_block_size,
			&data_key2,
			&wb)) {
		VB2_DEBUG("Preamble verification failed.\n");
		goto fail;
	}

	VB2_DEBUG("Kernel preamble is good.\n");

	/* Verify kernel data */
	body_offset = key_block->key_block_size + preamble->preamble_size;
	if (VB2_SUCCESS != vb2_verify_data(
			(const uint8_t *)(kbuf + body_offset),
			image_size - body_offset,
			(struct vb2_signature *)&preamble->body_signature,
			&data_key2, &wb)) {
		VB2_DEBUG("Kernel data verification failed.\n");
		goto fail;
	}

	VB2_DEBUG("Kernel is good.\n");

	/* Fill in output parameters. */
	kparams->kernel_buffer = kbuf + body_offset;
	kparams->kernel_buffer_size = image_size - body_offset;
	kparams->bootloader_address = preamble->bootloader_address;
	kparams->bootloader_size = preamble->bootloader_size;
	if (VbKernelHasFlags(preamble) == VBOOT_SUCCESS)
		kparams->flags = preamble->flags;

	retval = VBERROR_SUCCESS;

fail:
	VbApiKernelFree(cparams);
	if (NULL != kernel_subkey)
		free(kernel_subkey);
	if (NULL != workbuf)
		free(workbuf);
	return retval;
}

VbError_t VbUnlockDevice(void)
{
	VB2_DEBUG("Enabling dev-mode...\n");
	if (TPM_SUCCESS != SetVirtualDevMode(1))
		return VBERROR_TPM_SET_BOOT_MODE_STATE;

	VB2_DEBUG("Mode change will take effect on next reboot.\n");
	return VBERROR_SUCCESS;
}

VbError_t VbLockDevice(void)
{
	VbNvLoad();

	VB2_DEBUG("Storing request to leave dev-mode.\n");
	VbNvSet(&vnc, VBNV_DISABLE_DEV_REQUEST, 1);

	VbNvCommit();

	VB2_DEBUG("Mode change will take effect on next reboot.\n");

	return VBERROR_SUCCESS;
}
