/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <string.h>

#include "../cgpt/cgpt.h"
#include "cgptlib_internal.h"
#include "cgptlib_test.h"
#include "crc32.h"
#include "crc32_test.h"
#include "gpt.h"
#include "test_common.h"
#define _STUB_IMPLEMENTATION_
#include "utility.h"

/*
 * Testing partition layout (sector_bytes=512)
 *
 *     LBA   Size  Usage
 * ---------------------------------------------------------
 *       0      1  PMBR
 *       1      1  primary partition header
 *       2     32  primary partition entries (128B * 128)
 *      34    100  kernel A (index: 0)
 *     134    100  root A (index: 1)
 *     234    100  root B (index: 2)
 *     334    100  kernel B (index: 3)
 *     434     32  secondary partition entries
 *     466      1  secondary partition header
 *     467
 */
#define KERNEL_A 0
#define KERNEL_B 1
#define ROOTFS_A 2
#define ROOTFS_B 3
#define KERNEL_X 2 /* Overload ROOTFS_A, for some GetNext tests */
#define KERNEL_Y 3 /* Overload ROOTFS_B, for some GetNext tests */

#define DEFAULT_SECTOR_SIZE 512
#define MAX_SECTOR_SIZE 4096
#define DEFAULT_DRIVE_SECTORS 467
#define TOTAL_ENTRIES_SIZE (MAX_NUMBER_OF_ENTRIES * sizeof(GptEntry)) /* 16384 */
#define PARTITION_ENTRIES_SIZE TOTAL_ENTRIES_SIZE /* 16384 */

static const Guid guid_zero = {{{0, 0, 0, 0, 0, {0, 0, 0, 0, 0, 0}}}};
static const Guid guid_kernel = GPT_ENT_TYPE_CHROMEOS_KERNEL;
static const Guid guid_rootfs = GPT_ENT_TYPE_CHROMEOS_ROOTFS;

// cgpt_common.c requires these be defined if linked in.
const char *progname = "CGPT-TEST";
const char *command = "TEST";

/*
 * Copy a random-for-this-program-only Guid into the dest. The num parameter
 * completely determines the Guid.
 */
static void SetGuid(void *dest, uint32_t num)
{
	Guid g = {{{num,0xd450,0x44bc,0xa6,0x93,
		    {0xb8,0xac,0x75,0x5f,0xcd,0x48}}}};
	Memcpy(dest, &g, sizeof(Guid));
}

/*
 * Given a GptData pointer, first re-calculate entries CRC32 value, then reset
 * header CRC32 value to 0, and calculate header CRC32 value.  Both primary and
 * secondary are updated.
 */
static void RefreshCrc32(GptData *gpt)
{
	GptHeader *header, *header2;
	GptEntry *entries, *entries2;

	header = (GptHeader *)gpt->primary_header;
	entries = (GptEntry *)gpt->primary_entries;
	header2 = (GptHeader *)gpt->secondary_header;
	entries2 = (GptEntry *)gpt->secondary_entries;

	header->entries_crc32 =
		Crc32((uint8_t *)entries,
		      header->number_of_entries * header->size_of_entry);
	header->header_crc32 = 0;
	header->header_crc32 = Crc32((uint8_t *)header, header->size);
	header2->entries_crc32 =
		Crc32((uint8_t *)entries2,
		      header2->number_of_entries * header2->size_of_entry);
	header2->header_crc32 = 0;
	header2->header_crc32 = Crc32((uint8_t *)header2, header2->size);
}

static void ZeroHeaders(GptData *gpt)
{
	Memset(gpt->primary_header, 0, MAX_SECTOR_SIZE);
	Memset(gpt->secondary_header, 0, MAX_SECTOR_SIZE);
}

static void ZeroEntries(GptData *gpt)
{
	Memset(gpt->primary_entries, 0, PARTITION_ENTRIES_SIZE);
	Memset(gpt->secondary_entries, 0, PARTITION_ENTRIES_SIZE);
}

static void ZeroHeadersEntries(GptData *gpt)
{
	ZeroHeaders(gpt);
	ZeroEntries(gpt);
}

/*
 * Return a pointer to a static GptData instance (no free is required).
 * All fields are zero except 4 pointers linking to header and entries.
 * All content of headers and entries are zero.
 */
static GptData *GetEmptyGptData(void)
{
	static GptData gpt;
	static uint8_t primary_header[MAX_SECTOR_SIZE];
	static uint8_t primary_entries[PARTITION_ENTRIES_SIZE];
	static uint8_t secondary_header[MAX_SECTOR_SIZE];
	static uint8_t secondary_entries[PARTITION_ENTRIES_SIZE];

	Memset(&gpt, 0, sizeof(gpt));
	gpt.primary_header = primary_header;
	gpt.primary_entries = primary_entries;
	gpt.secondary_header = secondary_header;
	gpt.secondary_entries = secondary_entries;
	ZeroHeadersEntries(&gpt);

	/* Initialize GptData internal states. */
	gpt.current_kernel = CGPT_KERNEL_ENTRY_NOT_FOUND;

	return &gpt;
}

/*
 * Fill in most of fields and creates the layout described in the top of this
 * file. Before calling this function, primary/secondary header/entries must
 * have been pointed to the buffer, say, a gpt returned from GetEmptyGptData().
 * This function returns a good (valid) copy of GPT layout described in top of
 * this file.
 */
static void BuildTestGptData(GptData *gpt)
{
	GptHeader *header, *header2;
	GptEntry *entries, *entries2;
	Guid chromeos_kernel = GPT_ENT_TYPE_CHROMEOS_KERNEL;
	Guid chromeos_rootfs = GPT_ENT_TYPE_CHROMEOS_ROOTFS;

	gpt->sector_bytes = DEFAULT_SECTOR_SIZE;
	gpt->streaming_drive_sectors =
		gpt->gpt_drive_sectors = DEFAULT_DRIVE_SECTORS;
	gpt->current_kernel = CGPT_KERNEL_ENTRY_NOT_FOUND;
	gpt->valid_headers = MASK_BOTH;
	gpt->valid_entries = MASK_BOTH;
	gpt->modified = 0;

	/* Build primary */
	header = (GptHeader *)gpt->primary_header;
	entries = (GptEntry *)gpt->primary_entries;
	Memcpy(header->signature, GPT_HEADER_SIGNATURE,
	       sizeof(GPT_HEADER_SIGNATURE));
	header->revision = GPT_HEADER_REVISION;
	header->size = sizeof(GptHeader);
	header->reserved_zero = 0;
	header->my_lba = 1;
	header->alternate_lba = DEFAULT_DRIVE_SECTORS - 1;
	header->first_usable_lba = 34;
	header->last_usable_lba = DEFAULT_DRIVE_SECTORS - 1 - 32 - 1;  /* 433 */
	header->entries_lba = 2;
	  /* 512B / 128B * 32sectors = 128 entries */
	header->number_of_entries = 128;
	header->size_of_entry = 128;  /* bytes */
	Memcpy(&entries[0].type, &chromeos_kernel, sizeof(chromeos_kernel));
	SetGuid(&entries[0].unique, 0);
	entries[0].starting_lba = 34;
	entries[0].ending_lba = 133;
	Memcpy(&entries[1].type, &chromeos_rootfs, sizeof(chromeos_rootfs));
	SetGuid(&entries[1].unique, 1);
	entries[1].starting_lba = 134;
	entries[1].ending_lba = 232;
	Memcpy(&entries[2].type, &chromeos_rootfs, sizeof(chromeos_rootfs));
	SetGuid(&entries[2].unique, 2);
	entries[2].starting_lba = 234;
	entries[2].ending_lba = 331;
	Memcpy(&entries[3].type, &chromeos_kernel, sizeof(chromeos_kernel));
	SetGuid(&entries[3].unique, 3);
	entries[3].starting_lba = 334;
	entries[3].ending_lba = 430;

	/* Build secondary */
	header2 = (GptHeader *)gpt->secondary_header;
	entries2 = (GptEntry *)gpt->secondary_entries;
	Memcpy(header2, header, sizeof(GptHeader));
	Memcpy(entries2, entries, PARTITION_ENTRIES_SIZE);
	header2->my_lba = DEFAULT_DRIVE_SECTORS - 1;  /* 466 */
	header2->alternate_lba = 1;
	header2->entries_lba = DEFAULT_DRIVE_SECTORS - 1 - 32;  /* 434 */

	RefreshCrc32(gpt);
}

/*
 * Test if the structures are the expected size; if this fails, struct packing
 * is not working properly.
 */
static int StructSizeTest(void)
{

	EXPECT(GUID_EXPECTED_SIZE == sizeof(Guid));
	EXPECT(GPTHEADER_EXPECTED_SIZE == sizeof(GptHeader));
	EXPECT(GPTENTRY_EXPECTED_SIZE == sizeof(GptEntry));
	return TEST_OK;
}


