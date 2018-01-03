/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for vboot_api_kernel.c
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "2sysincludes.h"
#include "2api.h"
#include "2common.h"
#include "2misc.h"
#include "2nvstorage.h"
#include "2rsa.h"
#include "gbb_header.h"
#include "host_common.h"
#include "load_kernel_fw.h"
#include "test_common.h"
#include "vb2_common.h"
#include "vboot_api.h"
#include "vboot_common.h"
#include "vboot_kernel.h"
#include "vboot_struct.h"

/* Mock data */
static uint8_t workbuf[VB2_KERNEL_WORKBUF_RECOMMENDED_SIZE];
static struct vb2_context ctx;
static struct vb2_shared_data *sd;
static VbCommonParams cparams;
static VbSelectAndLoadKernelParams kparams;
static uint8_t shared_data[VB_SHARED_DATA_MIN_SIZE];
static VbSharedDataHeader *shared = (VbSharedDataHeader *)shared_data;
static GoogleBinaryBlockHeader gbb;

static uint8_t kernel_buffer[80000];
static int key_block_verify_fail;  /* 0=ok, 1=sig, 2=hash */
static int preamble_verify_fail;
static int verify_data_fail;
static int unpack_key_fail;

static VbKeyBlockHeader kbh;
static VbKernelPreambleHeader kph;

static int hash_only_check;

/**
 * Reset mock data (for use before each test)
 */
static void ResetMocks(void)
{
	memset(&cparams, 0, sizeof(cparams));
	cparams.shared_data_size = sizeof(shared_data);
	cparams.shared_data_blob = shared_data;
	cparams.gbb_data = &gbb;
	cparams.gbb_size = sizeof(gbb);

	memset(&kparams, 0, sizeof(kparams));

	memset(&gbb, 0, sizeof(gbb));
	gbb.major_version = GBB_MAJOR_VER;
	gbb.minor_version = GBB_MINOR_VER;
	gbb.flags = 0;

	memset(&ctx, 0, sizeof(ctx));
	ctx.workbuf = workbuf;
	ctx.workbuf_size = sizeof(workbuf);
	vb2_init_context(&ctx);
	vb2_nv_init(&ctx);

	sd = vb2_get_sd(&ctx);
	sd->vbsd = shared;

	memset(&shared_data, 0, sizeof(shared_data));
	VbSharedDataInit(shared, sizeof(shared_data));

	key_block_verify_fail = 0;
	preamble_verify_fail = 0;
	verify_data_fail = 0;

	memset(&kbh, 0, sizeof(kbh));
	kbh.data_key.key_version = 2;
	kbh.key_block_flags = -1;
	kbh.key_block_size = sizeof(kbh);

	memset(&kph, 0, sizeof(kph));
	kph.kernel_version = 1;
	kph.preamble_size = 4096 - kbh.key_block_size;
	kph.body_signature.data_size = 70144;
	kph.bootloader_address = 0xbeadd008;
	kph.bootloader_size = 0x1234;

	memcpy(kernel_buffer, &kbh, sizeof(kbh));
	memcpy((kernel_buffer + kbh.key_block_size), &kph, sizeof(kph));

	hash_only_check = -1;
}

static void copy_kbh(void)
{
	memcpy(kernel_buffer, &kbh, sizeof(kbh));
}

/* Mocks */
int vb2_unpack_key_buffer(struct vb2_public_key *key,
		   const uint8_t *buf,
		   uint32_t size)
{
	if (--unpack_key_fail == 0)
		return VB2_ERROR_MOCK;

	return VB2_SUCCESS;
}

int vb2_verify_keyblock(struct vb2_keyblock *block,
			uint32_t size,
			const struct vb2_public_key *key,
			const struct vb2_workbuf *wb)
{
	hash_only_check = 0;

	if (key_block_verify_fail)
		return VB2_ERROR_MOCK;

	/* Use this as an opportunity to override the key block */
	memcpy((void *)block, &kbh, sizeof(kbh));
	return VB2_SUCCESS;
}

