/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for vboot_kernel.c
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
#include "2sha.h"
#include "cgptlib.h"
#include "cgptlib_internal.h"
#include "crc32.h"
#include "gbb_header.h"
#include "gpt.h"
#include "host_common.h"
#include "load_kernel_fw.h"
#include "rollback_index.h"
#include "test_common.h"
#include "vb2_struct.h"
#include "vboot_api.h"
#include "vboot_common.h"
#include "vboot_kernel.h"

#define LOGCALL(fmt, args...) sprintf(call_log + strlen(call_log), fmt, ##args)
#define TEST_CALLS(expect_log) TEST_STR_EQ(call_log, expect_log, "  calls")

#define MOCK_SECTOR_SIZE  512
#define MOCK_SECTOR_COUNT 1024

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
static int verify_data_fail;
static int unpack_key_fail;
static int gpt_flag_external;

static uint8_t gbb_data[sizeof(GoogleBinaryBlockHeader) + 2048];
static GoogleBinaryBlockHeader *gbb = (GoogleBinaryBlockHeader*)gbb_data;
static VbExDiskHandle_t handle;
static uint8_t shared_data[VB_SHARED_DATA_MIN_SIZE];
static VbSharedDataHeader *shared = (VbSharedDataHeader *)shared_data;
static LoadKernelParams lkp;
static VbKeyBlockHeader kbh;
static VbKernelPreambleHeader kph;
static struct RollbackSpaceFwmp fwmp;
static uint8_t mock_disk[MOCK_SECTOR_SIZE * MOCK_SECTOR_COUNT];
static GptHeader *mock_gpt_primary =
	(GptHeader*)&mock_disk[MOCK_SECTOR_SIZE * 1];
static GptHeader *mock_gpt_secondary =
	(GptHeader*)&mock_disk[MOCK_SECTOR_SIZE * (MOCK_SECTOR_COUNT - 1)];
static uint8_t mock_digest[VB2_SHA256_DIGEST_SIZE] = {12, 34, 56, 78};
static uint8_t workbuf[VB2_KERNEL_WORKBUF_RECOMMENDED_SIZE];
static struct vb2_context ctx;

/**
 * Prepare a valid GPT header that will pass CheckHeader() tests
 */