/* Test if the default structure returned by BuildTestGptData() is good. */
static int TestBuildTestGptData(void)
{
	GptData *gpt;

	gpt = GetEmptyGptData();
	BuildTestGptData(gpt);
	EXPECT(GPT_SUCCESS == GptInit(gpt));
	gpt->sector_bytes = 0;
	EXPECT(GPT_ERROR_INVALID_SECTOR_SIZE == GptInit(gpt));
	return TEST_OK;
}

/*
 * Test if wrong sector_bytes or drive_sectors is detected by GptInit().
 * Currently we only support 512 bytes per sector.  In the future, we may
 * support other sizes.  A too small drive_sectors should be rejected by
 * GptInit().
 */
static int ParameterTests(void)
{
	GptData *gpt;
	struct {
		uint32_t sector_bytes;
		uint64_t drive_sectors;
		int expected_retval;
	} cases[] = {
		{512, DEFAULT_DRIVE_SECTORS, GPT_SUCCESS},
		{520, DEFAULT_DRIVE_SECTORS, GPT_ERROR_INVALID_SECTOR_SIZE},
		{512, 0, GPT_ERROR_INVALID_SECTOR_NUMBER},
		{512, 10, GPT_ERROR_INVALID_SECTOR_NUMBER},
		{512, GPT_PMBR_SECTORS + GPT_HEADER_SECTORS * 2 +
		 TOTAL_ENTRIES_SIZE / DEFAULT_SECTOR_SIZE * 2, GPT_SUCCESS},
		{4096, DEFAULT_DRIVE_SECTORS, GPT_ERROR_INVALID_SECTOR_SIZE},
	};
	int i;

	gpt = GetEmptyGptData();
	for (i = 0; i < ARRAY_SIZE(cases); ++i) {
		BuildTestGptData(gpt);
		gpt->sector_bytes = cases[i].sector_bytes;
		gpt->streaming_drive_sectors =
			gpt->gpt_drive_sectors = cases[i].drive_sectors;
		EXPECT(cases[i].expected_retval == CheckParameters(gpt));
	}

	return TEST_OK;
}

/* Test if header CRC in two copies are calculated. */
static int HeaderCrcTest(void)
{
	GptData *gpt = GetEmptyGptData();
	GptHeader *h1 = (GptHeader *)gpt->primary_header;

	BuildTestGptData(gpt);
	EXPECT(HeaderCrc(h1) == h1->header_crc32);

	/* CRC covers first byte of header */
	BuildTestGptData(gpt);
	gpt->primary_header[0] ^= 0xa5;
	EXPECT(HeaderCrc(h1) != h1->header_crc32);

	/* CRC covers last byte of header */
	BuildTestGptData(gpt);
	gpt->primary_header[h1->size - 1] ^= 0x5a;
	EXPECT(HeaderCrc(h1) != h1->header_crc32);

	/* CRC only covers header */
	BuildTestGptData(gpt);
	gpt->primary_header[h1->size] ^= 0x5a;
	EXPECT(HeaderCrc(h1) == h1->header_crc32);

	return TEST_OK;
}

/* Test if header-same comparison works. */
static int HeaderSameTest(void)
{
	GptData *gpt = GetEmptyGptData();
	GptHeader *h1 = (GptHeader *)gpt->primary_header;
	GptHeader *h2 = (GptHeader *)gpt->secondary_header;
	GptHeader h3;

	EXPECT(0 == HeaderFieldsSame(h1, h2));

	Memcpy(&h3, h2, sizeof(h3));
	h3.signature[0] ^= 0xba;
	EXPECT(1 == HeaderFieldsSame(h1, &h3));

	Memcpy(&h3, h2, sizeof(h3));
	h3.revision++;
	EXPECT(1 == HeaderFieldsSame(h1, &h3));

	Memcpy(&h3, h2, sizeof(h3));
	h3.size++;
	EXPECT(1 == HeaderFieldsSame(h1, &h3));

	Memcpy(&h3, h2, sizeof(h3));
	h3.reserved_zero++;
	EXPECT(1 == HeaderFieldsSame(h1, &h3));

	Memcpy(&h3, h2, sizeof(h3));
	h3.first_usable_lba++;
	EXPECT(1 == HeaderFieldsSame(h1, &h3));

	Memcpy(&h3, h2, sizeof(h3));
	h3.last_usable_lba++;
	EXPECT(1 == HeaderFieldsSame(h1, &h3));

	Memcpy(&h3, h2, sizeof(h3));
	h3.disk_uuid.u.raw[0] ^= 0xba;
	EXPECT(1 == HeaderFieldsSame(h1, &h3));

	Memcpy(&h3, h2, sizeof(h3));
	h3.number_of_entries++;
	EXPECT(1 == HeaderFieldsSame(h1, &h3));

	Memcpy(&h3, h2, sizeof(h3));
	h3.size_of_entry++;
	EXPECT(1 == HeaderFieldsSame(h1, &h3));

	Memcpy(&h3, h2, sizeof(h3));
	h3.entries_crc32++;
	EXPECT(1 == HeaderFieldsSame(h1, &h3));

	return TEST_OK;
}

/* Test if signature ("EFI PART") is checked. */
static int SignatureTest(void)
{
	GptData *gpt = GetEmptyGptData();
	GptHeader *h1 = (GptHeader *)gpt->primary_header;
	GptHeader *h2 = (GptHeader *)gpt->secondary_header;
	int i;

	EXPECT(1 == CheckHeader(NULL, 0, gpt->streaming_drive_sectors, gpt->gpt_drive_sectors, 0));

	for (i = 0; i < 8; ++i) {
		BuildTestGptData(gpt);
		h1->signature[i] ^= 0xff;
		h2->signature[i] ^= 0xff;
		RefreshCrc32(gpt);
		EXPECT(1 == CheckHeader(h1, 0, gpt->streaming_drive_sectors, gpt->gpt_drive_sectors, 0));
		EXPECT(1 == CheckHeader(h2, 1, gpt->streaming_drive_sectors, gpt->gpt_drive_sectors, 0));
	}

	return TEST_OK;
}

/*
 * The revision we currently support is GPT_HEADER_REVISION.  If the revision
 * in header is not that, we expect the header is invalid.
 */
static int RevisionTest(void)
{
	GptData *gpt = GetEmptyGptData();
	GptHeader *h1 = (GptHeader *)gpt->primary_header;
	GptHeader *h2 = (GptHeader *)gpt->secondary_header;
	int i;

	struct {
		uint32_t value_to_test;
		int expect_rv;
	} cases[] = {
		{0x01000000, 1},
		{0x00010000, 0},  /* GPT_HEADER_REVISION */
		{0x00000100, 1},
		{0x00000001, 1},
		{0x23010456, 1},
	};

	for (i = 0; i < ARRAY_SIZE(cases); ++i) {
		BuildTestGptData(gpt);
		h1->revision = cases[i].value_to_test;
		h2->revision = cases[i].value_to_test;
		RefreshCrc32(gpt);

		EXPECT(CheckHeader(h1, 0, gpt->streaming_drive_sectors, gpt->gpt_drive_sectors, 0) ==
		       cases[i].expect_rv);
		EXPECT(CheckHeader(h2, 1, gpt->streaming_drive_sectors, gpt->gpt_drive_sectors, 0) ==
		       cases[i].expect_rv);
	}
	return TEST_OK;
}

static int SizeTest(void)
{
	GptData *gpt = GetEmptyGptData();
	GptHeader *h1 = (GptHeader *)gpt->primary_header;
	GptHeader *h2 = (GptHeader *)gpt->secondary_header;
	int i;

	struct {
		uint32_t value_to_test;
		int expect_rv;
	} cases[] = {
		{91, 1},
		{92, 0},
		{93, 0},
		{511, 0},
		{512, 0},
		{513, 1},
	};

	for (i = 0; i < ARRAY_SIZE(cases); ++i) {
		BuildTestGptData(gpt);
		h1->size = cases[i].value_to_test;
		h2->size = cases[i].value_to_test;
		RefreshCrc32(gpt);

		EXPECT(CheckHeader(h1, 0, gpt->streaming_drive_sectors, gpt->gpt_drive_sectors, 0) ==
		       cases[i].expect_rv);
		EXPECT(CheckHeader(h2, 1, gpt->streaming_drive_sectors, gpt->gpt_drive_sectors, 0) ==
		       cases[i].expect_rv);
	}
	return TEST_OK;
}

