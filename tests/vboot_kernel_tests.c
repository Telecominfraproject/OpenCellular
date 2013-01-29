/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for vboot_kernel.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cgptlib.h"
#include "gbb_header.h"
#include "gpt.h"
#include "host_common.h"
#include "load_kernel_fw.h"
#include "test_common.h"
#include "vboot_api.h"
#include "vboot_common.h"
#include "vboot_kernel.h"
#include "vboot_nvstorage.h"

#define LOGCALL(fmt, args...) sprintf(call_log + strlen(call_log), fmt, ##args)
#define TEST_CALLS(expect_log) TEST_STR_EQ(call_log, expect_log, "  calls")

/* Mock data */
static char call_log[4096];
static int disk_call_to_fail;
static int disk_calls;
static int gpt_init_fail;

static GoogleBinaryBlockHeader gbb;
static VbExDiskHandle_t handle;
static VbNvContext vnc;
static uint8_t shared_data[VB_SHARED_DATA_MIN_SIZE];
static VbSharedDataHeader *shared = (VbSharedDataHeader *)shared_data;
static LoadKernelParams lkp;

static void ResetCallLog(void)
{
	*call_log = 0;
}

/**
 * Reset mock data (for use before each test)
 */
static void ResetMocks(void)
{
	ResetCallLog();

	disk_call_to_fail = 0;
	disk_calls = 0;

	gpt_init_fail = 0;

	memset(&gbb, 0, sizeof(gbb));
	gbb.major_version = GBB_MAJOR_VER;
	gbb.minor_version = GBB_MINOR_VER;
	gbb.flags = 0;

	memset(&vnc, 0, sizeof(vnc));
	VbNvSetup(&vnc);
	VbNvTeardown(&vnc);                   /* So CRC gets generated */

	memset(&shared_data, 0, sizeof(shared_data));
	VbSharedDataInit(shared, sizeof(shared_data));

	memset(&lkp, 0, sizeof(lkp));
	lkp.nv_context = &vnc;
	lkp.shared_data_blob = shared;
	lkp.gbb_data = &gbb;
	lkp.bytes_per_lba = 512;
	lkp.ending_lba = 1023;
}

/* Mocks */

VbError_t VbExDiskRead(VbExDiskHandle_t handle, uint64_t lba_start,
                       uint64_t lba_count, void *buffer)
{
	LOGCALL("VbExDiskRead(h, %d, %d)\n", (int)lba_start, (int)lba_count);

	if (++disk_calls == disk_call_to_fail)
		return VBERROR_SIMULATED;

	return VBERROR_SUCCESS;
}

VbError_t VbExDiskWrite(VbExDiskHandle_t handle, uint64_t lba_start,
			uint64_t lba_count, const void *buffer)
{
	LOGCALL("VbExDiskWrite(h, %d, %d)\n", (int)lba_start, (int)lba_count);

	if (++disk_calls == disk_call_to_fail)
		return VBERROR_SIMULATED;

	return VBERROR_SUCCESS;
}

int GptInit(GptData *gpt)
{
	return gpt_init_fail;
}

/**
 * Test reading/writing GPT
 */
static void ReadWriteGptTest(void)
{
	GptData g;
	GptHeader *h;

	g.sector_bytes = 512;
	g.drive_sectors = 1024;

	ResetMocks();
	TEST_EQ(AllocAndReadGptData(handle, &g), 0, "AllocAndRead");
	TEST_CALLS("VbExDiskRead(h, 1, 1)\n"
		   "VbExDiskRead(h, 2, 32)\n"
		   "VbExDiskRead(h, 991, 32)\n"
		   "VbExDiskRead(h, 1023, 1)\n");
	ResetCallLog();
	TEST_EQ(WriteAndFreeGptData(handle, &g), 0, "WriteAndFree");
	TEST_CALLS("");

	/* Data which is changed is written */
	ResetMocks();
	AllocAndReadGptData(handle, &g);
	g.modified |= GPT_MODIFIED_HEADER1 | GPT_MODIFIED_ENTRIES1;
	ResetCallLog();
	TEST_EQ(WriteAndFreeGptData(handle, &g), 0, "WriteAndFree mod 1");
	TEST_CALLS("VbExDiskWrite(h, 1, 1)\n"
		   "VbExDiskWrite(h, 2, 32)\n");

	/* Data which is changed is written */
	ResetMocks();
	AllocAndReadGptData(handle, &g);
	g.modified = -1;
	ResetCallLog();
	TEST_EQ(WriteAndFreeGptData(handle, &g), 0, "WriteAndFree mod all");
	TEST_CALLS("VbExDiskWrite(h, 1, 1)\n"
		   "VbExDiskWrite(h, 2, 32)\n"
		   "VbExDiskWrite(h, 991, 32)\n"
		   "VbExDiskWrite(h, 1023, 1)\n");

	/* If legacy signature, don't modify GPT header/entries 1 */
	ResetMocks();
	AllocAndReadGptData(handle, &g);
	h = (GptHeader *)g.primary_header;
	memcpy(h->signature, GPT_HEADER_SIGNATURE2, GPT_HEADER_SIGNATURE_SIZE);
	g.modified = -1;
	ResetCallLog();
	TEST_EQ(WriteAndFreeGptData(handle, &g), 0, "WriteAndFree mod all");
	TEST_CALLS("VbExDiskWrite(h, 991, 32)\n"
		   "VbExDiskWrite(h, 1023, 1)\n");

	/* Error reading */
	ResetMocks();
	disk_call_to_fail = 1;
	TEST_NEQ(AllocAndReadGptData(handle, &g), 0, "AllocAndRead disk fail");
	WriteAndFreeGptData(handle, &g);

	/* Error writing */
	ResetMocks();
	disk_call_to_fail = 5;
	AllocAndReadGptData(handle, &g);
	g.modified = -1;
	TEST_NEQ(WriteAndFreeGptData(handle, &g), 0, "WriteAndFree disk fail");
}

/**
 * Trivial invalid calls to LoadKernel()
 */
static void InvalidParamsTest(void)
{
	ResetMocks();
	lkp.bytes_per_lba = 0;
	TEST_EQ(LoadKernel(&lkp), VBERROR_INVALID_PARAMETER, "Bad lba size");

	ResetMocks();
	lkp.ending_lba = 0;
	TEST_EQ(LoadKernel(&lkp), VBERROR_INVALID_PARAMETER, "Bad lba count");

	ResetMocks();
	lkp.bytes_per_lba = 128*1024;
	TEST_EQ(LoadKernel(&lkp), VBERROR_INVALID_PARAMETER, "Huge lba size");

	ResetMocks();
	disk_call_to_fail = 1;
	TEST_EQ(LoadKernel(&lkp), VBERROR_NO_KERNEL_FOUND, "Can't read disk");

	ResetMocks();
	gpt_init_fail = 1;
	TEST_EQ(LoadKernel(&lkp), VBERROR_NO_KERNEL_FOUND, "Bad GPT");
}

int main(void)
{
	ReadWriteGptTest();
	InvalidParamsTest();

	return gTestSuccess ? 0 : 255;
}