static void SetupGptHeader(GptHeader *h, int is_secondary)
{
	memset(h, '\0', MOCK_SECTOR_SIZE);

	/* "EFI PART" */
	memcpy(h->signature, GPT_HEADER_SIGNATURE, GPT_HEADER_SIGNATURE_SIZE);
	h->revision = GPT_HEADER_REVISION;
	h->size = MIN_SIZE_OF_HEADER;

	/* 16KB: 128 entries of 128 bytes */
	h->size_of_entry = sizeof(GptEntry);
	h->number_of_entries = MAX_NUMBER_OF_ENTRIES;

	/* Set LBA pointers for primary or secondary header */
	if (is_secondary) {
		h->my_lba = MOCK_SECTOR_COUNT - GPT_HEADER_SECTORS;
		h->entries_lba = h->my_lba - CalculateEntriesSectors(h,
							MOCK_SECTOR_SIZE);
	} else {
		h->my_lba = GPT_PMBR_SECTORS;
		h->entries_lba = h->my_lba + 1;
	}

	h->first_usable_lba = 2 + CalculateEntriesSectors(h, MOCK_SECTOR_SIZE);
	h->last_usable_lba = MOCK_SECTOR_COUNT - 2 - CalculateEntriesSectors(h,
								MOCK_SECTOR_SIZE);

	h->header_crc32 = HeaderCrc(h);
}

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

	memset(&mock_disk, 0, sizeof(mock_disk));
	SetupGptHeader(mock_gpt_primary, 0);
	SetupGptHeader(mock_gpt_secondary, 1);

	disk_read_to_fail = -1;
	disk_write_to_fail = -1;

	gpt_init_fail = 0;
	key_block_verify_fail = 0;
	preamble_verify_fail = 0;
	verify_data_fail = 0;
	unpack_key_fail = 0;

	gpt_flag_external = 0;

	memset(gbb, 0, sizeof(*gbb));
	gbb->major_version = GBB_MAJOR_VER;
	gbb->minor_version = GBB_MINOR_VER;
	gbb->flags = 0;

	memset(&shared_data, 0, sizeof(shared_data));
	VbSharedDataInit(shared, sizeof(shared_data));
	shared->kernel_version_tpm = 0x20001;

	memset(&lkp, 0, sizeof(lkp));
	lkp.bytes_per_lba = 512;
	lkp.streaming_lba_count = 1024;
	lkp.gpt_lba_count = 1024;
	lkp.kernel_buffer = kernel_buffer;
	lkp.kernel_buffer_size = sizeof(kernel_buffer);
	lkp.disk_handle = (VbExDiskHandle_t)1;

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

	memset(&fwmp, 0, sizeof(fwmp));
	memcpy(fwmp.dev_key_hash, mock_digest, sizeof(fwmp.dev_key_hash));

	memset(mock_parts, 0, sizeof(mock_parts));
	mock_parts[0].start = 100;
	mock_parts[0].size = 150;  /* 75 KB */
	mock_part_next = 0;

	memset(&ctx, 0, sizeof(ctx));
	ctx.workbuf = workbuf;
	ctx.workbuf_size = sizeof(workbuf);
	vb2_nv_init(&ctx);

	struct vb2_shared_data *sd = vb2_get_sd(&ctx);
	sd->vbsd = shared;
	sd->gbb = (struct vb2_gbb_header *)gbb_data;
	sd->gbb_size = sizeof(gbb_data);

	// TODO: more workbuf fields - flags, secdata, secdatak
}

/* Mocks */

VbError_t VbExDiskRead(VbExDiskHandle_t handle, uint64_t lba_start,
                       uint64_t lba_count, void *buffer)
{
	LOGCALL("VbExDiskRead(h, %d, %d)\n", (int)lba_start, (int)lba_count);

	if ((int)lba_start == disk_read_to_fail)
		return VBERROR_SIMULATED;

	memcpy(buffer, &mock_disk[lba_start * MOCK_SECTOR_SIZE],
	       lba_count * MOCK_SECTOR_SIZE);

	return VBERROR_SUCCESS;
}

VbError_t VbExDiskWrite(VbExDiskHandle_t handle, uint64_t lba_start,
			uint64_t lba_count, const void *buffer)
{
	LOGCALL("VbExDiskWrite(h, %d, %d)\n", (int)lba_start, (int)lba_count);

	if ((int)lba_start == disk_write_to_fail)
		return VBERROR_SIMULATED;

	memcpy(&mock_disk[lba_start * MOCK_SECTOR_SIZE], buffer,
	       lba_count * MOCK_SECTOR_SIZE);

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

	if (gpt->flags & GPT_FLAG_EXTERNAL)
		gpt_flag_external++;

	gpt->current_kernel = mock_part_next;
	*start_sector = p->start;
	*size = p->size;
	mock_part_next++;
	return GPT_SUCCESS;
}

void GetCurrentKernelUniqueGuid(GptData *gpt, void *dest)
{
	static char fake_guid[] = "FakeGuid";

	memcpy(dest, fake_guid, sizeof(fake_guid));
}

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
	if (key_block_verify_fail >= 1)
		return VB2_ERROR_MOCK;

	/* Use this as an opportunity to override the key block */
	memcpy((void *)block, &kbh, sizeof(kbh));
	return VB2_SUCCESS;
}