/* Test if CRC is checked. */
static int CrcFieldTest(void)
{
	GptData *gpt = GetEmptyGptData();
	GptHeader *h1 = (GptHeader *)gpt->primary_header;
	GptHeader *h2 = (GptHeader *)gpt->secondary_header;

	BuildTestGptData(gpt);
	/* Modify a field that the header verification doesn't care about */
	h1->entries_crc32++;
	h2->entries_crc32++;
	EXPECT(1 == CheckHeader(h1, 0, gpt->streaming_drive_sectors, gpt->gpt_drive_sectors, 0));
	EXPECT(1 == CheckHeader(h2, 1, gpt->streaming_drive_sectors, gpt->gpt_drive_sectors, 0));
	/* Refresh the CRC; should pass now */
	RefreshCrc32(gpt);
	EXPECT(0 == CheckHeader(h1, 0, gpt->streaming_drive_sectors, gpt->gpt_drive_sectors, 0));
	EXPECT(0 == CheckHeader(h2, 1, gpt->streaming_drive_sectors, gpt->gpt_drive_sectors, 0));

	return TEST_OK;
}

/* Test if reserved fields are checked.  We'll try non-zero values to test. */
static int ReservedFieldsTest(void)
{
	GptData *gpt = GetEmptyGptData();
	GptHeader *h1 = (GptHeader *)gpt->primary_header;
	GptHeader *h2 = (GptHeader *)gpt->secondary_header;

	BuildTestGptData(gpt);
	h1->reserved_zero ^= 0x12345678;  /* whatever random */
	h2->reserved_zero ^= 0x12345678;  /* whatever random */
	RefreshCrc32(gpt);
	EXPECT(1 == CheckHeader(h1, 0, gpt->streaming_drive_sectors, gpt->gpt_drive_sectors, 0));
	EXPECT(1 == CheckHeader(h2, 1, gpt->streaming_drive_sectors, gpt->gpt_drive_sectors, 0));

#ifdef PADDING_CHECKED
	/* TODO: padding check is currently disabled */
	BuildTestGptData(gpt);
	h1->padding[12] ^= 0x34;  /* whatever random */
	h2->padding[56] ^= 0x78;  /* whatever random */
	RefreshCrc32(gpt);
	EXPECT(1 == CheckHeader(h1, 0, gpt->streaming_drive_sectors, gpt->gpt_drive_sectors, 0));
	EXPECT(1 == CheckHeader(h2, 1, gpt->streaming_drive_sectors, gpt->gpt_drive_sectors, 0));
#endif

	return TEST_OK;
}

/*
 * Technically, any size which is 2^N where N > 6 should work, but our
 * library only supports one size.
 */
static int SizeOfPartitionEntryTest(void) {
	GptData *gpt = GetEmptyGptData();
	GptHeader *h1 = (GptHeader *)gpt->primary_header;
	GptHeader *h2 = (GptHeader *)gpt->secondary_header;
	int i;

	struct {
		uint32_t value_to_test;
		int expect_rv;
	} cases[] = {
		{127, 1},
		{128, 0},
		{129, 1},
		{256, 1},
		{512, 1},
	};

	/* Check size of entryes */
	for (i = 0; i < ARRAY_SIZE(cases); ++i) {
		BuildTestGptData(gpt);
		h1->size_of_entry = cases[i].value_to_test;
		h2->size_of_entry = cases[i].value_to_test;
		h1->number_of_entries = TOTAL_ENTRIES_SIZE /
			cases[i].value_to_test;
		h2->number_of_entries = TOTAL_ENTRIES_SIZE /
			cases[i].value_to_test;
		RefreshCrc32(gpt);

		EXPECT(CheckHeader(h1, 0, gpt->streaming_drive_sectors, gpt->gpt_drive_sectors, 0) ==
		       cases[i].expect_rv);
		EXPECT(CheckHeader(h2, 1, gpt->streaming_drive_sectors, gpt->gpt_drive_sectors, 0) ==
		       cases[i].expect_rv);
	}

	return TEST_OK;
}

/*
 * Technically, any size which is 2^N where N > 6 should work, but our library
 * only supports one size.
 */
static int NumberOfPartitionEntriesTest(void)
{
	GptData *gpt = GetEmptyGptData();
	GptHeader *h1 = (GptHeader *)gpt->primary_header;
	GptHeader *h2 = (GptHeader *)gpt->secondary_header;

	BuildTestGptData(gpt);
	h1->number_of_entries--;
	h2->number_of_entries /= 2;
	/* Because we halved h2 entries, its entries_lba is going to change. */
	h2->entries_lba = h2->my_lba - CalculateEntriesSectors(h2);
	RefreshCrc32(gpt);
	EXPECT(1 == CheckHeader(h1, 0, gpt->streaming_drive_sectors, gpt->gpt_drive_sectors, 0));
	EXPECT(1 == CheckHeader(h2, 1, gpt->streaming_drive_sectors, gpt->gpt_drive_sectors, 0));
	/* But it's okay to have less if the GPT structs are stored elsewhere. */
	EXPECT(0 == CheckHeader(h1, 0, gpt->streaming_drive_sectors, gpt->gpt_drive_sectors, GPT_FLAG_EXTERNAL));
	EXPECT(0 == CheckHeader(h2, 1, gpt->streaming_drive_sectors, gpt->gpt_drive_sectors, GPT_FLAG_EXTERNAL));

	return TEST_OK;
}


/* Test if myLBA field is checked (1 for primary, last for secondary). */
static int MyLbaTest(void)
{
	GptData *gpt = GetEmptyGptData();
	GptHeader *h1 = (GptHeader *)gpt->primary_header;
	GptHeader *h2 = (GptHeader *)gpt->secondary_header;

	/* myLBA depends on primary vs secondary flag */
	BuildTestGptData(gpt);
	EXPECT(1 == CheckHeader(h1, 1, gpt->streaming_drive_sectors, gpt->gpt_drive_sectors, 0));
	EXPECT(1 == CheckHeader(h2, 0, gpt->streaming_drive_sectors, gpt->gpt_drive_sectors, 0));

	BuildTestGptData(gpt);
	h1->my_lba--;
	h2->my_lba--;
	RefreshCrc32(gpt);
	EXPECT(1 == CheckHeader(h1, 0, gpt->streaming_drive_sectors, gpt->gpt_drive_sectors, 0));
	EXPECT(1 == CheckHeader(h2, 1, gpt->streaming_drive_sectors, gpt->gpt_drive_sectors, 0));

	BuildTestGptData(gpt);
	h1->my_lba = 2;
	h2->my_lba--;
	RefreshCrc32(gpt);
	EXPECT(1 == CheckHeader(h1, 0, gpt->streaming_drive_sectors, gpt->gpt_drive_sectors, 0));
	EXPECT(1 == CheckHeader(h2, 1, gpt->streaming_drive_sectors, gpt->gpt_drive_sectors, 0));

	/* We should ignore the alternate_lba field entirely */
	BuildTestGptData(gpt);
	h1->alternate_lba++;
	h2->alternate_lba++;
	RefreshCrc32(gpt);
	EXPECT(0 == CheckHeader(h1, 0, gpt->streaming_drive_sectors, gpt->gpt_drive_sectors, 0));
	EXPECT(0 == CheckHeader(h2, 1, gpt->streaming_drive_sectors, gpt->gpt_drive_sectors, 0));

	BuildTestGptData(gpt);
	h1->alternate_lba--;
	h2->alternate_lba--;
	RefreshCrc32(gpt);
	EXPECT(0 == CheckHeader(h1, 0, gpt->streaming_drive_sectors, gpt->gpt_drive_sectors, 0));
	EXPECT(0 == CheckHeader(h2, 1, gpt->streaming_drive_sectors, gpt->gpt_drive_sectors, 0));

	BuildTestGptData(gpt);
	h1->entries_lba++;
	h2->entries_lba++;
	RefreshCrc32(gpt);
	/*
	 * We support a padding between primary GPT header and its entries. So
	 * this still passes.
	 */
	EXPECT(0 == CheckHeader(h1, 0, gpt->streaming_drive_sectors, gpt->gpt_drive_sectors, 0));
	/*
	 * But the secondary table should fail because it would overlap the
	 * header, which is now lying after its entry array.
	 */
	EXPECT(1 == CheckHeader(h2, 1, gpt->streaming_drive_sectors, gpt->gpt_drive_sectors, 0));

	BuildTestGptData(gpt);
	h1->entries_lba--;
	h2->entries_lba--;
	RefreshCrc32(gpt);
	EXPECT(1 == CheckHeader(h1, 0, gpt->streaming_drive_sectors, gpt->gpt_drive_sectors, 0));
	EXPECT(1 == CheckHeader(h2, 1, gpt->streaming_drive_sectors, gpt->gpt_drive_sectors, 0));

	return TEST_OK;
}

/* Test if FirstUsableLBA and LastUsableLBA are checked.
 * FirstUsableLBA must be after the end of the primary GPT table array.
 * LastUsableLBA must be before the start of the secondary GPT table array.
 * FirstUsableLBA <= LastUsableLBA.
 * Also see CheckHeaderOffDevice() test. */
