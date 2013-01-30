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

/* Mock kernel partition */
struct mock_part {
	uint32_t start;
	uint32_t size;
};

/* Partition list; ends with a 0-size partition. */
#define MOCK_PART_COUNT 8
static struct mock_part mock_parts[MOCK_PART_COUNT];
static int mock_part_next;

/* Mock data */
static char call_log[4096];
static uint8_t kernel_buffer[80000];
static int disk_read_to_fail;
static int disk_write_to_fail;
static int gpt_init_fail;
static int key_block_verify_fail;  /* 0=ok, 1=sig, 2=hash */
static int preamble_verify_fail;
static RSAPublicKey *mock_data_key;
static int mock_data_key_allocated;

static GoogleBinaryBlockHeader gbb;
static VbExDiskHandle_t handle;
static VbNvContext vnc;
static uint8_t shared_data[VB_SHARED_DATA_MIN_SIZE];
static VbSharedDataHeader *shared = (VbSharedDataHeader *)shared_data;
static LoadKernelParams lkp;
static VbKeyBlockHeader kbh;
static VbKernelPreambleHeader kph;

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

	disk_read_to_fail = -1;
	disk_write_to_fail = -1;

	gpt_init_fail = 0;
	key_block_verify_fail = 0;
	preamble_verify_fail = 0;

	mock_data_key = (RSAPublicKey *)"TestDataKey";
	mock_data_key_allocated = 0;

	memset(&gbb, 0, sizeof(gbb));
	gbb.major_version = GBB_MAJOR_VER;
	gbb.minor_version = GBB_MINOR_VER;
	gbb.flags = 0;

	memset(&vnc, 0, sizeof(vnc));
	VbNvSetup(&vnc);
	VbNvTeardown(&vnc);                   /* So CRC gets generated */

	memset(&shared_data, 0, sizeof(shared_data));
	VbSharedDataInit(shared, sizeof(shared_data));
	shared->kernel_version_tpm = 0x20001;

	memset(&lkp, 0, sizeof(lkp));
	lkp.nv_context = &vnc;
	lkp.shared_data_blob = shared;
	lkp.gbb_data = &gbb;
	lkp.bytes_per_lba = 512;
	lkp.ending_lba = 1023;
	lkp.kernel_buffer = kernel_buffer;
	lkp.kernel_buffer_size = sizeof(kernel_buffer);

	memset(&kbh, 0, sizeof(kbh));
	kbh.data_key.key_version = 2;
	kbh.key_block_flags = -1;
	kbh.key_block_size = sizeof(kbh);

	memset(&kph, 0, sizeof(kph));
	kph.kernel_version = 1;
	kph.preamble_size = 4096 - kbh.key_block_size;
	kph.body_signature.data_size = 70000;

	memset(mock_parts, 0, sizeof(mock_parts));
	mock_parts[0].start = 100;
	mock_parts[0].size = 150;  /* 75 KB */
	mock_part_next = 0;
}

/* Mocks */

VbError_t VbExDiskRead(VbExDiskHandle_t handle, uint64_t lba_start,
                       uint64_t lba_count, void *buffer)
{
	LOGCALL("VbExDiskRead(h, %d, %d)\n", (int)lba_start, (int)lba_count);

	if ((int)lba_start == disk_read_to_fail)
		return VBERROR_SIMULATED;

	return VBERROR_SUCCESS;
}

VbError_t VbExDiskWrite(VbExDiskHandle_t handle, uint64_t lba_start,
			uint64_t lba_count, const void *buffer)
{
	LOGCALL("VbExDiskWrite(h, %d, %d)\n", (int)lba_start, (int)lba_count);

	if ((int)lba_start == disk_write_to_fail)
		return VBERROR_SIMULATED;

	return VBERROR_SUCCESS;
}

int GptInit(GptData *gpt)
{
	return gpt_init_fail;
}

int GptNextKernelEntry(GptData *gpt, uint64_t *start_sector, uint64_t *size)
{
	struct mock_part *p = mock_parts + mock_part_next;

	if (!p->size)
		return GPT_ERROR_NO_VALID_KERNEL;

	gpt->current_kernel = mock_part_next;
	*start_sector = p->start;
	*size = p->size;
	mock_part_next++;
	return GPT_SUCCESS;
}

int KeyBlockVerify(const VbKeyBlockHeader *block, uint64_t size,
		   const VbPublicKey *key, int hash_only) {

	if (hash_only && key_block_verify_fail >= 2)
		return VBERROR_SIMULATED;
	else if (!hash_only && key_block_verify_fail >= 1)
		return VBERROR_SIMULATED;

	/* Use this as an opportunity to override the key block */
	memcpy((void *)block, &kbh, sizeof(kbh));
	return VBERROR_SUCCESS;
}

RSAPublicKey *PublicKeyToRSA(const VbPublicKey *key)
{
	TEST_EQ(mock_data_key_allocated, 0, "  mock data key not allocated");

	if (mock_data_key)
		mock_data_key_allocated++;

	return mock_data_key;
}

void RSAPublicKeyFree(RSAPublicKey* key)
{
	TEST_EQ(mock_data_key_allocated, 1, "  mock data key allocated");
	TEST_PTR_EQ(key, mock_data_key, "  data key ptr");
	mock_data_key_allocated--;
}

