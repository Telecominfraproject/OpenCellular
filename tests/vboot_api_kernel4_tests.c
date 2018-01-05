/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for vboot_api_kernel, part 4 - select and load kernel
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "2sysincludes.h"
#include "2api.h"
#include "2misc.h"
#include "2nvstorage.h"
#include "ec_sync.h"
#include "gbb_header.h"
#include "host_common.h"
#include "load_kernel_fw.h"
#include "rollback_index.h"
#include "test_common.h"
#include "vboot_audio.h"
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

static uint32_t rkr_version;
static uint32_t new_version;
static struct RollbackSpaceFwmp rfr_fwmp;
static int rkr_retval, rkw_retval, rkl_retval, rfr_retval;
static VbError_t vbboot_retval;

/* Reset mock data (for use before each test) */
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
	vb2_nv_set(&ctx, VB2_NV_KERNEL_MAX_ROLLFORWARD, 0xffffffff);

	sd = vb2_get_sd(&ctx);
	sd->vbsd = shared;

	memset(&shared_data, 0, sizeof(shared_data));
	VbSharedDataInit(shared, sizeof(shared_data));

	memset(&rfr_fwmp, 0, sizeof(rfr_fwmp));
	rfr_retval = TPM_SUCCESS;

	rkr_version = new_version = 0x10002;
	rkr_retval = rkw_retval = rkl_retval = VBERROR_SUCCESS;
	vbboot_retval = VBERROR_SUCCESS;
}

/* Mock functions */

VbError_t VbExNvStorageRead(uint8_t *buf)
{
	memcpy(buf, ctx.nvdata, sizeof(ctx.nvdata));
	return VBERROR_SUCCESS;
}

VbError_t VbExNvStorageWrite(const uint8_t *buf)
{
	memcpy(ctx.nvdata, buf, sizeof(ctx.nvdata));
	return VBERROR_SUCCESS;
}

uint32_t RollbackKernelRead(uint32_t *version)
{
	*version = rkr_version;
	return rkr_retval;
}

uint32_t RollbackKernelWrite(uint32_t version)
{
	rkr_version = version;
	return rkw_retval;
}

uint32_t RollbackKernelLock(int recovery_mode)
{
	return rkl_retval;
}

uint32_t RollbackFwmpRead(struct RollbackSpaceFwmp *fwmp)
{
	memcpy(fwmp, &rfr_fwmp, sizeof(*fwmp));
	return rfr_retval;
}

uint32_t VbTryLoadKernel(struct vb2_context *ctx, uint32_t get_info_flags)
{
	shared->kernel_version_tpm = new_version;

	if (vbboot_retval == -1)
		return VBERROR_SIMULATED;

	return vbboot_retval;
}

VbError_t VbBootDeveloper(struct vb2_context *ctx)
{
	shared->kernel_version_tpm = new_version;

	if (vbboot_retval == -2)
		return VBERROR_SIMULATED;

	return vbboot_retval;
}

VbError_t VbBootRecovery(struct vb2_context *ctx)
{
	shared->kernel_version_tpm = new_version;

	if (vbboot_retval == -3)
		return VBERROR_SIMULATED;

	return vbboot_retval;
}

static void test_slk(VbError_t retval, int recovery_reason, const char *desc)
{
	TEST_EQ(VbSelectAndLoadKernel(&cparams, &kparams), retval, desc);
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST),
		recovery_reason, "  recovery reason");
}

/* Tests */