static int FirstUsableLbaAndLastUsableLbaTest(void)
{
	GptData *gpt = GetEmptyGptData();
	GptHeader *h1 = (GptHeader *)gpt->primary_header;
	GptHeader *h2 = (GptHeader *)gpt->secondary_header;
	int i;

	struct {
		uint64_t primary_entries_lba;
		uint64_t primary_first_usable_lba;
		uint64_t primary_last_usable_lba;
		uint64_t secondary_first_usable_lba;
		uint64_t secondary_last_usable_lba;
		uint64_t secondary_entries_lba;
		int primary_rv;
		int secondary_rv;
	} cases[] = {
		{2,  34, 433,   34, 433, 434,  0, 0},
		{2,  34, 432,   34, 430, 434,  0, 0},
		{2,  33, 433,   33, 433, 434,  1, 1},
		{2,  34, 434,   34, 433, 434,  1, 0},
		{2,  34, 433,   34, 434, 434,  0, 1},
		{2,  35, 433,   35, 433, 434,  0, 0},
		{2, 433, 433,  433, 433, 434,  0, 0},
		{2, 434, 433,  434, 434, 434,  1, 1},
		{2, 433,  34,   34, 433, 434,  1, 0},
		{2,  34, 433,  433,  34, 434,  0, 1},
	};

	for (i = 0; i < ARRAY_SIZE(cases); ++i) {
		BuildTestGptData(gpt);
		h1->entries_lba = cases[i].primary_entries_lba;
		h1->first_usable_lba = cases[i].primary_first_usable_lba;
		h1->last_usable_lba = cases[i].primary_last_usable_lba;
		h2->entries_lba = cases[i].secondary_entries_lba;
		h2->first_usable_lba = cases[i].secondary_first_usable_lba;
		h2->last_usable_lba = cases[i].secondary_last_usable_lba;
		RefreshCrc32(gpt);

		EXPECT(CheckHeader(h1, 0, gpt->streaming_drive_sectors, gpt->gpt_drive_sectors, 0) ==
		       cases[i].primary_rv);
		EXPECT(CheckHeader(h2, 1, gpt->streaming_drive_sectors, gpt->gpt_drive_sectors, 0) ==
		       cases[i].secondary_rv);
	}

	return TEST_OK;
}

/*
 * Test if PartitionEntryArrayCRC32 is checked.  PartitionEntryArrayCRC32 must
 * be calculated over SizeOfPartitionEntry * NumberOfPartitionEntries bytes.
 */
static int EntriesCrcTest(void)
{
	GptData *gpt = GetEmptyGptData();
	GptHeader *h1 = (GptHeader *)gpt->primary_header;
	GptEntry *e1 = (GptEntry *)(gpt->primary_entries);
	GptEntry *e2 = (GptEntry *)(gpt->secondary_entries);

	/* Modify first byte of primary entries, and expect the CRC is wrong. */
	BuildTestGptData(gpt);
	EXPECT(0 == CheckEntries(e1, h1));
	EXPECT(0 == CheckEntries(e2, h1));
	gpt->primary_entries[0] ^= 0xa5;  /* just XOR a non-zero value */
	gpt->secondary_entries[TOTAL_ENTRIES_SIZE-1] ^= 0x5a;
	EXPECT(GPT_ERROR_CRC_CORRUPTED == CheckEntries(e1, h1));
	EXPECT(GPT_ERROR_CRC_CORRUPTED == CheckEntries(e2, h1));

	return TEST_OK;
}

/*
 * Test if partition geometry is checked.
 * All active (non-zero PartitionTypeGUID) partition entries should have:
 *   entry.StartingLBA >= header.FirstUsableLBA
 *   entry.EndingLBA <= header.LastUsableLBA
 *   entry.StartingLBA <= entry.EndingLBA
 */
static int ValidEntryTest(void)
{
	GptData *gpt = GetEmptyGptData();
	GptHeader *h1 = (GptHeader *)gpt->primary_header;
	GptEntry *e1 = (GptEntry *)(gpt->primary_entries);

	/* error case: entry.StartingLBA < header.FirstUsableLBA */
	BuildTestGptData(gpt);
	e1[0].starting_lba = h1->first_usable_lba - 1;
	RefreshCrc32(gpt);
	EXPECT(GPT_ERROR_OUT_OF_REGION == CheckEntries(e1, h1));

	/* error case: entry.EndingLBA > header.LastUsableLBA */
	BuildTestGptData(gpt);
	e1[2].ending_lba = h1->last_usable_lba + 1;
	RefreshCrc32(gpt);
	EXPECT(GPT_ERROR_OUT_OF_REGION == CheckEntries(e1, h1));

	/* error case: entry.StartingLBA > entry.EndingLBA */
	BuildTestGptData(gpt);
	e1[3].starting_lba = e1[3].ending_lba + 1;
	RefreshCrc32(gpt);
	EXPECT(GPT_ERROR_OUT_OF_REGION == CheckEntries(e1, h1));

	/* case: non active entry should be ignored. */
	BuildTestGptData(gpt);
	Memset(&e1[1].type, 0, sizeof(e1[1].type));
	e1[1].starting_lba = e1[1].ending_lba + 1;
	RefreshCrc32(gpt);
	EXPECT(0 == CheckEntries(e1, h1));

	return TEST_OK;
}

/* Test if overlapped partition tables can be detected. */
static int OverlappedPartitionTest(void) {
	GptData *gpt = GetEmptyGptData();
	GptHeader *h = (GptHeader *)gpt->primary_header;
	GptEntry *e = (GptEntry *)gpt->primary_entries;
	int i, j;

	struct {
		int overlapped;
		struct {
			int active;
			uint64_t starting_lba;
			uint64_t ending_lba;
		} entries[16];  /* enough for testing. */
	} cases[] = {
		{GPT_SUCCESS, {{0, 100, 199}}},
		{GPT_SUCCESS, {{1, 100, 199}}},
		{GPT_SUCCESS, {{1, 100, 150}, {1, 200, 250}, {1, 300, 350}}},
		{GPT_ERROR_START_LBA_OVERLAP,
		 {{1, 200, 299}, {1, 100, 199}, {1, 100, 100}}},
		{GPT_ERROR_END_LBA_OVERLAP,
		 {{1, 200, 299}, {1, 100, 199}, {1, 299, 299}}},
		{GPT_SUCCESS, {{1, 300, 399}, {1, 200, 299}, {1, 100, 199}}},
		{GPT_ERROR_END_LBA_OVERLAP,
		 {{1, 100, 199}, {1, 199, 299}, {1, 299, 399}}},
		{GPT_ERROR_START_LBA_OVERLAP,
		 {{1, 100, 199}, {1, 200, 299}, {1, 75, 399}}},
		{GPT_ERROR_START_LBA_OVERLAP,
		 {{1, 100, 199}, {1, 75, 250}, {1, 200, 299}}},
		{GPT_ERROR_END_LBA_OVERLAP,
		 {{1, 75, 150}, {1, 100, 199}, {1, 200, 299}}},
		{GPT_ERROR_START_LBA_OVERLAP,
		 {{1, 200, 299}, {1, 100, 199}, {1, 300, 399}, {1, 100, 399}}},
		{GPT_SUCCESS,
		 {{1, 200, 299}, {1, 100, 199}, {1, 300, 399}, {0, 100, 399}}},
		{GPT_ERROR_START_LBA_OVERLAP,
		 {{1, 200, 300}, {1, 100, 200}, {1, 100, 400}, {1, 300, 400}}},
		{GPT_ERROR_START_LBA_OVERLAP,
		 {{0, 200, 300}, {1, 100, 200}, {1, 100, 400}, {1, 300, 400}}},
		{GPT_SUCCESS,
		 {{1, 200, 300}, {1, 100, 199}, {0, 100, 400}, {0, 300, 400}}},
		{GPT_ERROR_END_LBA_OVERLAP,
		 {{1, 200, 299}, {1, 100, 199}, {1, 199, 199}}},
		{GPT_SUCCESS, {{1, 200, 299}, {0, 100, 199}, {1, 199, 199}}},
		{GPT_SUCCESS, {{1, 200, 299}, {1, 100, 199}, {0, 199, 199}}},
		{GPT_ERROR_START_LBA_OVERLAP,
		 {{1, 199, 199}, {1, 200, 200}, {1, 201, 201}, {1, 202, 202},
		  {1, 203, 203}, {1, 204, 204}, {1, 205, 205}, {1, 206, 206},
		  {1, 207, 207}, {1, 208, 208}, {1, 199, 199}}},
		{GPT_SUCCESS,
		 {{1, 199, 199}, {1, 200, 200}, {1, 201, 201}, {1, 202, 202},
		  {1, 203, 203}, {1, 204, 204}, {1, 205, 205}, {1, 206, 206},
		  {1, 207, 207}, {1, 208, 208}, {0, 199, 199}}},
	};

	for (i = 0; i < ARRAY_SIZE(cases); ++i) {
		BuildTestGptData(gpt);
		ZeroEntries(gpt);
		for(j = 0; j < ARRAY_SIZE(cases[0].entries); ++j) {
			if (!cases[i].entries[j].starting_lba)
				break;

			if (cases[i].entries[j].active) {
				Memcpy(&e[j].type, &guid_kernel, sizeof(Guid));
			}
			SetGuid(&e[j].unique, j);
			e[j].starting_lba = cases[i].entries[j].starting_lba;
			e[j].ending_lba = cases[i].entries[j].ending_lba;
		}
		RefreshCrc32(gpt);

		EXPECT(cases[i].overlapped == CheckEntries(e, h));
	}
	return TEST_OK;
}