int VerifyKernelPreamble(const VbKernelPreambleHeader *preamble,
			 uint64_t size, const RSAPublicKey *key)
{
	if (preamble_verify_fail)
		return VBERROR_SIMULATED;

	/* Use this as an opportunity to override the preamble */
	memcpy((void *)preamble, &kph, sizeof(kph));
	return VBERROR_SUCCESS;
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
	disk_read_to_fail = 1;
	TEST_NEQ(AllocAndReadGptData(handle, &g), 0, "AllocAndRead disk fail");
	WriteAndFreeGptData(handle, &g);

	/* Error writing */
	ResetMocks();
	disk_write_to_fail = 1;
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
	disk_read_to_fail = 1;
	TEST_EQ(LoadKernel(&lkp), VBERROR_NO_KERNEL_FOUND, "Can't read disk");

	ResetMocks();
	gpt_init_fail = 1;
	TEST_EQ(LoadKernel(&lkp), VBERROR_NO_KERNEL_FOUND, "Bad GPT");
}

static void KernelLoopTest(void)
{
	/* Fail if no kernels found */
	ResetMocks();
	mock_parts[0].size = 0;
	TEST_EQ(LoadKernel(&lkp), VBERROR_NO_KERNEL_FOUND, "No kernels");

	/* Skip kernels which are too small */
	ResetMocks();
	mock_parts[0].size = 10;
	TEST_EQ(LoadKernel(&lkp), VBERROR_INVALID_KERNEL_FOUND, "Too small");

	ResetMocks();
	disk_read_to_fail = 100;
	TEST_EQ(LoadKernel(&lkp), VBERROR_INVALID_KERNEL_FOUND,
		"Fail reading kernel start");

	ResetMocks();
	key_block_verify_fail = 1;
	TEST_EQ(LoadKernel(&lkp), VBERROR_INVALID_KERNEL_FOUND,
		"Fail key block sig");

	/* In dev mode, fail if hash is bad too */
	ResetMocks();
	lkp.boot_flags |= BOOT_FLAG_DEVELOPER;
	key_block_verify_fail = 2;
	TEST_EQ(LoadKernel(&lkp), VBERROR_INVALID_KERNEL_FOUND,
		"Fail key block dev hash");

	/* In dev mode and requiring signed kernel, fail if sig is bad */
	ResetMocks();
	lkp.boot_flags |= BOOT_FLAG_DEVELOPER;
	VbNvSet(&vnc, VBNV_DEV_BOOT_SIGNED_ONLY, 1);
	VbNvTeardown(&vnc);
	key_block_verify_fail = 1;
	TEST_EQ(LoadKernel(&lkp), VBERROR_INVALID_KERNEL_FOUND,
		"Fail key block dev sig");

	/* Check key block flag mismatches */
	ResetMocks();
	kbh.key_block_flags =
		KEY_BLOCK_FLAG_RECOVERY_0 | KEY_BLOCK_FLAG_DEVELOPER_1;
	TEST_EQ(LoadKernel(&lkp), VBERROR_INVALID_KERNEL_FOUND,
		"Key block dev flag mismatch");

	ResetMocks();
	kbh.key_block_flags =
		KEY_BLOCK_FLAG_RECOVERY_1 | KEY_BLOCK_FLAG_DEVELOPER_0;
	TEST_EQ(LoadKernel(&lkp), VBERROR_INVALID_KERNEL_FOUND,
		"Key block rec flag mismatch");

	ResetMocks();
	lkp.boot_flags |= BOOT_FLAG_RECOVERY;
	kbh.key_block_flags =
		KEY_BLOCK_FLAG_RECOVERY_1 | KEY_BLOCK_FLAG_DEVELOPER_1;
	TEST_EQ(LoadKernel(&lkp), VBERROR_INVALID_KERNEL_FOUND,
		"Key block recdev flag mismatch");

	ResetMocks();
	lkp.boot_flags |= BOOT_FLAG_RECOVERY | BOOT_FLAG_DEVELOPER;
	kbh.key_block_flags =
		KEY_BLOCK_FLAG_RECOVERY_1 | KEY_BLOCK_FLAG_DEVELOPER_0;
	TEST_EQ(LoadKernel(&lkp), VBERROR_INVALID_KERNEL_FOUND,
		"Key block rec!dev flag mismatch");

	ResetMocks();
	kbh.data_key.key_version = 1;
	TEST_EQ(LoadKernel(&lkp), VBERROR_INVALID_KERNEL_FOUND,
		"Key block kernel key rollback");

	ResetMocks();
	kbh.data_key.key_version = 0x10000;
	TEST_EQ(LoadKernel(&lkp), VBERROR_INVALID_KERNEL_FOUND,
		"Key block kernel key version too big");

	/* TODO: key version ignored in recovery mode */
	/* TODO: key block validity ignored in dev mode */

	ResetMocks();
	mock_data_key = NULL;
	TEST_EQ(LoadKernel(&lkp), VBERROR_INVALID_KERNEL_FOUND, "Bad data key");

	ResetMocks();
	preamble_verify_fail = 1;
	TEST_EQ(LoadKernel(&lkp), VBERROR_INVALID_KERNEL_FOUND,	"Bad preamble");

	ResetMocks();
	kph.kernel_version = 0;
	TEST_EQ(LoadKernel(&lkp), VBERROR_INVALID_KERNEL_FOUND,
		"Kernel version rollback");

	/* TODO: kernel version ignored in recovery and dev modes */

	ResetMocks();
	kph.preamble_size |= 0x07;
	TEST_EQ(LoadKernel(&lkp), VBERROR_INVALID_KERNEL_FOUND,
		"Kernel body offset");

	ResetMocks();
	lkp.kernel_buffer_size = 8192;
	TEST_EQ(LoadKernel(&lkp), VBERROR_INVALID_KERNEL_FOUND,
		"Kernel too big for buffer");

	ResetMocks();
	mock_parts[0].size = 130;
	TEST_EQ(LoadKernel(&lkp), VBERROR_INVALID_KERNEL_FOUND,
		"Kernel too big for partition");

}

int main(void)
{
	ReadWriteGptTest();
	InvalidParamsTest();
	KernelLoopTest();

	return gTestSuccess ? 0 : 255;
}