int vb2_verify_keyblock_hash(const struct vb2_keyblock *block,
			     uint32_t size,
			     const struct vb2_workbuf *wb)
{
	if (key_block_verify_fail >= 2)
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

int vb2_digest_buffer(const uint8_t *buf,
		      uint32_t size,
		      enum vb2_hash_algorithm hash_alg,
		      uint8_t *digest,
		      uint32_t digest_size)
{
	memcpy(digest, mock_digest, sizeof(mock_digest));
	return VB2_SUCCESS;
}

/**
 * Test reading/writing GPT
 */
static void ReadWriteGptTest(void)
{
	GptData g;
	GptHeader *h;

	g.sector_bytes = MOCK_SECTOR_SIZE;
	g.streaming_drive_sectors = g.gpt_drive_sectors = MOCK_SECTOR_COUNT;
	g.valid_headers = g.valid_entries = MASK_BOTH;

	ResetMocks();
	TEST_EQ(AllocAndReadGptData(handle, &g), 0, "AllocAndRead");
	TEST_CALLS("VbExDiskRead(h, 1, 1)\n"
		   "VbExDiskRead(h, 2, 32)\n"
		   "VbExDiskRead(h, 1023, 1)\n"
		   "VbExDiskRead(h, 991, 32)\n");
	ResetCallLog();
	/*
	 * Valgrind complains about access to uninitialized memory here, so
	 * zero the primary header before each test.
	 */
	memset(g.primary_header, '\0', g.sector_bytes);
	TEST_EQ(WriteAndFreeGptData(handle, &g), 0, "WriteAndFree");
	TEST_CALLS("");

	/*
	 * Invalidate primary GPT header,
	 * check that AllocAndReadGptData still succeeds
	 */
	ResetMocks();
	memset(mock_gpt_primary, '\0', sizeof(*mock_gpt_primary));
	TEST_EQ(AllocAndReadGptData(handle, &g), 0,
		"AllocAndRead primary invalid");
	TEST_EQ(CheckHeader(mock_gpt_primary, 0, g.streaming_drive_sectors,
                g.gpt_drive_sectors, 0, g.sector_bytes),
                1, "Primary header is invalid");
	TEST_EQ(CheckHeader(mock_gpt_secondary, 1, g.streaming_drive_sectors,
		g.gpt_drive_sectors, 0, g.sector_bytes),
                0, "Secondary header is valid");
	TEST_CALLS("VbExDiskRead(h, 1, 1)\n"
		   "VbExDiskRead(h, 1023, 1)\n"
		   "VbExDiskRead(h, 991, 32)\n");
	WriteAndFreeGptData(handle, &g);

	/*
	 * Invalidate secondary GPT header,
	 * check that AllocAndReadGptData still succeeds
	 */
	ResetMocks();
	memset(mock_gpt_secondary, '\0', sizeof(*mock_gpt_secondary));
	TEST_EQ(AllocAndReadGptData(handle, &g), 0,
		"AllocAndRead secondary invalid");
	TEST_EQ(CheckHeader(mock_gpt_primary, 0, g.streaming_drive_sectors,
		g.gpt_drive_sectors, 0, g.sector_bytes),
                0, "Primary header is valid");
	TEST_EQ(CheckHeader(mock_gpt_secondary, 1, g.streaming_drive_sectors,
		g.gpt_drive_sectors, 0, g.sector_bytes),
                1, "Secondary header is invalid");
	TEST_CALLS("VbExDiskRead(h, 1, 1)\n"
		   "VbExDiskRead(h, 2, 32)\n"
		   "VbExDiskRead(h, 1023, 1)\n");
	WriteAndFreeGptData(handle, &g);

	/*
	 * Invalidate primary AND secondary GPT header,
	 * check that AllocAndReadGptData fails.
	 */
	ResetMocks();
	memset(mock_gpt_primary, '\0', sizeof(*mock_gpt_primary));
	memset(mock_gpt_secondary, '\0', sizeof(*mock_gpt_secondary));
	TEST_EQ(AllocAndReadGptData(handle, &g), 1,
		"AllocAndRead primary and secondary invalid");
	TEST_EQ(CheckHeader(mock_gpt_primary, 0, g.streaming_drive_sectors,
		g.gpt_drive_sectors, 0, g.sector_bytes),
                1, "Primary header is invalid");
	TEST_EQ(CheckHeader(mock_gpt_secondary, 1, g.streaming_drive_sectors,
		g.gpt_drive_sectors, 0, g.sector_bytes),
                1, "Secondary header is invalid");
	TEST_CALLS("VbExDiskRead(h, 1, 1)\n"
		   "VbExDiskRead(h, 1023, 1)\n");
	WriteAndFreeGptData(handle, &g);

	/*
	 * Invalidate primary GPT header and check that it is
	 * repaired by GptRepair().
	 *
	 * This would normally be called by LoadKernel()->GptInit()
	 * but this callback is mocked in these tests.
	 */
	ResetMocks();
	memset(mock_gpt_primary, '\0', sizeof(*mock_gpt_primary));
	TEST_EQ(AllocAndReadGptData(handle, &g), 0,
		"Fix Primary GPT: AllocAndRead");
	/* Call GptRepair() with input indicating secondary GPT is valid */
	g.valid_headers = g.valid_entries = MASK_SECONDARY;
	GptRepair(&g);
	TEST_EQ(WriteAndFreeGptData(handle, &g), 0,
		"Fix Primary GPT: WriteAndFreeGptData");
	TEST_CALLS("VbExDiskRead(h, 1, 1)\n"
		   "VbExDiskRead(h, 1023, 1)\n"
		   "VbExDiskRead(h, 991, 32)\n"
		   "VbExDiskWrite(h, 1, 1)\n"
		   "VbExDiskWrite(h, 2, 32)\n");
	TEST_EQ(CheckHeader(mock_gpt_primary, 0, g.streaming_drive_sectors,
		g.gpt_drive_sectors, 0, g.sector_bytes),
                0, "Fix Primary GPT: Primary header is valid");

	/*
	 * Invalidate secondary GPT header and check that it can be
	 * repaired by GptRepair().
	 *
	 * This would normally be called by LoadKernel()->GptInit()
	 * but this callback is mocked in these tests.
	 */
	ResetMocks();
	memset(mock_gpt_secondary, '\0', sizeof(*mock_gpt_secondary));
	TEST_EQ(AllocAndReadGptData(handle, &g), 0,
		"Fix Secondary GPT: AllocAndRead");
	/* Call GptRepair() with input indicating primary GPT is valid */
	g.valid_headers = g.valid_entries = MASK_PRIMARY;
	GptRepair(&g);
	TEST_EQ(WriteAndFreeGptData(handle, &g), 0,
		"Fix Secondary GPT: WriteAndFreeGptData");
	TEST_CALLS("VbExDiskRead(h, 1, 1)\n"
		   "VbExDiskRead(h, 2, 32)\n"
		   "VbExDiskRead(h, 1023, 1)\n"
		   "VbExDiskWrite(h, 1023, 1)\n"
		   "VbExDiskWrite(h, 991, 32)\n");
	TEST_EQ(CheckHeader(mock_gpt_secondary, 1, g.streaming_drive_sectors,
		g.gpt_drive_sectors, 0, g.sector_bytes),
                0, "Fix Secondary GPT: Secondary header is valid");

	/* Data which is changed is written */
	ResetMocks();
	AllocAndReadGptData(handle, &g);
	g.modified |= GPT_MODIFIED_HEADER1 | GPT_MODIFIED_ENTRIES1;
	ResetCallLog();
	memset(g.primary_header, '\0', g.sector_bytes);
	h = (GptHeader*)g.primary_header;
	h->entries_lba = 2;
	h->number_of_entries = MAX_NUMBER_OF_ENTRIES;
	h->size_of_entry = sizeof(GptEntry);
	TEST_EQ(WriteAndFreeGptData(handle, &g), 0, "WriteAndFree mod 1");
	TEST_CALLS("VbExDiskWrite(h, 1, 1)\n"
		   "VbExDiskWrite(h, 2, 32)\n");

	/* Data which is changed is written */
	ResetMocks();
	AllocAndReadGptData(handle, &g);
	g.modified = -1;
	ResetCallLog();
	memset(g.primary_header, '\0', g.sector_bytes);
	h = (GptHeader*)g.primary_header;
	h->entries_lba = 2;
	h->number_of_entries = MAX_NUMBER_OF_ENTRIES;
	h->size_of_entry = sizeof(GptEntry);
	h = (GptHeader*)g.secondary_header;
	h->entries_lba = 991;
	TEST_EQ(WriteAndFreeGptData(handle, &g), 0, "WriteAndFree mod all");
	TEST_CALLS("VbExDiskWrite(h, 1, 1)\n"
		   "VbExDiskWrite(h, 2, 32)\n"
		   "VbExDiskWrite(h, 1023, 1)\n"
		   "VbExDiskWrite(h, 991, 32)\n");

	/* If legacy signature, don't modify GPT header/entries 1 */
	ResetMocks();
	AllocAndReadGptData(handle, &g);
	h = (GptHeader *)g.primary_header;
	memcpy(h->signature, GPT_HEADER_SIGNATURE2, GPT_HEADER_SIGNATURE_SIZE);
	g.modified = -1;
	ResetCallLog();
	TEST_EQ(WriteAndFreeGptData(handle, &g), 0, "WriteAndFree mod all");
	TEST_CALLS("VbExDiskWrite(h, 1023, 1)\n"
		   "VbExDiskWrite(h, 991, 32)\n");

	/* Error reading */
	ResetMocks();
	disk_read_to_fail = 1;
	TEST_EQ(AllocAndReadGptData(handle, &g), 0, "AllocAndRead disk fail");
	g.valid_headers = g.valid_entries = MASK_SECONDARY;
	GptRepair(&g);
	ResetCallLog();
	TEST_EQ(WriteAndFreeGptData(handle, &g), 0, "WriteAndFree mod 1");
	TEST_CALLS("VbExDiskWrite(h, 1, 1)\n"
		   "VbExDiskWrite(h, 2, 32)\n");

	ResetMocks();
	disk_read_to_fail = 2;
	TEST_EQ(AllocAndReadGptData(handle, &g), 0, "AllocAndRead disk fail");
	g.valid_headers = MASK_BOTH;
	g.valid_entries = MASK_SECONDARY;
	GptRepair(&g);
	ResetCallLog();
	TEST_EQ(WriteAndFreeGptData(handle, &g), 0, "WriteAndFree mod 1");
	TEST_CALLS("VbExDiskWrite(h, 2, 32)\n");

	ResetMocks();
	disk_read_to_fail = 991;
	TEST_EQ(AllocAndReadGptData(handle, &g), 0, "AllocAndRead disk fail");
	g.valid_headers = MASK_BOTH;
	g.valid_entries = MASK_PRIMARY;
	GptRepair(&g);
	ResetCallLog();
	TEST_EQ(WriteAndFreeGptData(handle, &g), 0, "WriteAndFree mod 2");
	TEST_CALLS("VbExDiskWrite(h, 991, 32)\n");

	ResetMocks();
	disk_read_to_fail = 1023;
	TEST_EQ(AllocAndReadGptData(handle, &g), 0, "AllocAndRead disk fail");
	g.valid_headers = g.valid_entries = MASK_PRIMARY;
	GptRepair(&g);
	ResetCallLog();
	TEST_EQ(WriteAndFreeGptData(handle, &g), 0, "WriteAndFree mod 2");
	TEST_CALLS("VbExDiskWrite(h, 1023, 1)\n"
		   "VbExDiskWrite(h, 991, 32)\n");

	/* Error writing */
	ResetMocks();
	disk_write_to_fail = 1;
	AllocAndReadGptData(handle, &g);
	g.modified = -1;
	memset(g.primary_header, '\0', g.sector_bytes);
	TEST_NEQ(WriteAndFreeGptData(handle, &g), 0, "WriteAndFree disk fail");

	ResetMocks();
	disk_write_to_fail = 2;
	AllocAndReadGptData(handle, &g);
	g.modified = -1;
	memset(g.primary_header, '\0', g.sector_bytes);
	h = (GptHeader*)g.primary_header;
	h->entries_lba = 2;
	TEST_NEQ(WriteAndFreeGptData(handle, &g), 0, "WriteAndFree disk fail");

	ResetMocks();
	disk_write_to_fail = 991;
	AllocAndReadGptData(handle, &g);
	g.modified = -1;
	memset(g.primary_header, '\0', g.sector_bytes);
	TEST_NEQ(WriteAndFreeGptData(handle, &g), 0, "WriteAndFree disk fail");

	ResetMocks();
	disk_write_to_fail = 1023;
	AllocAndReadGptData(handle, &g);
	g.modified = -1;
	memset(g.primary_header, '\0', g.sector_bytes);
	TEST_NEQ(WriteAndFreeGptData(handle, &g), 0, "WriteAndFree disk fail");

}

static void TestLoadKernel(int expect_retval, char *test_name)
{
	TEST_EQ(LoadKernel(&ctx, &lkp), expect_retval, test_name);
}

/**
 * Trivial invalid calls to LoadKernel()
 */
static void InvalidParamsTest(void)
{
	ResetMocks();
	gpt_init_fail = 1;
	TestLoadKernel(VBERROR_NO_KERNEL_FOUND, "Bad GPT");

	/* This causes the stream open call to fail */
	ResetMocks();
	lkp.disk_handle = NULL;
	TestLoadKernel(VBERROR_INVALID_KERNEL_FOUND, "Bad disk handle");
}

static void LoadKernelTest(void)
{
	ResetMocks();

	TestLoadKernel(0, "First kernel good");
	TEST_EQ(lkp.partition_number, 1, "  part num");
	TEST_EQ(lkp.bootloader_address, 0xbeadd008, "  bootloader addr");
	TEST_EQ(lkp.bootloader_size, 0x1234, "  bootloader size");
	TEST_STR_EQ((char *)lkp.partition_guid, "FakeGuid", "  guid");
	TEST_EQ(gpt_flag_external, 0, "GPT was internal");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST),
		0, "  recovery request");

	ResetMocks();
	mock_parts[1].start = 300;
	mock_parts[1].size = 150;
	TestLoadKernel(0, "Two good kernels");
	TEST_EQ(lkp.partition_number, 1, "  part num");
	TEST_EQ(mock_part_next, 1, "  didn't read second one");

	/* Fail if no kernels found */
	ResetMocks();
	mock_parts[0].size = 0;
	TestLoadKernel(VBERROR_NO_KERNEL_FOUND, "No kernels");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST),
		VB2_RECOVERY_RW_NO_OS, "  recovery request");

	/* Skip kernels which are too small */
	ResetMocks();
	mock_parts[0].size = 10;
	TestLoadKernel(VBERROR_INVALID_KERNEL_FOUND, "Too small");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST),
		VB2_RECOVERY_RW_INVALID_OS, "  recovery request");

	ResetMocks();
	disk_read_to_fail = 100;
	TestLoadKernel(VBERROR_INVALID_KERNEL_FOUND,
		       "Fail reading kernel start");

	ResetMocks();
	key_block_verify_fail = 1;
	TestLoadKernel(VBERROR_INVALID_KERNEL_FOUND, "Fail key block sig");

	/* In dev mode, fail if hash is bad too */
	ResetMocks();
	ctx.flags |= VB2_CONTEXT_DEVELOPER_MODE;
	key_block_verify_fail = 2;
	TestLoadKernel(VBERROR_INVALID_KERNEL_FOUND, "Fail key block dev hash");

	/* But just bad sig is ok */
	ResetMocks();
	ctx.flags |= VB2_CONTEXT_DEVELOPER_MODE;
	key_block_verify_fail = 1;
	TestLoadKernel(0, "Succeed key block dev sig");

	/* In dev mode and requiring signed kernel, fail if sig is bad */
	ResetMocks();
	ctx.flags |= VB2_CONTEXT_DEVELOPER_MODE;
	vb2_nv_set(&ctx, VB2_NV_DEV_BOOT_SIGNED_ONLY, 1);
	key_block_verify_fail = 1;
	TestLoadKernel(VBERROR_INVALID_KERNEL_FOUND, "Fail key block dev sig");

	ResetMocks();
	ctx.flags |= VB2_CONTEXT_DEVELOPER_MODE;
	lkp.fwmp = &fwmp;
	fwmp.flags |= FWMP_DEV_ENABLE_OFFICIAL_ONLY;
	key_block_verify_fail = 1;
	TestLoadKernel(VBERROR_INVALID_KERNEL_FOUND,
		       "Fail key block dev sig fwmp");

	/* Check key block flag mismatches */
	ResetMocks();
	kbh.key_block_flags =
		KEY_BLOCK_FLAG_RECOVERY_0 | KEY_BLOCK_FLAG_DEVELOPER_1;
	TestLoadKernel(VBERROR_INVALID_KERNEL_FOUND,
		       "Key block dev flag mismatch");

	ResetMocks();
	kbh.key_block_flags =
		KEY_BLOCK_FLAG_RECOVERY_1 | KEY_BLOCK_FLAG_DEVELOPER_0;
	TestLoadKernel(VBERROR_INVALID_KERNEL_FOUND,
		       "Key block rec flag mismatch");

	ResetMocks();
	ctx.flags |= VB2_CONTEXT_RECOVERY_MODE;
	kbh.key_block_flags =
		KEY_BLOCK_FLAG_RECOVERY_1 | KEY_BLOCK_FLAG_DEVELOPER_1;
	TestLoadKernel(VBERROR_INVALID_KERNEL_FOUND,
		       "Key block recdev flag mismatch");

	ResetMocks();
	ctx.flags |= VB2_CONTEXT_RECOVERY_MODE | VB2_CONTEXT_DEVELOPER_MODE;
	kbh.key_block_flags =
		KEY_BLOCK_FLAG_RECOVERY_1 | KEY_BLOCK_FLAG_DEVELOPER_0;
	TestLoadKernel(VBERROR_INVALID_KERNEL_FOUND,
		       "Key block rec!dev flag mismatch");

	ResetMocks();
	kbh.data_key.key_version = 1;
	TestLoadKernel(VBERROR_INVALID_KERNEL_FOUND,
		       "Key block kernel key rollback");

	ResetMocks();
	kbh.data_key.key_version = 0x10000;
	TestLoadKernel(VBERROR_INVALID_KERNEL_FOUND,
		       "Key block kernel key version too big");

	ResetMocks();
	kbh.data_key.key_version = 3;
	TestLoadKernel(0, "Key block version roll forward");
	TEST_EQ(shared->kernel_version_tpm, 0x30001, "  shared version");

	ResetMocks();
	kbh.data_key.key_version = 3;
	mock_parts[1].start = 300;
	mock_parts[1].size = 150;
	TestLoadKernel(0, "Two kernels roll forward");
	TEST_EQ(mock_part_next, 2, "  read both");
	TEST_EQ(shared->kernel_version_tpm, 0x30001, "  shared version");

	ResetMocks();
	kbh.data_key.key_version = 1;
	ctx.flags |= VB2_CONTEXT_DEVELOPER_MODE;
	TestLoadKernel(0, "Key version ignored in dev mode");

	ResetMocks();
	kbh.data_key.key_version = 1;
	ctx.flags |= VB2_CONTEXT_RECOVERY_MODE;
	TestLoadKernel(0, "Key version ignored in rec mode");

	ResetMocks();
	unpack_key_fail = 2;
	TestLoadKernel(VBERROR_INVALID_KERNEL_FOUND, "Bad data key");

	ResetMocks();
	preamble_verify_fail = 1;
	TestLoadKernel(VBERROR_INVALID_KERNEL_FOUND, "Bad preamble");

	ResetMocks();
	kph.kernel_version = 0;
	TestLoadKernel(VBERROR_INVALID_KERNEL_FOUND, "Kernel version rollback");

	ResetMocks();
	kph.kernel_version = 0;
	ctx.flags |= VB2_CONTEXT_DEVELOPER_MODE;
	TestLoadKernel(0, "Kernel version ignored in dev mode");

	ResetMocks();
	kph.kernel_version = 0;
	ctx.flags |= VB2_CONTEXT_RECOVERY_MODE;
	TestLoadKernel(0, "Kernel version ignored in rec mode");

	/* Check developer key hash - bad */
	ResetMocks();
	ctx.flags |= VB2_CONTEXT_DEVELOPER_MODE;
	lkp.fwmp = &fwmp;
	fwmp.flags |= FWMP_DEV_USE_KEY_HASH;
	fwmp.dev_key_hash[0]++;
	TestLoadKernel(VBERROR_INVALID_KERNEL_FOUND,
		       "Fail key block dev fwmp hash");

	/* Check developer key hash - good */
	ResetMocks();
	ctx.flags |= VB2_CONTEXT_DEVELOPER_MODE;
	lkp.fwmp = &fwmp;
	fwmp.flags |= FWMP_DEV_USE_KEY_HASH;
	TestLoadKernel(0, "Good key block dev fwmp hash");

	ResetMocks();
	kph.preamble_size |= 0x07;
	TestLoadKernel(VBERROR_INVALID_KERNEL_FOUND, "Kernel body offset");

	ResetMocks();
	kph.preamble_size += 65536;
	TestLoadKernel(VBERROR_INVALID_KERNEL_FOUND, "Kernel body offset huge");

	/* Check getting kernel load address from header */
	ResetMocks();
	kph.body_load_address = (size_t)kernel_buffer;
	lkp.kernel_buffer = NULL;
	TestLoadKernel(0, "Get load address from preamble");
	TEST_PTR_EQ(lkp.kernel_buffer, kernel_buffer, "  address");
	/* Size is rounded up to nearest sector */
	TEST_EQ(lkp.kernel_buffer_size, 70144, "  size");

	ResetMocks();
	lkp.kernel_buffer_size = 8192;
	TestLoadKernel(VBERROR_INVALID_KERNEL_FOUND,
		       "Kernel too big for buffer");

	ResetMocks();
	mock_parts[0].size = 130;
	TestLoadKernel(VBERROR_INVALID_KERNEL_FOUND,
		       "Kernel too big for partition");

	ResetMocks();
	kph.body_signature.data_size = 8192;
	TestLoadKernel(0, "Kernel tiny");

	ResetMocks();
	disk_read_to_fail = 228;
	TestLoadKernel(VBERROR_INVALID_KERNEL_FOUND,
		       "Fail reading kernel data");

	ResetMocks();
	verify_data_fail = 1;
	TestLoadKernel(VBERROR_INVALID_KERNEL_FOUND, "Bad data");

	/* Check that EXTERNAL_GPT flag makes it down */
	ResetMocks();
	lkp.boot_flags |= BOOT_FLAG_EXTERNAL_GPT;
	TestLoadKernel(0, "Succeed external GPT");
	TEST_EQ(gpt_flag_external, 1, "GPT was external");

	/* Check recovery from unreadble primary GPT */
	ResetMocks();
	disk_read_to_fail = 1;
	TestLoadKernel(0, "Can't read disk");
}

int main(void)
{
	ReadWriteGptTest();
	InvalidParamsTest();
	LoadKernelTest();

	return gTestSuccess ? 0 : 255;
}