/* Test both sanity checking and repair. */
static int SanityCheckTest(void)
{
	GptData *gpt = GetEmptyGptData();
	GptHeader *h1 = (GptHeader *)gpt->primary_header;
	GptEntry *e1 = (GptEntry *)gpt->primary_entries;
	uint8_t *tempptr;

	/* Unmodified test data is completely sane */
	BuildTestGptData(gpt);
	EXPECT(GPT_SUCCESS == GptSanityCheck(gpt));
	EXPECT(MASK_BOTH == gpt->valid_headers);
	EXPECT(MASK_BOTH == gpt->valid_entries);
	/* Repair doesn't damage it */
	GptRepair(gpt);
	EXPECT(GPT_SUCCESS == GptSanityCheck(gpt));
	EXPECT(MASK_BOTH == gpt->valid_headers);
	EXPECT(MASK_BOTH == gpt->valid_entries);
	EXPECT(0 == gpt->modified);

	/* Invalid sector size should fail */
	BuildTestGptData(gpt);
	gpt->sector_bytes = 1024;
	EXPECT(GPT_ERROR_INVALID_SECTOR_SIZE == GptSanityCheck(gpt));

	/* Modify headers */
	BuildTestGptData(gpt);
	gpt->primary_header[0]++;
	gpt->secondary_header[0]++;
	EXPECT(GPT_ERROR_INVALID_HEADERS == GptSanityCheck(gpt));
	EXPECT(0 == gpt->valid_headers);
	EXPECT(0 == gpt->valid_entries);
	/* Repair can't fix completely busted headers */
	GptRepair(gpt);
	EXPECT(GPT_ERROR_INVALID_HEADERS == GptSanityCheck(gpt));
	EXPECT(0 == gpt->valid_headers);
	EXPECT(0 == gpt->valid_entries);
	EXPECT(0 == gpt->modified);

	BuildTestGptData(gpt);
	gpt->primary_header[0]++;
	EXPECT(GPT_SUCCESS == GptSanityCheck(gpt));
	EXPECT(MASK_SECONDARY == gpt->valid_headers);
	EXPECT(MASK_BOTH == gpt->valid_entries);
	GptRepair(gpt);
	EXPECT(GPT_SUCCESS == GptSanityCheck(gpt));
	EXPECT(MASK_BOTH == gpt->valid_headers);
	EXPECT(MASK_BOTH == gpt->valid_entries);
	EXPECT(GPT_MODIFIED_HEADER1 == gpt->modified);

	BuildTestGptData(gpt);
	gpt->secondary_header[0]++;
	EXPECT(GPT_SUCCESS == GptSanityCheck(gpt));
	EXPECT(MASK_PRIMARY == gpt->valid_headers);
	EXPECT(MASK_BOTH == gpt->valid_entries);
	GptRepair(gpt);
	EXPECT(GPT_SUCCESS == GptSanityCheck(gpt));
	EXPECT(MASK_BOTH == gpt->valid_headers);
	EXPECT(MASK_BOTH == gpt->valid_entries);
	EXPECT(GPT_MODIFIED_HEADER2 == gpt->modified);

	/*
	 * Modify header1 and update its CRC.  Since header2 is now different
	 * than header1, it'll be the one considered invalid.
	 */
	BuildTestGptData(gpt);
	h1->size++;
	RefreshCrc32(gpt);
	EXPECT(GPT_SUCCESS == GptSanityCheck(gpt));
	EXPECT(MASK_PRIMARY == gpt->valid_headers);
	EXPECT(MASK_BOTH == gpt->valid_entries);
	GptRepair(gpt);
	EXPECT(GPT_SUCCESS == GptSanityCheck(gpt));
	EXPECT(MASK_BOTH == gpt->valid_headers);
	EXPECT(MASK_BOTH == gpt->valid_entries);
	EXPECT(GPT_MODIFIED_HEADER2 == gpt->modified);

	/* Modify entries */
	BuildTestGptData(gpt);
	gpt->primary_entries[0]++;
	gpt->secondary_entries[0]++;
	EXPECT(GPT_ERROR_INVALID_ENTRIES == GptSanityCheck(gpt));
	EXPECT(MASK_BOTH == gpt->valid_headers);
	EXPECT(MASK_NONE == gpt->valid_entries);
	/* Repair can't fix both copies of entries being bad, either. */
	GptRepair(gpt);
	EXPECT(GPT_ERROR_INVALID_ENTRIES == GptSanityCheck(gpt));
	EXPECT(MASK_BOTH == gpt->valid_headers);
	EXPECT(MASK_NONE == gpt->valid_entries);
	EXPECT(0 == gpt->modified);

	BuildTestGptData(gpt);
	gpt->primary_entries[0]++;
	EXPECT(GPT_SUCCESS == GptSanityCheck(gpt));
	EXPECT(MASK_BOTH == gpt->valid_headers);
	EXPECT(MASK_SECONDARY == gpt->valid_entries);
	GptRepair(gpt);
	EXPECT(GPT_SUCCESS == GptSanityCheck(gpt));
	EXPECT(MASK_BOTH == gpt->valid_headers);
	EXPECT(MASK_BOTH == gpt->valid_entries);
	EXPECT(GPT_MODIFIED_ENTRIES1 == gpt->modified);

	BuildTestGptData(gpt);
	gpt->secondary_entries[0]++;
	EXPECT(GPT_SUCCESS == GptSanityCheck(gpt));
	EXPECT(MASK_BOTH == gpt->valid_headers);
	EXPECT(MASK_PRIMARY == gpt->valid_entries);
	GptRepair(gpt);
	EXPECT(GPT_SUCCESS == GptSanityCheck(gpt));
	EXPECT(MASK_BOTH == gpt->valid_headers);
	EXPECT(MASK_BOTH == gpt->valid_entries);
	EXPECT(GPT_MODIFIED_ENTRIES2 == gpt->modified);

	/*
	 * Modify entries and recompute CRCs, then make both primary and
	 * secondary entry pointers use the secondary data.  The primary
	 * header will have the wrong entries CRC, so we should fall back
	 * to the secondary header.
	 */
	BuildTestGptData(gpt);
	e1->starting_lba++;
	RefreshCrc32(gpt);
	tempptr = gpt->primary_entries;
	gpt->primary_entries = gpt->secondary_entries;
	EXPECT(GPT_SUCCESS == GptSanityCheck(gpt));
	EXPECT(MASK_SECONDARY == gpt->valid_headers);
	EXPECT(MASK_BOTH == gpt->valid_entries);
	gpt->primary_entries = tempptr;

	/* Modify both header and entries */
	BuildTestGptData(gpt);
	gpt->primary_header[0]++;
	gpt->primary_entries[0]++;
	EXPECT(GPT_SUCCESS == GptSanityCheck(gpt));
	EXPECT(MASK_SECONDARY == gpt->valid_headers);
	EXPECT(MASK_SECONDARY == gpt->valid_entries);
	GptRepair(gpt);
	EXPECT(GPT_SUCCESS == GptSanityCheck(gpt));
	EXPECT(MASK_BOTH == gpt->valid_headers);
	EXPECT(MASK_BOTH == gpt->valid_entries);
	EXPECT((GPT_MODIFIED_HEADER1 | GPT_MODIFIED_ENTRIES1) == gpt->modified);

	BuildTestGptData(gpt);
	gpt->secondary_header[0]++;
	gpt->secondary_entries[0]++;
	EXPECT(GPT_SUCCESS == GptSanityCheck(gpt));
	EXPECT(MASK_PRIMARY == gpt->valid_headers);
	EXPECT(MASK_PRIMARY == gpt->valid_entries);
	GptRepair(gpt);
	EXPECT(GPT_SUCCESS == GptSanityCheck(gpt));
	EXPECT(MASK_BOTH == gpt->valid_headers);
	EXPECT(MASK_BOTH == gpt->valid_entries);
	EXPECT((GPT_MODIFIED_HEADER2 | GPT_MODIFIED_ENTRIES2) == gpt->modified);

	/* Test cross-correction (h1+e2, h2+e1) */
	BuildTestGptData(gpt);
	gpt->primary_header[0]++;
	gpt->secondary_entries[0]++;
	EXPECT(GPT_SUCCESS == GptSanityCheck(gpt));
	EXPECT(MASK_SECONDARY == gpt->valid_headers);
	EXPECT(MASK_PRIMARY == gpt->valid_entries);
	GptRepair(gpt);
	EXPECT(GPT_SUCCESS == GptSanityCheck(gpt));
	EXPECT(MASK_BOTH == gpt->valid_headers);
	EXPECT(MASK_BOTH == gpt->valid_entries);
	EXPECT((GPT_MODIFIED_HEADER1 | GPT_MODIFIED_ENTRIES2) == gpt->modified);

	BuildTestGptData(gpt);
	gpt->secondary_header[0]++;
	gpt->primary_entries[0]++;
	EXPECT(GPT_SUCCESS == GptSanityCheck(gpt));
	EXPECT(MASK_PRIMARY == gpt->valid_headers);
	EXPECT(MASK_SECONDARY == gpt->valid_entries);
	GptRepair(gpt);
	EXPECT(GPT_SUCCESS == GptSanityCheck(gpt));
	EXPECT(MASK_BOTH == gpt->valid_headers);
	EXPECT(MASK_BOTH == gpt->valid_entries);
	EXPECT((GPT_MODIFIED_HEADER2 | GPT_MODIFIED_ENTRIES1) == gpt->modified);

	/*
	 * Test mismatched pairs (h1+e1 valid, h2+e2 valid but different.  This
	 * simulates a partial update of the drive.
	 */
	BuildTestGptData(gpt);
	gpt->secondary_entries[0]++;
	RefreshCrc32(gpt);
	EXPECT(GPT_SUCCESS == GptSanityCheck(gpt));
	EXPECT(MASK_PRIMARY == gpt->valid_headers);
	EXPECT(MASK_PRIMARY == gpt->valid_entries);
	GptRepair(gpt);
	EXPECT(GPT_SUCCESS == GptSanityCheck(gpt));
	EXPECT(MASK_BOTH == gpt->valid_headers);
	EXPECT(MASK_BOTH == gpt->valid_entries);
	EXPECT((GPT_MODIFIED_HEADER2 | GPT_MODIFIED_ENTRIES2) == gpt->modified);

	/* Test unloaded entry array. */
	gpt = GetEmptyGptData();
	BuildTestGptData(gpt);
	gpt->primary_entries = NULL;
	EXPECT(GPT_SUCCESS == GptSanityCheck(gpt));
	EXPECT(MASK_SECONDARY == gpt->valid_entries);
	gpt = GetEmptyGptData();
	BuildTestGptData(gpt);
	gpt->secondary_entries = NULL;
	EXPECT(GPT_SUCCESS == GptSanityCheck(gpt));
	EXPECT(MASK_PRIMARY == gpt->valid_entries);

	/* Test unloaded header. */
	gpt = GetEmptyGptData();
	BuildTestGptData(gpt);
	gpt->primary_header = NULL;
	EXPECT(GPT_SUCCESS == GptSanityCheck(gpt));
	EXPECT(MASK_SECONDARY == gpt->valid_headers);
	gpt = GetEmptyGptData();
	BuildTestGptData(gpt);
	gpt->secondary_header = NULL;
	EXPECT(GPT_SUCCESS == GptSanityCheck(gpt));
	EXPECT(MASK_PRIMARY == gpt->valid_headers);

	return TEST_OK;
}