int vb2_verify_keyblock_hash(const struct vb2_keyblock *block,
			     uint32_t size,
			     const struct vb2_workbuf *wb)
{
	hash_only_check = 1;

	if (key_block_verify_fail)
		return VB2_ERROR_MOCK;

	/* Use this as an opportunity to override the key block */
	memcpy((void *)block, &kbh, sizeof(kbh));
	return VB2_SUCCESS;
}

int vb2_verify_kernel_preamble(struct vb2_kernel_preamble *preamble,
			       uint32_t size,
			       const struct vb2_public_key *key,
			       const struct vb2_workbuf *wb)
{
	if (preamble_verify_fail)
		return VB2_ERROR_MOCK;

	/* Use this as an opportunity to override the preamble */
	memcpy((void *)preamble, &kph, sizeof(kph));
	return VB2_SUCCESS;
}

int vb2_verify_data(const uint8_t *data,
		    uint32_t size,
		    struct vb2_signature *sig,
		    const struct vb2_public_key *key,
		    const struct vb2_workbuf *wb)
{
	if (verify_data_fail)
		return VB2_ERROR_MOCK;

	return VB2_SUCCESS;
}

VbError_t VbExNvStorageRead(uint8_t *buf)
{
	memcpy(buf, ctx.nvdata, sizeof(ctx.nvdata));
	return VBERROR_SUCCESS;
}