static void VbSlkTest(void)
{
	ResetMocks();
	test_slk(0, 0, "Normal");
	TEST_EQ(rkr_version, 0x10002, "  version");

	/*
	 * If shared->flags doesn't ask for software sync, we won't notice
	 * that error.
	 */
	ResetMocks();
	test_slk(0, 0, "EC sync not done");

	/* Same if shared->flags asks for sync, but it's overridden by GBB */
	ResetMocks();
	shared->flags |= VBSD_EC_SOFTWARE_SYNC;
	gbb.flags |= GBB_FLAG_DISABLE_EC_SOFTWARE_SYNC;
	test_slk(0, 0, "EC sync disabled by GBB");

	/* Rollback kernel version */
	ResetMocks();
	rkr_retval = 123;
	test_slk(VBERROR_TPM_READ_KERNEL,
		 VB2_RECOVERY_RW_TPM_R_ERROR, "Read kernel rollback");

	ResetMocks();
	new_version = 0x20003;
	test_slk(0, 0, "Roll forward");
	TEST_EQ(rkr_version, 0x20003, "  version");

	ResetMocks();
	new_version = 0x20003;
	shared->flags |= VBSD_FWB_TRIED;
	shared->firmware_index = 1;
	test_slk(0, 0, "Don't roll forward during try B");
	TEST_EQ(rkr_version, 0x10002, "  version");

	ResetMocks();
	vb2_nv_set(&ctx, VB2_NV_KERNEL_MAX_ROLLFORWARD, 0x30005);
	new_version = 0x40006;
	test_slk(0, 0, "Limit max roll forward");
	TEST_EQ(rkr_version, 0x30005, "  version");

	ResetMocks();
	vb2_nv_set(&ctx, VB2_NV_KERNEL_MAX_ROLLFORWARD, 0x10001);
	new_version = 0x40006;
	test_slk(0, 0, "Max roll forward can't rollback");
	TEST_EQ(rkr_version, 0x10002, "  version");

	ResetMocks();
	vbboot_retval = VBERROR_INVALID_KERNEL_FOUND;
	vb2_nv_set(&ctx, VB2_NV_RECOVERY_REQUEST, 123);
	shared->flags |= VBSD_FWB_TRIED;
	shared->firmware_index = 1;
	test_slk(VBERROR_INVALID_KERNEL_FOUND,
		 0, "Don't go to recovery if try b fails to find a kernel");

	ResetMocks();
	new_version = 0x20003;
	rkw_retval = 123;
	test_slk(VBERROR_TPM_WRITE_KERNEL,
		 VB2_RECOVERY_RW_TPM_W_ERROR, "Write kernel rollback");

	ResetMocks();
	rkl_retval = 123;
	test_slk(VBERROR_TPM_LOCK_KERNEL,
		 VB2_RECOVERY_RW_TPM_L_ERROR, "Lock kernel rollback");

	/* Boot normal */
	ResetMocks();
	vbboot_retval = -1;
	test_slk(VBERROR_SIMULATED, 0, "Normal boot bad");

	/* Boot dev */
	ResetMocks();
	shared->flags |= VBSD_BOOT_DEV_SWITCH_ON;
	vbboot_retval = -2;
	test_slk(VBERROR_SIMULATED, 0, "Dev boot bad");

	ResetMocks();
	shared->flags |= VBSD_BOOT_DEV_SWITCH_ON;
	new_version = 0x20003;
	test_slk(0, 0, "Dev doesn't roll forward");
	TEST_EQ(rkr_version, 0x10002, "  version");

	/* Boot recovery */
	ResetMocks();
	shared->recovery_reason = 123;
	vbboot_retval = -3;
	test_slk(VBERROR_SIMULATED, 0, "Recovery boot bad");

	ResetMocks();
	shared->recovery_reason = 123;
	new_version = 0x20003;
	test_slk(0, 0, "Recovery doesn't roll forward");
	TEST_EQ(rkr_version, 0x10002, "  version");

	ResetMocks();
	shared->recovery_reason = 123;
	rkr_retval = rkw_retval = rkl_retval = VBERROR_SIMULATED;
	test_slk(0, 0, "Recovery ignore TPM errors");

	ResetMocks();
	shared->recovery_reason = VB2_RECOVERY_TRAIN_AND_REBOOT;
	test_slk(VBERROR_REBOOT_REQUIRED, 0, "Recovery train and reboot");

	// todo: rkr/w/l fail ignored if recovery


}

int main(void)
{
	VbSlkTest();

	return gTestSuccess ? 0 : 255;
}