static int EntryAttributeGetSetTest(void)
{
	GptData *gpt = GetEmptyGptData();
	GptEntry *e = (GptEntry *)(gpt->primary_entries);

	e->attrs.whole = 0x0000000000000000ULL;
	SetEntrySuccessful(e, 1);
	EXPECT(0x0100000000000000ULL == e->attrs.whole);
	EXPECT(1 == GetEntrySuccessful(e));
	e->attrs.whole = 0xFFFFFFFFFFFFFFFFULL;
	SetEntrySuccessful(e, 0);
	EXPECT(0xFEFFFFFFFFFFFFFFULL == e->attrs.whole);
	EXPECT(0 == GetEntrySuccessful(e));

	e->attrs.whole = 0x0000000000000000ULL;
	SetEntryTries(e, 15);
	EXPECT(15 == GetEntryTries(e));
	EXPECT(0x00F0000000000000ULL == e->attrs.whole);
	e->attrs.whole = 0xFFFFFFFFFFFFFFFFULL;
	SetEntryTries(e, 0);
	EXPECT(0xFF0FFFFFFFFFFFFFULL == e->attrs.whole);
	EXPECT(0 == GetEntryTries(e));

	e->attrs.whole = 0x0000000000000000ULL;
	SetEntryPriority(e, 15);
	EXPECT(0x000F000000000000ULL == e->attrs.whole);
	EXPECT(15 == GetEntryPriority(e));
	e->attrs.whole = 0xFFFFFFFFFFFFFFFFULL;
	SetEntryPriority(e, 0);
	EXPECT(0xFFF0FFFFFFFFFFFFULL == e->attrs.whole);
	EXPECT(0 == GetEntryPriority(e));

	e->attrs.whole = 0xFFFFFFFFFFFFFFFFULL;
	EXPECT(1 == GetEntrySuccessful(e));
	EXPECT(15 == GetEntryPriority(e));
	EXPECT(15 == GetEntryTries(e));

	e->attrs.whole = 0x0123000000000000ULL;
	EXPECT(1 == GetEntrySuccessful(e));
	EXPECT(2 == GetEntryTries(e));
	EXPECT(3 == GetEntryPriority(e));

	return TEST_OK;
}

static int EntryTypeTest(void)
{
	GptData *gpt = GetEmptyGptData();
	GptEntry *e = (GptEntry *)(gpt->primary_entries);

	Memcpy(&e->type, &guid_zero, sizeof(Guid));
	EXPECT(1 == IsUnusedEntry(e));
	EXPECT(0 == IsKernelEntry(e));

	Memcpy(&e->type, &guid_kernel, sizeof(Guid));
	EXPECT(0 == IsUnusedEntry(e));
	EXPECT(1 == IsKernelEntry(e));

	Memcpy(&e->type, &guid_rootfs, sizeof(Guid));
	EXPECT(0 == IsUnusedEntry(e));
	EXPECT(0 == IsKernelEntry(e));

	return TEST_OK;
}

/* Make an entry unused by clearing its type. */
static void FreeEntry(GptEntry *e)
{
	Memset(&e->type, 0, sizeof(Guid));
}

/* Set up an entry. */
static void FillEntry(GptEntry *e, int is_kernel,
                      int priority, int successful, int tries)
{
	Memcpy(&e->type, (is_kernel ? &guid_kernel : &guid_zero), sizeof(Guid));
	SetEntryPriority(e, priority);
	SetEntrySuccessful(e, successful);
	SetEntryTries(e, tries);
}

/*
 * Invalidate all kernel entries and expect GptNextKernelEntry() cannot find
 * any usable kernel entry.
 */
static int NoValidKernelEntryTest(void)
{
	GptData *gpt = GetEmptyGptData();
	GptEntry *e1 = (GptEntry *)(gpt->primary_entries);

	BuildTestGptData(gpt);
	SetEntryPriority(e1 + KERNEL_A, 0);
	FreeEntry(e1 + KERNEL_B);
	RefreshCrc32(gpt);
	EXPECT(GPT_ERROR_NO_VALID_KERNEL ==
	       GptNextKernelEntry(gpt, NULL, NULL));

	return TEST_OK;
}

static int GetNextNormalTest(void)
{
	GptData *gpt = GetEmptyGptData();
	GptEntry *e1 = (GptEntry *)(gpt->primary_entries);
	uint64_t start, size;

	/* Normal case - both kernels successful */
	BuildTestGptData(gpt);
	FillEntry(e1 + KERNEL_A, 1, 2, 1, 0);
	FillEntry(e1 + KERNEL_B, 1, 2, 1, 0);
	RefreshCrc32(gpt);
	GptInit(gpt);

	EXPECT(GPT_SUCCESS == GptNextKernelEntry(gpt, &start, &size));
	EXPECT(KERNEL_A == gpt->current_kernel);
	EXPECT(34 == start);
	EXPECT(100 == size);

	EXPECT(GPT_SUCCESS == GptNextKernelEntry(gpt, &start, &size));
	EXPECT(KERNEL_B == gpt->current_kernel);
	EXPECT(134 == start);
	EXPECT(99 == size);

	EXPECT(GPT_ERROR_NO_VALID_KERNEL ==
	       GptNextKernelEntry(gpt, &start, &size));
	EXPECT(-1 == gpt->current_kernel);

	/* Call as many times as you want; you won't get another kernel... */
	EXPECT(GPT_ERROR_NO_VALID_KERNEL ==
	       GptNextKernelEntry(gpt, &start, &size));
	EXPECT(-1 == gpt->current_kernel);

	return TEST_OK;
}