static void VerifyMemoryBootImageTest(void)
{
	uint32_t u;

	int kernel_body_offset;
	int kernel_body_size;
	uintptr_t kernel_body_start;
	size_t kernel_buffer_size = sizeof(kernel_buffer);

	ResetMocks();

	kernel_body_offset = kbh.key_block_size + kph.preamble_size;
	kernel_body_size = sizeof(kernel_buffer) - kernel_body_offset;
	kernel_body_start = (uintptr_t)kernel_buffer + kernel_body_offset;

	u = VbVerifyMemoryBootImage(&cparams, &kparams, kernel_buffer,
				    kernel_buffer_size);
	TEST_EQ(u, 0, "Image good");
	TEST_EQ(kparams.partition_number, 0, "  part num");
	TEST_EQ(kparams.bootloader_address, 0xbeadd008, "  bootloader addr");
	TEST_EQ(kparams.bootloader_size, 0x1234, "  bootloader size");
	TEST_PTR_EQ(kparams.kernel_buffer, (void *)(kernel_body_start),
		    "  kernel buffer");
	TEST_EQ(kparams.kernel_buffer_size, kernel_body_size,
		"  kernel buffer size");

	/* Empty image buffer. */
	ResetMocks();
	TEST_EQ(VbVerifyMemoryBootImage(&cparams, &kparams, NULL,
					kernel_buffer_size),
		VBERROR_INVALID_PARAMETER, "Empty image");

	/* Illegal image size. */
	ResetMocks();
	TEST_EQ(VbVerifyMemoryBootImage(&cparams, &kparams, kernel_buffer, 0),
		VBERROR_INVALID_PARAMETER, "Illegal image size");

	/* Key Block Verification Failure */
	ResetMocks();
	key_block_verify_fail = 1;
	TEST_EQ(VbVerifyMemoryBootImage(&cparams, &kparams, kernel_buffer,
					kernel_buffer_size),
		VBERROR_INVALID_KERNEL_FOUND, "Key verify failed");
	TEST_EQ(hash_only_check, 0, "  hash check");

	/* Key Block Hash Failure */
	ResetMocks();
	shared->flags = VBSD_BOOT_DEV_SWITCH_ON;
	gbb.flags = GBB_FLAG_FORCE_DEV_BOOT_FASTBOOT_FULL_CAP;
	key_block_verify_fail = 1;
	TEST_EQ(VbVerifyMemoryBootImage(&cparams, &kparams, kernel_buffer,
					kernel_buffer_size),
		VBERROR_INVALID_KERNEL_FOUND, "Key verify failed");
	TEST_EQ(hash_only_check, 1, "  hash check");

	/* Key Block Hash Failure -- VBNV */
	ResetMocks();
	shared->flags = VBSD_BOOT_DEV_SWITCH_ON;
	key_block_verify_fail = 1;
	vb2_nv_set(&ctx, VB2_NV_DEV_BOOT_FASTBOOT_FULL_CAP, 1);
	TEST_EQ(VbVerifyMemoryBootImage(&cparams, &kparams, kernel_buffer,
					kernel_buffer_size),
		VBERROR_INVALID_KERNEL_FOUND, "Key verify failed");
	TEST_EQ(hash_only_check, 1, "  hash check -- VBNV flag");

	/* Developer flag mismatch - dev switch on */
	ResetMocks();
	kbh.key_block_flags = KEY_BLOCK_FLAG_DEVELOPER_0 |
		KEY_BLOCK_FLAG_RECOVERY_1;
	copy_kbh();
	shared->flags = VBSD_BOOT_DEV_SWITCH_ON;
	TEST_EQ(VbVerifyMemoryBootImage(&cparams, &kparams, kernel_buffer,
					kernel_buffer_size),
		VBERROR_INVALID_KERNEL_FOUND,
		"Developer flag mismatch - dev switch on");

	/* Developer flag mismatch - dev switch on with GBB override */
	ResetMocks();
	kbh.key_block_flags = KEY_BLOCK_FLAG_DEVELOPER_0 |
		KEY_BLOCK_FLAG_RECOVERY_1;
	copy_kbh();
	gbb.flags = GBB_FLAG_FORCE_DEV_BOOT_FASTBOOT_FULL_CAP;
	shared->flags = VBSD_BOOT_DEV_SWITCH_ON;
	TEST_EQ(VbVerifyMemoryBootImage(&cparams, &kparams, kernel_buffer,
					kernel_buffer_size),
		VBERROR_SUCCESS,
		"Developer flag mismatch - dev switch on(gbb override)");

	/* Recovery flag mismatch - dev switch on with GBB override */
	ResetMocks();
	kbh.key_block_flags = KEY_BLOCK_FLAG_DEVELOPER_0 |
		KEY_BLOCK_FLAG_RECOVERY_0;
	copy_kbh();
	shared->flags = VBSD_BOOT_DEV_SWITCH_ON;
	gbb.flags = GBB_FLAG_FORCE_DEV_BOOT_FASTBOOT_FULL_CAP;
	TEST_EQ(VbVerifyMemoryBootImage(&cparams, &kparams, kernel_buffer,
					kernel_buffer_size),
		VBERROR_SUCCESS,
		"Recovery flag mismatch - dev switch on(gbb override)");

	/* Developer flag mismatch - dev switch off */
	ResetMocks();
	kbh.key_block_flags = KEY_BLOCK_FLAG_DEVELOPER_1 |
		KEY_BLOCK_FLAG_RECOVERY_1;
	copy_kbh();
	TEST_EQ(VbVerifyMemoryBootImage(&cparams, &kparams, kernel_buffer,
					kernel_buffer_size),
		VBERROR_INVALID_KERNEL_FOUND,
		"Developer flag mismatch - dev switch off");

	/* Recovery flag mismatch */
	ResetMocks();
	kbh.key_block_flags = KEY_BLOCK_FLAG_DEVELOPER_0 |
		KEY_BLOCK_FLAG_RECOVERY_0;
	shared->flags = 0;
	copy_kbh();
	TEST_EQ(VbVerifyMemoryBootImage(&cparams, &kparams, kernel_buffer,
					kernel_buffer_size),
		VBERROR_INVALID_KERNEL_FOUND, "Recovery flag mismatch");

	/* Preamble verification */
	ResetMocks();
	preamble_verify_fail = 1;
	TEST_EQ(VbVerifyMemoryBootImage(&cparams, &kparams, kernel_buffer,
					kernel_buffer_size),
		VBERROR_INVALID_KERNEL_FOUND, "Preamble verification");

	/* Data verification */
	ResetMocks();
	verify_data_fail = 1;
	TEST_EQ(VbVerifyMemoryBootImage(&cparams, &kparams, kernel_buffer,
					kernel_buffer_size),
		VBERROR_INVALID_KERNEL_FOUND, "Data verification");
}

int main(void)
{
	VerifyMemoryBootImageTest();

	return gTestSuccess ? 0 : 255;
}