static int GetNextPrioTest(void)
{
	GptData *gpt = GetEmptyGptData();
	GptEntry *e1 = (GptEntry *)(gpt->primary_entries);
	uint64_t start, size;

	/* Priority 3, 4, 0, 4 - should boot order B, Y, A */
	BuildTestGptData(gpt);
	FillEntry(e1 + KERNEL_A, 1, 3, 1, 0);
	FillEntry(e1 + KERNEL_B, 1, 4, 1, 0);
	FillEntry(e1 + KERNEL_X, 1, 0, 1, 0);
	FillEntry(e1 + KERNEL_Y, 1, 4, 1, 0);
	RefreshCrc32(gpt);
	GptInit(gpt);

	EXPECT(GPT_SUCCESS == GptNextKernelEntry(gpt, &start, &size));
	EXPECT(KERNEL_B == gpt->current_kernel);
	EXPECT(GPT_SUCCESS == GptNextKernelEntry(gpt, &start, &size));
	EXPECT(KERNEL_Y == gpt->current_kernel);
	EXPECT(GPT_SUCCESS == GptNextKernelEntry(gpt, &start, &size));
	EXPECT(KERNEL_A == gpt->current_kernel);
	EXPECT(GPT_ERROR_NO_VALID_KERNEL ==
	       GptNextKernelEntry(gpt, &start, &size));

	return TEST_OK;
}

static int GetNextTriesTest(void)
{
	GptData *gpt = GetEmptyGptData();
	GptEntry *e1 = (GptEntry *)(gpt->primary_entries);
	uint64_t start, size;

	/* Tries=nonzero is attempted just like success, but tries=0 isn't */
	BuildTestGptData(gpt);
	FillEntry(e1 + KERNEL_A, 1, 2, 1, 0);
	FillEntry(e1 + KERNEL_B, 1, 3, 0, 0);
	FillEntry(e1 + KERNEL_X, 1, 4, 0, 1);
	FillEntry(e1 + KERNEL_Y, 1, 0, 0, 5);
	RefreshCrc32(gpt);
	GptInit(gpt);

	EXPECT(GPT_SUCCESS == GptNextKernelEntry(gpt, &start, &size));
	EXPECT(KERNEL_X == gpt->current_kernel);
	EXPECT(GPT_SUCCESS == GptNextKernelEntry(gpt, &start, &size));
	EXPECT(KERNEL_A == gpt->current_kernel);
	EXPECT(GPT_ERROR_NO_VALID_KERNEL ==
	       GptNextKernelEntry(gpt, &start, &size));

	return TEST_OK;
}

static int GptUpdateTest(void)
{
	GptData *gpt = GetEmptyGptData();
	GptEntry *e = (GptEntry *)(gpt->primary_entries);
	GptEntry *e2 = (GptEntry *)(gpt->secondary_entries);
	uint64_t start, size;

	/* Tries=nonzero is attempted just like success, but tries=0 isn't */
	BuildTestGptData(gpt);
	FillEntry(e + KERNEL_A, 1, 4, 1, 0);
	FillEntry(e + KERNEL_B, 1, 3, 0, 2);
	FillEntry(e + KERNEL_X, 1, 2, 0, 2);
	RefreshCrc32(gpt);
	GptInit(gpt);
	gpt->modified = 0;  /* Nothing modified yet */

	/* Successful kernel */
	EXPECT(GPT_SUCCESS == GptNextKernelEntry(gpt, &start, &size));
	EXPECT(KERNEL_A == gpt->current_kernel);
	EXPECT(1 == GetEntrySuccessful(e + KERNEL_A));
	EXPECT(4 == GetEntryPriority(e + KERNEL_A));
	EXPECT(0 == GetEntryTries(e + KERNEL_A));
	EXPECT(1 == GetEntrySuccessful(e2 + KERNEL_A));
	EXPECT(4 == GetEntryPriority(e2 + KERNEL_A));
	EXPECT(0 == GetEntryTries(e2 + KERNEL_A));
	/* Trying successful kernel changes nothing */
	EXPECT(GPT_SUCCESS == GptUpdateKernelEntry(gpt, GPT_UPDATE_ENTRY_TRY));
	EXPECT(1 == GetEntrySuccessful(e + KERNEL_A));
	EXPECT(4 == GetEntryPriority(e + KERNEL_A));
	EXPECT(0 == GetEntryTries(e + KERNEL_A));
	EXPECT(0 == gpt->modified);
	/* Marking it bad also does not update it. */
	EXPECT(GPT_SUCCESS == GptUpdateKernelEntry(gpt, GPT_UPDATE_ENTRY_BAD));
	EXPECT(1 == GetEntrySuccessful(e + KERNEL_A));
	EXPECT(4 == GetEntryPriority(e + KERNEL_A));
	EXPECT(0 == GetEntryTries(e + KERNEL_A));
	EXPECT(0 == gpt->modified);

	/* Kernel with tries */
	EXPECT(GPT_SUCCESS == GptNextKernelEntry(gpt, &start, &size));
	EXPECT(KERNEL_B == gpt->current_kernel);
	EXPECT(0 == GetEntrySuccessful(e + KERNEL_B));
	EXPECT(3 == GetEntryPriority(e + KERNEL_B));
	EXPECT(2 == GetEntryTries(e + KERNEL_B));
	/* Marking it bad clears it */
	EXPECT(GPT_SUCCESS == GptUpdateKernelEntry(gpt, GPT_UPDATE_ENTRY_BAD));
	EXPECT(0 == GetEntrySuccessful(e + KERNEL_B));
	EXPECT(0 == GetEntryPriority(e + KERNEL_B));
	EXPECT(0 == GetEntryTries(e + KERNEL_B));
	/* Which affects both copies of the partition entries */
	EXPECT(0 == GetEntrySuccessful(e2 + KERNEL_B));
	EXPECT(0 == GetEntryPriority(e2 + KERNEL_B));
	EXPECT(0 == GetEntryTries(e2 + KERNEL_B));
	/* And that's caused the GPT to need updating */
	EXPECT(0x0F == gpt->modified);

	/* Another kernel with tries */
	EXPECT(GPT_SUCCESS == GptNextKernelEntry(gpt, &start, &size));
	EXPECT(KERNEL_X == gpt->current_kernel);
	EXPECT(0 == GetEntrySuccessful(e + KERNEL_X));
	EXPECT(2 == GetEntryPriority(e + KERNEL_X));
	EXPECT(2 == GetEntryTries(e + KERNEL_X));
	/* Trying it uses up a try */
	EXPECT(GPT_SUCCESS == GptUpdateKernelEntry(gpt, GPT_UPDATE_ENTRY_TRY));
	EXPECT(0 == GetEntrySuccessful(e + KERNEL_X));
	EXPECT(2 == GetEntryPriority(e + KERNEL_X));
	EXPECT(1 == GetEntryTries(e + KERNEL_X));
	EXPECT(0 == GetEntrySuccessful(e2 + KERNEL_X));
	EXPECT(2 == GetEntryPriority(e2 + KERNEL_X));
	EXPECT(1 == GetEntryTries(e2 + KERNEL_X));
	/* Trying it again marks it inactive */
	EXPECT(GPT_SUCCESS == GptUpdateKernelEntry(gpt, GPT_UPDATE_ENTRY_TRY));
	EXPECT(0 == GetEntrySuccessful(e + KERNEL_X));
	EXPECT(0 == GetEntryPriority(e + KERNEL_X));
	EXPECT(0 == GetEntryTries(e + KERNEL_X));

	/* Can't update if entry isn't a kernel, or there isn't an entry */
	Memcpy(&e[KERNEL_X].type, &guid_rootfs, sizeof(guid_rootfs));
	EXPECT(GPT_ERROR_INVALID_UPDATE_TYPE ==
	       GptUpdateKernelEntry(gpt, GPT_UPDATE_ENTRY_BAD));
	gpt->current_kernel = CGPT_KERNEL_ENTRY_NOT_FOUND;
	EXPECT(GPT_ERROR_INVALID_UPDATE_TYPE ==
	       GptUpdateKernelEntry(gpt, GPT_UPDATE_ENTRY_BAD));


	return TEST_OK;
}

/*
 * Give an invalid kernel type, and expect GptUpdateKernelEntry() returns
 * GPT_ERROR_INVALID_UPDATE_TYPE.
 */
static int UpdateInvalidKernelTypeTest(void)
{
	GptData *gpt = GetEmptyGptData();

	BuildTestGptData(gpt);
	/* anything, but not CGPT_KERNEL_ENTRY_NOT_FOUND */
	gpt->current_kernel = 0;
	/* any invalid update_type value */
	EXPECT(GPT_ERROR_INVALID_UPDATE_TYPE ==
	       GptUpdateKernelEntry(gpt, 99));

	return TEST_OK;
}

/* Test duplicate UniqueGuids can be detected. */
static int DuplicateUniqueGuidTest(void)
{
	GptData *gpt = GetEmptyGptData();
	GptHeader *h = (GptHeader *)gpt->primary_header;
	GptEntry *e = (GptEntry *)gpt->primary_entries;
	int i, j;

	struct {
		int duplicate;
		struct {
			uint64_t starting_lba;
			uint64_t ending_lba;
			uint32_t type_guid;
			uint32_t unique_guid;
		} entries[16];   /* enough for testing. */
	} cases[] = {
		{GPT_SUCCESS, {{100, 109, 1, 1},
			       {110, 119, 2, 2},
			       {120, 129, 3, 3},
			       {130, 139, 4, 4},
			}},
		{GPT_SUCCESS, {{100, 109, 1, 1},
			       {110, 119, 1, 2},
			       {120, 129, 2, 3},
			       {130, 139, 2, 4},
			}},
		{GPT_ERROR_DUP_GUID, {{100, 109, 1, 1},
				      {110, 119, 2, 2},
				      {120, 129, 3, 1},
				      {130, 139, 4, 4},
			}},
		{GPT_ERROR_DUP_GUID, {{100, 109, 1, 1},
				      {110, 119, 1, 2},
				      {120, 129, 2, 3},
				      {130, 139, 2, 2},
			}},
	};

	for (i = 0; i < ARRAY_SIZE(cases); ++i) {
		BuildTestGptData(gpt);
		ZeroEntries(gpt);
		for(j = 0; j < ARRAY_SIZE(cases[0].entries); ++j) {
			if (!cases[i].entries[j].starting_lba)
				break;

			e[j].starting_lba = cases[i].entries[j].starting_lba;
			e[j].ending_lba = cases[i].entries[j].ending_lba;
			SetGuid(&e[j].type, cases[i].entries[j].type_guid);
			SetGuid(&e[j].unique, cases[i].entries[j].unique_guid);
		}
		RefreshCrc32(gpt);

		EXPECT(cases[i].duplicate == CheckEntries(e, h));
	}

	return TEST_OK;
}

/* Test getting the current kernel GUID */
static int GetKernelGuidTest(void)
{
	GptData *gpt = GetEmptyGptData();
	GptEntry *e = (GptEntry *)gpt->primary_entries;
	Guid g;

	BuildTestGptData(gpt);
	gpt->current_kernel = 0;
	GetCurrentKernelUniqueGuid(gpt, &g);
	EXPECT(!Memcmp(&g, &e[0].unique, sizeof(Guid)));
	gpt->current_kernel = 1;
	GetCurrentKernelUniqueGuid(gpt, &g);
	EXPECT(!Memcmp(&g, &e[1].unique, sizeof(Guid)));

	return TEST_OK;
}

/* Test getting GPT error text strings */
static int ErrorTextTest(void)
{
	int i;

	/* Known errors are not unknown */
	for (i = 0; i < GPT_ERROR_COUNT; i++) {
		EXPECT(GptErrorText(i));
		EXPECT(strcmp(GptErrorText(i), "Unknown"));
	}

	/* But other error values are */
	EXPECT(!strcmp(GptErrorText(GPT_ERROR_COUNT), "Unknown"));

	return TEST_OK;
}

static int CheckHeaderOffDevice()
{
	GptData* gpt = GetEmptyGptData();
	BuildTestGptData(gpt);

	GptHeader* primary_header = (GptHeader*)gpt->primary_header;
	primary_header->first_usable_lba = 0;
	RefreshCrc32(gpt);
	// GPT is stored on the same device so first usable lba should not
	// start at 0.
	EXPECT(1 == CheckHeader(primary_header, 0, gpt->streaming_drive_sectors,
		gpt->gpt_drive_sectors, 0));
	// But off device, it is okay to accept this GPT header.
	EXPECT(0 == CheckHeader(primary_header, 0, gpt->streaming_drive_sectors,
		gpt->gpt_drive_sectors, GPT_FLAG_EXTERNAL));

	BuildTestGptData(gpt);
	primary_header->number_of_entries = 100;
	RefreshCrc32(gpt);
	// Normally, number of entries is 128. So this should fail.
	EXPECT(1 == CheckHeader(primary_header, 0, gpt->streaming_drive_sectors,
		gpt->gpt_drive_sectors, 0));
	// But off device, it is okay.
	EXPECT(0 == CheckHeader(primary_header, 0, gpt->streaming_drive_sectors,
		gpt->gpt_drive_sectors, GPT_FLAG_EXTERNAL));

	primary_header->number_of_entries = MIN_NUMBER_OF_ENTRIES - 1;
	RefreshCrc32(gpt);
	// However, too few entries is not good.
	EXPECT(1 == CheckHeader(primary_header, 0, gpt->streaming_drive_sectors,
		gpt->gpt_drive_sectors, GPT_FLAG_EXTERNAL));

	// Repeat for secondary header.
	BuildTestGptData(gpt);
	GptHeader* secondary_header = (GptHeader*)gpt->secondary_header;
	secondary_header->first_usable_lba = 0;
	RefreshCrc32(gpt);
	EXPECT(1 == CheckHeader(secondary_header, 1, gpt->streaming_drive_sectors,
		gpt->gpt_drive_sectors, 0));
	EXPECT(0 == CheckHeader(secondary_header, 1, gpt->streaming_drive_sectors,
		gpt->gpt_drive_sectors, GPT_FLAG_EXTERNAL));

	BuildTestGptData(gpt);
	secondary_header->number_of_entries = 100;
	/* Because we change number of entries, we need to also update entrie_lba. */
	secondary_header->entries_lba = secondary_header->my_lba -
		CalculateEntriesSectors(secondary_header);
	RefreshCrc32(gpt);
	EXPECT(1 == CheckHeader(secondary_header, 1, gpt->streaming_drive_sectors,
		gpt->gpt_drive_sectors, 0));
	EXPECT(0 == CheckHeader(secondary_header, 1, gpt->streaming_drive_sectors,
		gpt->gpt_drive_sectors, GPT_FLAG_EXTERNAL));

	secondary_header->number_of_entries = MIN_NUMBER_OF_ENTRIES - 1;
	RefreshCrc32(gpt);
	EXPECT(1 == CheckHeader(secondary_header, 1, gpt->streaming_drive_sectors,
		gpt->gpt_drive_sectors, GPT_FLAG_EXTERNAL));

	return TEST_OK;
}

int main(int argc, char *argv[])
{
	int i;
	int error_count = 0;
	struct {
		char *name;
		test_func fp;
		int retval;
	} test_cases[] = {
		{ TEST_CASE(StructSizeTest), },
		{ TEST_CASE(TestBuildTestGptData), },
		{ TEST_CASE(ParameterTests), },
		{ TEST_CASE(HeaderCrcTest), },
		{ TEST_CASE(HeaderSameTest), },
		{ TEST_CASE(SignatureTest), },
		{ TEST_CASE(RevisionTest), },
		{ TEST_CASE(SizeTest), },
		{ TEST_CASE(CrcFieldTest), },
		{ TEST_CASE(ReservedFieldsTest), },
		{ TEST_CASE(SizeOfPartitionEntryTest), },
		{ TEST_CASE(NumberOfPartitionEntriesTest), },
		{ TEST_CASE(MyLbaTest), },
		{ TEST_CASE(FirstUsableLbaAndLastUsableLbaTest), },
		{ TEST_CASE(EntriesCrcTest), },
		{ TEST_CASE(ValidEntryTest), },
		{ TEST_CASE(OverlappedPartitionTest), },
		{ TEST_CASE(SanityCheckTest), },
		{ TEST_CASE(NoValidKernelEntryTest), },
		{ TEST_CASE(EntryAttributeGetSetTest), },
		{ TEST_CASE(EntryTypeTest), },
		{ TEST_CASE(GetNextNormalTest), },
		{ TEST_CASE(GetNextPrioTest), },
		{ TEST_CASE(GetNextTriesTest), },
		{ TEST_CASE(GptUpdateTest), },
		{ TEST_CASE(UpdateInvalidKernelTypeTest), },
		{ TEST_CASE(DuplicateUniqueGuidTest), },
		{ TEST_CASE(TestCrc32TestVectors), },
		{ TEST_CASE(GetKernelGuidTest), },
		{ TEST_CASE(ErrorTextTest), },
		{ TEST_CASE(CheckHeaderOffDevice), },
	};

	for (i = 0; i < sizeof(test_cases)/sizeof(test_cases[0]); ++i) {
		printf("Running %s() ...\n", test_cases[i].name);
		test_cases[i].retval = test_cases[i].fp();
		if (test_cases[i].retval) {
			printf(COL_RED "[ERROR]\n\n" COL_STOP);
			++error_count;
		} else {
			printf(COL_GREEN "[PASS]\n\n" COL_STOP);
		}
	}

	if (error_count) {
		printf("\n------------------------------------------------\n");
		printf(COL_RED "The following %d test cases are failed:\n"
		       COL_STOP, error_count);
		for (i = 0; i < sizeof(test_cases)/sizeof(test_cases[0]); ++i) {
			if (test_cases[i].retval)
				printf("  %s()\n", test_cases[i].name);
		}
	}

	return error_count ? 1 : 0;
}
