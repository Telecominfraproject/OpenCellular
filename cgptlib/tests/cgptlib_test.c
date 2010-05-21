/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "cgptlib_test.h"
#include <string.h>
#include "cgptlib.h"
#include "cgptlib_internal.h"
#include "crc32.h"
#include "crc32_test.h"
#include "gpt.h"
#include "quick_sort_test.h"
#include "utility.h"

/* Testing partition layout (sector_bytes=512)
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
#define ROOTFS_A 1
#define ROOTFS_B 2
#define KERNEL_B 3

#define DEFAULT_SECTOR_SIZE 512
#define MAX_SECTOR_SIZE 4096
#define DEFAULT_DRIVE_SECTORS 467
#define PARTITION_ENTRIES_SIZE TOTAL_ENTRIES_SIZE /* 16384 */

/* Given a GptData pointer, first re-calculate entries CRC32 value,
 * then reset header CRC32 value to 0, and calculate header CRC32 value.
 * Both primary and secondary are updated. */
void RefreshCrc32(GptData *gpt) {
  GptHeader *header, *header2;
  GptEntry *entries, *entries2;

  header = (GptHeader*)gpt->primary_header;
  entries = (GptEntry*)gpt->primary_entries;
  header2 = (GptHeader*)gpt->secondary_header;
  entries2 = (GptEntry*)gpt->secondary_entries;

  header->entries_crc32 =
      Crc32((uint8_t*)entries,
            header->number_of_entries * header->size_of_entry);
  header->header_crc32 = 0;
  header->header_crc32 = Crc32((uint8_t*)header, header->size);
  header2->entries_crc32 =
      Crc32((uint8_t*)entries2,
            header2->number_of_entries * header2->size_of_entry);
  header2->header_crc32 = 0;
  header2->header_crc32 = Crc32((uint8_t*)header2, header2->size);
}

void ZeroHeaders(GptData* gpt) {
  Memset(gpt->primary_header, 0, MAX_SECTOR_SIZE);
  Memset(gpt->secondary_header, 0, MAX_SECTOR_SIZE);
}

void ZeroEntries(GptData* gpt) {
  Memset(gpt->primary_entries, 0, PARTITION_ENTRIES_SIZE);
  Memset(gpt->secondary_entries, 0, PARTITION_ENTRIES_SIZE);
}

void ZeroHeadersEntries(GptData* gpt) {
  ZeroHeaders(gpt);
  ZeroEntries(gpt);
}

/* Returns a pointer to a static GptData instance (no free is required).
 * All fields are zero except 4 pointers linking to header and entries.
 * All content of headers and entries are zero. */
GptData* GetEmptyGptData() {
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

/* Fills in most of fields and creates the layout described in the top of this
 * file. Before calling this function, primary/secondary header/entries must
 * have been pointed to the buffer, say, a gpt returned from GetEmptyGptData().
 * This function returns a good (valid) copy of GPT layout described in top of
 * this file. */
void BuildTestGptData(GptData *gpt) {
  GptHeader *header, *header2;
  GptEntry *entries, *entries2;
  Guid chromeos_kernel = GPT_ENT_TYPE_CHROMEOS_KERNEL;
  Guid chromeos_rootfs = GPT_ENT_TYPE_CHROMEOS_ROOTFS;

  gpt->sector_bytes = DEFAULT_SECTOR_SIZE;
  gpt->drive_sectors = DEFAULT_DRIVE_SECTORS;
  gpt->current_kernel = CGPT_KERNEL_ENTRY_NOT_FOUND;

  /* build primary */
  header = (GptHeader*)gpt->primary_header;
  entries = (GptEntry*)gpt->primary_entries;
  Memcpy(header->signature, GPT_HEADER_SIGNATURE, sizeof(GPT_HEADER_SIGNATURE));
  header->revision = GPT_HEADER_REVISION;
  header->size = sizeof(GptHeader) - sizeof(header->padding);
  header->reserved = 0;
  header->my_lba = 1;
  header->first_usable_lba = 34;
  header->last_usable_lba = DEFAULT_DRIVE_SECTORS - 1 - 32 - 1;  /* 433 */
  header->entries_lba = 2;
  header->number_of_entries = 128;  /* 512B / 128B * 32sectors = 128 entries */
  header->size_of_entry = 128;  /* bytes */
  Memcpy(&entries[0].type, &chromeos_kernel, sizeof(chromeos_kernel));
  entries[0].starting_lba = 34;
  entries[0].ending_lba = 133;
  Memcpy(&entries[1].type, &chromeos_rootfs, sizeof(chromeos_rootfs));
  entries[1].starting_lba = 134;
  entries[1].ending_lba = 233;
  Memcpy(&entries[2].type, &chromeos_rootfs, sizeof(chromeos_rootfs));
  entries[2].starting_lba = 234;
  entries[2].ending_lba = 333;
  Memcpy(&entries[3].type, &chromeos_kernel, sizeof(chromeos_kernel));
  entries[3].starting_lba = 334;
  entries[3].ending_lba = 433;
  header->padding = 0;

  /* build secondary */
  header2 = (GptHeader*)gpt->secondary_header;
  entries2 = (GptEntry*)gpt->secondary_entries;
  Memcpy(header2, header, sizeof(GptHeader));
  Memcpy(entries2, entries, PARTITION_ENTRIES_SIZE);
  header2->my_lba = DEFAULT_DRIVE_SECTORS - 1;  /* 466 */
  header2->entries_lba = DEFAULT_DRIVE_SECTORS - 1 - 32;  /* 434 */

  RefreshCrc32(gpt);
}

/* Dumps memory starting from [vp] with [len] bytes.
 * Prints [memo] if not NULL. Example output:
 *
 *  00 01 02 03 04 05 06 07 - 08 09 0a 0b 0c 0d 0e 0f
 *  10 11 12 13 14 15 16 17 - 18 19 1a 1b 1c 1d 1e 1f
 *  ...
 */
static void Dump(void *vp, int len, char* memo) {
  uint8_t *start = vp;
  int i;
  if (memo) printf("--[%s]----------\n", memo);
  for (i = 0; i < len; ++i) {
    printf("%02x%s", start[i],
                     (!(~i & 15) ? "\n" :
                      !(~i & 7) ? " - ": " "));
  }
  if (i&15) printf("\n");
}

/* More formatted dump with GptData. */
void DumpGptData(GptData *gpt) {
  printf("DumpGptData(%p)...\n", gpt);
  Dump(gpt, sizeof(gpt), NULL);
  Dump(gpt->primary_header, sizeof(GptHeader), "Primary header");
  Dump(gpt->primary_entries, sizeof(GptEntry) * 8, "Primary entries");
  Dump(gpt->secondary_header, sizeof(GptHeader), "Secondary header");
  Dump(gpt->secondary_entries, sizeof(GptEntry) * 8,
       "Secondary entries");
}

/* Tests if the default structure returned by BuildTestGptData() is good. */
int TestBuildTestGptData() {
  GptData *gpt;
  gpt = GetEmptyGptData();
  BuildTestGptData(gpt);
  EXPECT(GPT_SUCCESS == GptInit(gpt));
  return TEST_OK;
}

/* Tests if wrong sector_bytes or drive_sectors is detected by GptInit().
 * Currently we only support 512 bytes per sector.
 * In the future, we may support other sizes.
 * A too small drive_sectors should be rejected by GptInit(). */
int ParameterTests() {
  GptData *gpt;
  struct {
    uint32_t sector_bytes;
    uint64_t drive_sectors;
    int expected_retval;
  } cases[] = {
    {512, DEFAULT_DRIVE_SECTORS, GPT_SUCCESS},
    {520, DEFAULT_DRIVE_SECTORS, GPT_ERROR_INVALID_SECTOR_SIZE},
    {512, 0, GPT_ERROR_INVALID_SECTOR_NUMBER},
    {512, 66, GPT_ERROR_INVALID_SECTOR_NUMBER},
    {512, GPT_PMBR_SECTOR + GPT_HEADER_SECTOR * 2 + GPT_ENTRIES_SECTORS * 2,
          GPT_SUCCESS},
    {4096, DEFAULT_DRIVE_SECTORS, GPT_ERROR_INVALID_SECTOR_SIZE},
  };
  int i;

  gpt = GetEmptyGptData();
  for (i = 0; i < ARRAY_SIZE(cases); ++i) {
    BuildTestGptData(gpt);
    gpt->sector_bytes = cases[i].sector_bytes;
    gpt->drive_sectors = cases[i].drive_sectors;
    EXPECT(cases[i].expected_retval == CheckParameters(gpt));
  }

  return TEST_OK;
}

/* Tests if signature ("EFI PART") is checked. */
int SignatureTest() {
  int i;
  GptData *gpt;
  int test_mask;
  GptHeader *headers[2];

  gpt = GetEmptyGptData();
  headers[PRIMARY] = (GptHeader*)gpt->primary_header;
  headers[SECONDARY] = (GptHeader*)gpt->secondary_header;

  for (test_mask = MASK_PRIMARY; test_mask <= MASK_BOTH; ++test_mask) {
    for (i = 0; i < 8; ++i) {
      BuildTestGptData(gpt);
      if (test_mask & MASK_PRIMARY)
        headers[PRIMARY]->signature[i] ^= 0xff;
      if (test_mask & MASK_SECONDARY)
        headers[SECONDARY]->signature[i] ^= 0xff;
      EXPECT((MASK_BOTH ^ test_mask) == CheckHeaderSignature(gpt));
    }
  }

  return TEST_OK;
}

/* The revision we currently support is GPT_HEADER_REVISION.
 * If the revision in header is not that, we expect the header is invalid. */
int RevisionTest() {
  GptData *gpt;
  struct {
    uint32_t value_to_test;
    int is_valid_value;
  } cases[] = {
    {0x01000000, 0},
    {0x00010000, 1},  /* GPT_HEADER_REVISION */
    {0x00000100, 0},
    {0x00000001, 0},
    {0x23010456, 0},
  };
  int i;
  int test_mask;
  GptHeader *headers[2];
  uint32_t valid_headers;

  gpt = GetEmptyGptData();
  headers[PRIMARY] = (GptHeader*)gpt->primary_header;
  headers[SECONDARY] = (GptHeader*)gpt->secondary_header;

  for (i = 0; i < ARRAY_SIZE(cases); ++i) {
    for (test_mask = MASK_PRIMARY; test_mask <= MASK_BOTH; ++test_mask) {
      BuildTestGptData(gpt);
      if (test_mask & MASK_PRIMARY)
        headers[PRIMARY]->revision = cases[i].value_to_test;
      if (test_mask & MASK_SECONDARY)
        headers[SECONDARY]->revision = cases[i].value_to_test;
      valid_headers = CheckRevision(gpt);
      if (cases[i].is_valid_value)
        EXPECT(MASK_BOTH == valid_headers);
      else
        EXPECT((MASK_BOTH ^ test_mask) == valid_headers);
    }
  }
  return TEST_OK;
}

int SizeTest() {
  GptData *gpt;
  struct {
    uint32_t value_to_test;
    int is_valid_value;
  } cases[] = {
    {91, 0},
    {92, 1},
    {93, 1},
    {511, 1},
    {512, 1},
    {513, 0},
  };
  int i;
  int test_mask;
  GptHeader *headers[2];
  uint32_t valid_headers;

  gpt = GetEmptyGptData();
  headers[PRIMARY] = (GptHeader*)gpt->primary_header;
  headers[SECONDARY] = (GptHeader*)gpt->secondary_header;

  for (i = 0; i < ARRAY_SIZE(cases); ++i) {
    for (test_mask = MASK_PRIMARY; test_mask <= MASK_BOTH; ++test_mask) {
      BuildTestGptData(gpt);
      if (test_mask & MASK_PRIMARY)
        headers[PRIMARY]->size = cases[i].value_to_test;
      if (test_mask & MASK_SECONDARY)
        headers[SECONDARY]->size = cases[i].value_to_test;
      valid_headers = CheckSize(gpt);
      if (cases[i].is_valid_value)
        EXPECT(MASK_BOTH == valid_headers);
      else
        EXPECT((MASK_BOTH ^ test_mask) == valid_headers);
    }
  }
  return TEST_OK;
}

/* Tests if reserved fields are checked.
 * We'll try non-zero values to test. */
int ReservedFieldsTest() {
  GptData *gpt;
  GptHeader *primary_header, *secondary_header;

  gpt = GetEmptyGptData();
  primary_header = (GptHeader*)gpt->primary_header;
  secondary_header = (GptHeader*)gpt->secondary_header;

  /* expect secondary is still valid. */
  BuildTestGptData(gpt);
  primary_header->reserved ^= 0x12345678;  /* whatever random */
  EXPECT(MASK_SECONDARY == CheckReservedFields(gpt));

  /* expect secondary is still valid. */
  BuildTestGptData(gpt);
  primary_header->padding ^= 0x12345678;  /* whatever random */
  EXPECT(MASK_SECONDARY == CheckReservedFields(gpt));

  /* expect primary is still valid. */
  BuildTestGptData(gpt);
  secondary_header->reserved ^= 0x12345678;  /* whatever random */
  EXPECT(MASK_PRIMARY == CheckReservedFields(gpt));

  /* expect primary is still valid. */
  BuildTestGptData(gpt);
  secondary_header->padding ^= 0x12345678;  /* whatever random */
  EXPECT(MASK_PRIMARY == CheckReservedFields(gpt));

  return TEST_OK;
}

/* Tests if myLBA field is checked (1 for primary, last for secondary). */
int MyLbaTest() {
  GptData *gpt;
  int test_mask;
  GptHeader *headers[2];
  uint32_t valid_headers;

  gpt = GetEmptyGptData();
  headers[PRIMARY] = (GptHeader*)gpt->primary_header;
  headers[SECONDARY] = (GptHeader*)gpt->secondary_header;

  for (test_mask = MASK_PRIMARY; test_mask <= MASK_BOTH; ++test_mask) {
    BuildTestGptData(gpt);
    if (test_mask & MASK_PRIMARY)
      ++headers[PRIMARY]->my_lba;
    if (test_mask & MASK_SECONDARY)
      --headers[SECONDARY]->my_lba;
    valid_headers = CheckMyLba(gpt);
    EXPECT((MASK_BOTH ^ test_mask) == valid_headers);
  }
  return TEST_OK;
}

/* Tests if SizeOfPartitionEntry is checked. SizeOfPartitionEntry must be
 * between 128 and 512, and a multiple of 8. */
int SizeOfPartitionEntryTest() {
  GptData *gpt;
  struct {
    uint32_t value_to_test;
    int is_valid_value;
  } cases[] = {
    {127, 0},
    {128, 1},
    {129, 0},
    {130, 0},
    {131, 0},
    {132, 0},
    {133, 0},
    {134, 0},
    {135, 0},
    {136, 1},
    {144, 1},
    {160, 1},
    {192, 1},
    {256, 1},
    {384, 1},
    {504, 1},
    {512, 1},
    {513, 0},
    {520, 0},
  };
  int i;
  int test_mask;
  GptHeader *headers[2];
  uint32_t valid_headers;

  gpt = GetEmptyGptData();
  headers[PRIMARY] = (GptHeader*)gpt->primary_header;
  headers[SECONDARY] = (GptHeader*)gpt->secondary_header;

  for (i = 0; i < ARRAY_SIZE(cases); ++i) {
    for (test_mask = MASK_PRIMARY; test_mask <= MASK_BOTH; ++test_mask) {
      BuildTestGptData(gpt);
      if (test_mask & MASK_PRIMARY) {
        headers[PRIMARY]->size_of_entry = cases[i].value_to_test;
        headers[PRIMARY]->number_of_entries =
            TOTAL_ENTRIES_SIZE / cases[i].value_to_test;
      }
      if (test_mask & MASK_SECONDARY) {
        headers[SECONDARY]->size_of_entry = cases[i].value_to_test;
        headers[SECONDARY]->number_of_entries =
            TOTAL_ENTRIES_SIZE / cases[i].value_to_test;
      }
      valid_headers = CheckSizeOfPartitionEntry(gpt);
      if (cases[i].is_valid_value)
        EXPECT(MASK_BOTH == valid_headers);
      else
        EXPECT((MASK_BOTH ^ test_mask) == valid_headers);
    }
  }
  return TEST_OK;
}

/* Tests if NumberOfPartitionEntries is checes. NumberOfPartitionEntries must
 * be between 32 and 512, and SizeOfPartitionEntry * NumberOfPartitionEntries
 * must be 16384. */
int NumberOfPartitionEntriesTest() {
  GptData *gpt;
  struct {
    uint32_t size_of_entry;
    uint32_t number_of_entries;
    int is_valid_value;
  } cases[] = {
    {111, 147, 0},
    {111, 149, 0},
    {128, 32, 0},
    {128, 64, 0},
    {128, 127, 0},
    {128, 128, 1},
    {128, 129, 0},
    {128, 256, 0},
    {256, 32, 0},
    {256, 64, 1},
    {256, 128, 0},
    {256, 256, 0},
    {512, 32, 1},
    {512, 64, 0},
    {512, 128, 0},
    {512, 256, 0},
    {1024, 128, 0},
  };
  int i;
  int test_mask;
  GptHeader *headers[2];
  uint32_t valid_headers;

  gpt = GetEmptyGptData();
  headers[PRIMARY] = (GptHeader*)gpt->primary_header;
  headers[SECONDARY] = (GptHeader*)gpt->secondary_header;

  for (i = 0; i < ARRAY_SIZE(cases); ++i) {
    for (test_mask = MASK_PRIMARY; test_mask <= MASK_BOTH; ++test_mask) {
      BuildTestGptData(gpt);
      if (test_mask & MASK_PRIMARY) {
        headers[PRIMARY]->size_of_entry = cases[i].size_of_entry;
        headers[PRIMARY]->number_of_entries = cases[i].number_of_entries;
      }
      if (test_mask & MASK_SECONDARY) {
        headers[SECONDARY]->size_of_entry = cases[i].size_of_entry;
        headers[SECONDARY]->number_of_entries = cases[i].number_of_entries;
      }
      valid_headers = CheckNumberOfEntries(gpt);
      if (cases[i].is_valid_value)
        EXPECT(MASK_BOTH == valid_headers);
      else
        EXPECT((MASK_BOTH ^ test_mask) == valid_headers);
    }
  }
  return TEST_OK;
}

/* Tests if PartitionEntryLBA in primary/secondary headers is checked. */
int PartitionEntryLbaTest() {
  GptData *gpt;
  int test_mask;
  GptHeader *headers[2];
  uint32_t valid_headers;

  gpt = GetEmptyGptData();
  headers[PRIMARY] = (GptHeader*)gpt->primary_header;
  headers[SECONDARY] = (GptHeader*)gpt->secondary_header;

  for (test_mask = MASK_PRIMARY; test_mask <= MASK_BOTH; ++test_mask) {
    BuildTestGptData(gpt);
    if (test_mask & MASK_PRIMARY)
      headers[PRIMARY]->entries_lba = 0;
    if (test_mask & MASK_SECONDARY)
      headers[SECONDARY]->entries_lba = DEFAULT_DRIVE_SECTORS - 31 - 1;
    valid_headers = CheckEntriesLba(gpt);
    EXPECT((MASK_BOTH ^ test_mask) == valid_headers);
  }
  return TEST_OK;
}

/* Tests if FirstUsableLBA and LastUsableLBA are checked.
 * FirstUsableLBA must be after the end of the primary GPT table array.
 * LastUsableLBA must be before the start of the secondary GPT table array.
 * FirstUsableLBA <= LastUsableLBA. */
int FirstUsableLbaAndLastUsableLbaTest() {
  GptData *gpt;
  GptHeader *primary_header, *secondary_header;
  uint32_t valid_headers;
  int i;
  struct {
    uint64_t primary_entries_lba;
    uint64_t primary_first_usable_lba;
    uint64_t primary_last_usable_lba;
    uint64_t secondary_first_usable_lba;
    uint64_t secondary_last_usable_lba;
    uint64_t secondary_entries_lba;
    int expected_masks;
  } cases[] = {
    {2, 34, 433, 34, 433, 434, MASK_BOTH},
    {2, 34, 432, 34, 430, 434, MASK_BOTH},
    {2, 33, 433, 33, 433, 434, MASK_NONE},
    {3, 34, 433, 35, 433, 434, MASK_SECONDARY},
    {3, 35, 433, 33, 433, 434, MASK_PRIMARY},
    {2, 34, 434, 34, 433, 434, MASK_SECONDARY},
    {2, 34, 433, 34, 434, 434, MASK_PRIMARY},
    {2, 35, 433, 35, 433, 434, MASK_BOTH},
    {2, 433, 433, 433, 433, 434, MASK_BOTH},
    {2, 434, 433, 434, 434, 434, MASK_NONE},
    {2, 433, 34, 34, 433, 434, MASK_SECONDARY},
    {2, 34, 433, 433, 34, 434, MASK_PRIMARY},
  };

  gpt = GetEmptyGptData();
  primary_header = (GptHeader*)gpt->primary_header;
  secondary_header = (GptHeader*)gpt->secondary_header;

  for (i = 0; i < ARRAY_SIZE(cases); ++i) {
    BuildTestGptData(gpt);
    primary_header->entries_lba = cases[i].primary_entries_lba;
    primary_header->first_usable_lba = cases[i].primary_first_usable_lba;
    primary_header->last_usable_lba = cases[i].primary_last_usable_lba;
    secondary_header->entries_lba = cases[i].secondary_entries_lba;
    secondary_header->first_usable_lba = cases[i].secondary_first_usable_lba;
    secondary_header->last_usable_lba = cases[i].secondary_last_usable_lba;
    valid_headers = CheckValidUsableLbas(gpt);
    EXPECT(cases[i].expected_masks == valid_headers);
  }

  return TEST_OK;
}

/* Tests if header CRC in two copies are calculated. */
int HeaderCrcTest() {
  GptData *gpt;
  GptHeader *primary_header, *secondary_header;

  gpt = GetEmptyGptData();
  primary_header = (GptHeader*)gpt->primary_header;
  secondary_header = (GptHeader*)gpt->secondary_header;

  /* Modify the first byte of primary header, and expect the CRC is wrong. */
  BuildTestGptData(gpt);
  gpt->primary_header[0] ^= 0xa5;  /* just XOR a non-zero value */
  EXPECT(MASK_SECONDARY == CheckHeaderCrc(gpt));

  /* Modify the last byte of secondary header, and expect the CRC is wrong. */
  BuildTestGptData(gpt);
  gpt->secondary_header[secondary_header->size-1] ^= 0x5a;
  EXPECT(MASK_PRIMARY == CheckHeaderCrc(gpt));

  /* Modify out of CRC range, expect CRC is still right. */
  BuildTestGptData(gpt);
  gpt->primary_header[primary_header->size] ^= 0x87;
  EXPECT(MASK_BOTH == CheckHeaderCrc(gpt));

  return TEST_OK;
}

/* Tests if PartitionEntryArrayCRC32 is checked.
 * PartitionEntryArrayCRC32 must be calculated over SizeOfPartitionEntry *
 * NumberOfPartitionEntries bytes.
 */
int EntriesCrcTest() {
  GptData *gpt;

  gpt = GetEmptyGptData();

  /* Modify the first byte of primary entries, and expect the CRC is wrong. */
  BuildTestGptData(gpt);
  gpt->primary_entries[0] ^= 0xa5;  /* just XOR a non-zero value */
  EXPECT(MASK_SECONDARY == CheckEntriesCrc(gpt));

  /* Modify the last byte of secondary entries, and expect the CRC is wrong. */
  BuildTestGptData(gpt);
  gpt->secondary_entries[TOTAL_ENTRIES_SIZE-1] ^= 0x5a;
  EXPECT(MASK_PRIMARY == CheckEntriesCrc(gpt));

  return TEST_OK;
}

/* Tests if GptInit() handles non-identical partition entries well.
 * Two copies of partition table entries must be identical. If not, we trust the
 * primary table entries, and mark secondary as modified. */
int IdenticalEntriesTest() {
  GptData *gpt;

  gpt = GetEmptyGptData();

  /* Tests RepairEntries() first. */
  BuildTestGptData(gpt);
  EXPECT(0 == RepairEntries(gpt, MASK_BOTH));
  gpt->secondary_entries[0] ^= 0xa5;  /* XOR any number */
  EXPECT(GPT_MODIFIED_ENTRIES2 == RepairEntries(gpt, MASK_BOTH));
  EXPECT(GPT_MODIFIED_ENTRIES2 == RepairEntries(gpt, MASK_PRIMARY));
  EXPECT(GPT_MODIFIED_ENTRIES1 == RepairEntries(gpt, MASK_SECONDARY));
  EXPECT(0 == RepairEntries(gpt, MASK_NONE));

  /* The first byte is different. We expect secondary entries is marked as
   * modified. */
  BuildTestGptData(gpt);
  gpt->primary_entries[0] ^= 0xff;
  RefreshCrc32(gpt);
  EXPECT(GPT_SUCCESS == GptInit(gpt));
  EXPECT(GPT_MODIFIED_ENTRIES2 == gpt->modified);
  EXPECT(0 ==  Memcmp(gpt->primary_entries, gpt->secondary_entries,
                      TOTAL_ENTRIES_SIZE));

  /* The last byte is different, but the primary entries CRC is bad.
   * We expect primary entries is marked as modified. */
  BuildTestGptData(gpt);
  gpt->primary_entries[TOTAL_ENTRIES_SIZE-1] ^= 0xff;
  EXPECT(GPT_SUCCESS == GptInit(gpt));
  EXPECT(GPT_MODIFIED_ENTRIES1 == gpt->modified);
  EXPECT(0 == Memcmp(gpt->primary_entries, gpt->secondary_entries,
                     TOTAL_ENTRIES_SIZE));

  return TEST_OK;
}

/* Tests if GptInit() handles synonymous headers well.
 * Note that two partition headers are NOT bit-swise identical.
 * For exmaple, my_lba must be different (pointing to respective self).
 * So in normal case, they are synonymous, not identical.
 * If not synonymous, we trust the primary partition header, and
 * overwrite secondary, then mark secondary as modified.*/
int SynonymousHeaderTest() {
  GptData *gpt;
  GptHeader *primary_header, *secondary_header;

  gpt = GetEmptyGptData();
  primary_header = (GptHeader*)gpt->primary_header;
  secondary_header = (GptHeader*)gpt->secondary_header;

  /* Tests RepairHeader() for synonymous cases first. */
  BuildTestGptData(gpt);
  EXPECT(0 == RepairHeader(gpt, MASK_BOTH));
  EXPECT(GPT_MODIFIED_HEADER2 == RepairHeader(gpt, MASK_PRIMARY));
  EXPECT(GPT_MODIFIED_HEADER1 == RepairHeader(gpt, MASK_SECONDARY));
  EXPECT(0 == RepairHeader(gpt, MASK_NONE));
  /* Then tests non-synonymous cases. */
  BuildTestGptData(gpt);
  ++secondary_header->first_usable_lba;  /* chnage any bit */
  EXPECT(GPT_MODIFIED_HEADER2 == RepairHeader(gpt, MASK_BOTH));
  EXPECT(primary_header->first_usable_lba ==
         secondary_header->first_usable_lba);
  /* ---- */
  BuildTestGptData(gpt);
  --secondary_header->last_usable_lba;
  EXPECT(GPT_MODIFIED_HEADER2 == RepairHeader(gpt, MASK_BOTH));
  EXPECT(primary_header->last_usable_lba == secondary_header->last_usable_lba);
  /* ---- */
  BuildTestGptData(gpt);
  ++secondary_header->number_of_entries;
  EXPECT(GPT_MODIFIED_HEADER2 == RepairHeader(gpt, MASK_BOTH));
  EXPECT(primary_header->number_of_entries ==
         secondary_header->number_of_entries);
  /* ---- */
  BuildTestGptData(gpt);
  --secondary_header->size_of_entry;
  EXPECT(GPT_MODIFIED_HEADER2 == RepairHeader(gpt, MASK_BOTH));
  EXPECT(primary_header->size_of_entry ==
         secondary_header->size_of_entry);
  /* ---- */
  BuildTestGptData(gpt);
  secondary_header->disk_uuid.u.raw[0] ^= 0x56;
  EXPECT(GPT_MODIFIED_HEADER2 == RepairHeader(gpt, MASK_BOTH));
  EXPECT(0 == Memcmp(&primary_header->disk_uuid,
                     &secondary_header->disk_uuid, sizeof(Guid)));

  /* Consider header repairing in GptInit(). */
  BuildTestGptData(gpt);
  ++secondary_header->first_usable_lba;
  RefreshCrc32(gpt);
  EXPECT(GPT_SUCCESS == GptInit(gpt));
  EXPECT(GPT_MODIFIED_HEADER2 == gpt->modified);
  EXPECT(primary_header->first_usable_lba ==
         secondary_header->first_usable_lba);

  return TEST_OK;
}

/* Tests if partition geometry is checked.
 * All active (non-zero PartitionTypeGUID) partition entries should have:
 *   entry.StartingLBA >= header.FirstUsableLBA
 *   entry.EndingLBA <= header.LastUsableLBA
 *   entry.StartingLBA <= entry.EndingLBA
 */
int ValidEntryTest() {
  GptData *gpt;
  GptHeader *primary_header, *secondary_header;
  GptEntry *primary_entries, *secondary_entries;

  gpt = GetEmptyGptData();
  primary_header = (GptHeader*)gpt->primary_header;
  secondary_header = (GptHeader*)gpt->secondary_header;
  primary_entries = (GptEntry*)gpt->primary_entries;
  secondary_entries = (GptEntry*)gpt->secondary_entries;

  /* error case: entry.StartingLBA < header.FirstUsableLBA */
  BuildTestGptData(gpt);
  primary_entries[0].starting_lba = primary_header->first_usable_lba - 1;
  EXPECT(MASK_SECONDARY == CheckValidEntries(gpt));
  secondary_entries[1].starting_lba = secondary_header->first_usable_lba - 1;
  EXPECT(MASK_NONE == CheckValidEntries(gpt));

  /* error case: entry.EndingLBA > header.LastUsableLBA */
  BuildTestGptData(gpt);
  primary_entries[2].ending_lba = primary_header->last_usable_lba + 1;
  EXPECT(MASK_SECONDARY == CheckValidEntries(gpt));
  secondary_entries[3].ending_lba = secondary_header->last_usable_lba + 1;
  EXPECT(MASK_NONE == CheckValidEntries(gpt));

  /* error case: entry.StartingLBA > entry.EndingLBA */
  BuildTestGptData(gpt);
  primary_entries[3].starting_lba = primary_entries[3].ending_lba + 1;
  EXPECT(MASK_SECONDARY == CheckValidEntries(gpt));
  secondary_entries[1].starting_lba = secondary_entries[1].ending_lba + 1;
  EXPECT(MASK_NONE == CheckValidEntries(gpt));

  /* case: non active entry should be ignored. */
  BuildTestGptData(gpt);
  Memset(&primary_entries[1].type, 0, sizeof(primary_entries[1].type));
  primary_entries[1].starting_lba = primary_entries[1].ending_lba + 1;
  EXPECT(MASK_BOTH == CheckValidEntries(gpt));
  Memset(&secondary_entries[2].type, 0, sizeof(secondary_entries[2].type));
  secondary_entries[2].starting_lba = secondary_entries[2].ending_lba + 1;
  EXPECT(MASK_BOTH == CheckValidEntries(gpt));

  return TEST_OK;
}

/* Tests if overlapped partition tables can be detected. */
int OverlappedPartitionTest() {
  GptData *gpt;
  struct {
    int overlapped;
    struct {
      int active;
      uint64_t starting_lba;
      uint64_t ending_lba;
    } entries[16];  /* enough for testing. */
  } cases[] = {
    {0, {{0, 100, 199}, {0, 0, 0}}},
    {0, {{1, 100, 199}, {0, 0, 0}}},
    {0, {{1, 100, 150}, {1, 200, 250}, {1, 300, 350}, {0, 0, 0}}},
    {1, {{1, 200, 299}, {1, 100, 199}, {1, 100, 100}, {0, 0, 0}}},
    {1, {{1, 200, 299}, {1, 100, 199}, {1, 299, 299}, {0, 0, 0}}},
    {0, {{1, 300, 399}, {1, 200, 299}, {1, 100, 199}, {0, 0, 0}}},
    {1, {{1, 100, 199}, {1, 199, 299}, {1, 299, 399}, {0, 0, 0}}},
    {1, {{1, 100, 199}, {1, 200, 299}, {1, 75, 399}, {0, 0, 0}}},
    {1, {{1, 100, 199}, {1, 75, 250}, {1, 200, 299}, {0, 0, 0}}},
    {1, {{1, 75, 150}, {1, 100, 199}, {1, 200, 299}, {0, 0, 0}}},
    {1, {{1, 200, 299}, {1, 100, 199}, {1, 300, 399}, {1, 100, 399},
         {0, 0, 0}}},
    {0, {{1, 200, 299}, {1, 100, 199}, {1, 300, 399}, {0, 100, 399},
         {0, 0, 0}}},
    {1, {{1, 200, 300}, {1, 100, 200}, {1, 100, 400}, {1, 300, 400},
         {0, 0, 0}}},
    {1, {{0, 200, 300}, {1, 100, 200}, {1, 100, 400}, {1, 300, 400},
         {0, 0, 0}}},
    {0, {{1, 200, 300}, {1, 100, 199}, {0, 100, 400}, {0, 300, 400},
         {0, 0, 0}}},
    {1, {{1, 200, 299}, {1, 100, 199}, {1, 199, 199}, {0, 0, 0}}},
    {0, {{1, 200, 299}, {0, 100, 199}, {1, 199, 199}, {0, 0, 0}}},
    {0, {{1, 200, 299}, {1, 100, 199}, {0, 199, 199}, {0, 0, 0}}},
    {1, {{1, 199, 199}, {1, 200, 200}, {1, 201, 201}, {1, 202, 202},
         {1, 203, 203}, {1, 204, 204}, {1, 205, 205}, {1, 206, 206},
         {1, 207, 207}, {1, 208, 208}, {1, 199, 199}, {0, 0, 0}}},
    {0, {{1, 199, 199}, {1, 200, 200}, {1, 201, 201}, {1, 202, 202},
         {1, 203, 203}, {1, 204, 204}, {1, 205, 205}, {1, 206, 206},
         {1, 207, 207}, {1, 208, 208}, {0, 199, 199}, {0, 0, 0}}},
  };
  Guid any_type = GPT_ENT_TYPE_CHROMEOS_KERNEL;
  int i, j;
  int test_mask;
  GptEntry *entries[2];

  gpt = GetEmptyGptData();
  entries[PRIMARY] = (GptEntry*)gpt->primary_entries;
  entries[SECONDARY] = (GptEntry*)gpt->secondary_entries;

  for (i = 0; i < ARRAY_SIZE(cases); ++i) {
    for (test_mask = MASK_PRIMARY; test_mask <= MASK_BOTH; ++test_mask) {
      BuildTestGptData(gpt);
      ZeroEntries(gpt);
      for(j = 0; j < ARRAY_SIZE(cases[0].entries); ++j) {
        if (!cases[i].entries[j].starting_lba) break;
        if (test_mask & MASK_PRIMARY) {
          if (cases[i].entries[j].active)
            Memcpy(&entries[PRIMARY][j].type, &any_type, sizeof(any_type));
          entries[PRIMARY][j].starting_lba = cases[i].entries[j].starting_lba;
          entries[PRIMARY][j].ending_lba = cases[i].entries[j].ending_lba;
        }
        if (test_mask & MASK_SECONDARY) {
          if (cases[i].entries[j].active)
            Memcpy(&entries[SECONDARY][j].type, &any_type, sizeof(any_type));
          entries[SECONDARY][j].starting_lba = cases[i].entries[j].starting_lba;
          entries[SECONDARY][j].ending_lba = cases[i].entries[j].ending_lba;
        }
      }
      EXPECT((cases[i].overlapped * test_mask) ==
             (OverlappedEntries(entries[PRIMARY], j) |
              (OverlappedEntries(entries[SECONDARY], j) << SECONDARY))
      );

      EXPECT((MASK_BOTH ^ (cases[i].overlapped * test_mask)) ==
             CheckOverlappedPartition(gpt));
    }
  }
  return TEST_OK;
}

/* Tests if GptInit() can survive in different corrupt header/entries
 * combinations, like:
 *   primary GPT header         - valid
 *   primary partition table    - invalid
 *   secondary GPT header       - invalid
 *   secondary partition table  - valid
 */
int CorruptCombinationTest() {
  GptData *gpt;
  GptHeader *primary_header, *secondary_header;
  GptEntry *primary_entries, *secondary_entries;

  gpt = GetEmptyGptData();
  primary_header = (GptHeader*)gpt->primary_header;
  secondary_header = (GptHeader*)gpt->secondary_header;
  primary_entries = (GptEntry*)gpt->primary_entries;
  secondary_entries = (GptEntry*)gpt->secondary_entries;

  /* Make primary entries and secondary header invalid, we expect GptInit()
   * can recover them (returns GPT_SUCCESS and MODIFIED flasgs). */
  BuildTestGptData(gpt);
  primary_entries[0].type.u.raw[0] ^= 0x33;
  secondary_header->header_crc32 ^= 0x55;
  EXPECT(GPT_SUCCESS == GptInit(gpt));
  EXPECT((GPT_MODIFIED_HEADER2 | GPT_MODIFIED_ENTRIES1) == gpt->modified);
  EXPECT(0 == Memcmp(primary_entries, secondary_entries, TOTAL_ENTRIES_SIZE));
  /* We expect the modified header/entries can pass GptInit(). */
  EXPECT(GPT_SUCCESS == GptInit(gpt));
  EXPECT(0 == gpt->modified);

  /* Make primary header invalid (the entries is not damaged actually). */
  BuildTestGptData(gpt);
  primary_header->entries_crc32 ^= 0x73;
  EXPECT(GPT_SUCCESS == GptInit(gpt));
  /* After header is repaired, the entries are valid actually. */
  EXPECT((GPT_MODIFIED_HEADER1) == gpt->modified);
  /* We expect the modified header/entries can pass GptInit(). */
  EXPECT(GPT_SUCCESS == GptInit(gpt));
  EXPECT(0 == gpt->modified);

  return TEST_OK;
}

/* Invalidate all kernel entries and expect GptNextKernelEntry() cannot find
 * any usable kernel entry.
 */
int NoValidKernelEntryTest() {
  GptData *gpt;
  GptEntry *entries, *entries2;

  gpt = GetEmptyGptData();
  entries = (GptEntry*)gpt->primary_entries;
  entries2 = (GptEntry*)gpt->secondary_entries;

  BuildTestGptData(gpt);
  entries[KERNEL_A].attributes |= CGPT_ATTRIBUTE_BAD_MASK;
  Memset(&entries[KERNEL_B].type, 0, sizeof(Guid));
  RefreshCrc32(gpt);

  EXPECT(GPT_ERROR_NO_VALID_KERNEL == GptNextKernelEntry(gpt, NULL, NULL));

  return TEST_OK;
}

/* This is the combination test. Both kernel A and B could be either inactive
 * or invalid. We expect GptNextKetnelEntry() returns good kernel or
 * GPT_ERROR_NO_VALID_KERNEL if no kernel is available. */
enum FAILURE_MASK {
  MASK_INACTIVE = 1,
  MASK_BAD_ENTRY = 2,
  MASK_FAILURE_BOTH = 3,
};
void BreakAnEntry(GptEntry *entry, enum FAILURE_MASK failure) {
  if (failure & MASK_INACTIVE)
    Memset(&entry->type, 0, sizeof(Guid));
  if (failure & MASK_BAD_ENTRY)
    entry->attributes |= CGPT_ATTRIBUTE_BAD_MASK;
}

int CombinationalNextKernelEntryTest() {
  GptData *gpt;
  enum {
    MASK_KERNEL_A = 1,
    MASK_KERNEL_B = 2,
    MASK_KERNEL_BOTH = 3,
  } kernel;
  enum FAILURE_MASK failure;
  uint64_t start_sector, size;
  int retval;

  for (kernel = MASK_KERNEL_A; kernel <= MASK_KERNEL_BOTH; ++kernel) {
    for (failure = MASK_INACTIVE; failure < MASK_FAILURE_BOTH; ++failure) {
      gpt = GetEmptyGptData();
      BuildTestGptData(gpt);

      if (kernel & MASK_KERNEL_A)
        BreakAnEntry(GetEntry(gpt, PRIMARY, KERNEL_A), failure);
      if (kernel & MASK_KERNEL_B)
        BreakAnEntry(GetEntry(gpt, PRIMARY, KERNEL_B), failure);

      retval = GptNextKernelEntry(gpt, &start_sector, &size);

      if (kernel == MASK_KERNEL_A) {
        EXPECT(retval == GPT_SUCCESS);
        EXPECT(start_sector == 334);
      } else if (kernel == MASK_KERNEL_B) {
        EXPECT(retval == GPT_SUCCESS);
        EXPECT(start_sector == 34);
      } else {  /* MASK_KERNEL_BOTH */
        EXPECT(retval == GPT_ERROR_NO_VALID_KERNEL);
      }
    }
  }
  return TEST_OK;
}

/* Increase tries value from zero, expect it won't explode/overflow after
 * CGPT_ATTRIBUTE_TRIES_MASK.
 */
/* Tries would not count up after CGPT_ATTRIBUTE_MAX_TRIES. */
#define EXPECTED_TRIES(tries) \
    ((tries >= CGPT_ATTRIBUTE_MAX_TRIES) ? CGPT_ATTRIBUTE_MAX_TRIES \
                                         : tries)
int IncreaseTriesTest() {
  GptData *gpt;
  int kernel_index[] = {
    KERNEL_B,
    KERNEL_A,
  };
  int i, tries, j;

  gpt = GetEmptyGptData();
  for (i = 0; i < ARRAY_SIZE(kernel_index); ++i) {
    GptEntry *entries[2] = {
      (GptEntry*)gpt->primary_entries,
      (GptEntry*)gpt->secondary_entries,
    };
    int current;

    BuildTestGptData(gpt);
    current = gpt->current_kernel = kernel_index[i];

    for (tries = 0; tries < 2 * CGPT_ATTRIBUTE_MAX_TRIES; ++tries) {
      for (j = 0; j < ARRAY_SIZE(entries); ++j) {
        EXPECT(EXPECTED_TRIES(tries) ==
               ((entries[j][current].attributes & CGPT_ATTRIBUTE_TRIES_MASK) >>
                CGPT_ATTRIBUTE_TRIES_OFFSET));
      }

      EXPECT(GPT_SUCCESS == GptUpdateKernelEntry(gpt, GPT_UPDATE_ENTRY_TRY));
      /* The expected tries value will be checked in next iteration. */

      if (tries < CGPT_ATTRIBUTE_MAX_TRIES)
        EXPECT((GPT_MODIFIED_HEADER1 | GPT_MODIFIED_ENTRIES1 |
                GPT_MODIFIED_HEADER2 | GPT_MODIFIED_ENTRIES2) == gpt->modified);
      gpt->modified = 0;  /* reset before next test */
      EXPECT(0 ==
             Memcmp(entries[PRIMARY], entries[SECONDARY], TOTAL_ENTRIES_SIZE));
    }
  }
  return TEST_OK;
}

/* Mark a kernel as bad. Expect:
 *   1. the both bad bits of kernel A in primary and secondary entries are set.
 *   2. headers and entries are marked as modified.
 *   3. primary and secondary entries are identical.
 */
int MarkBadKernelEntryTest() {
  GptData *gpt;
  GptEntry *entries, *entries2;

  gpt = GetEmptyGptData();
  entries = (GptEntry*)gpt->primary_entries;
  entries2 = (GptEntry*)gpt->secondary_entries;

  BuildTestGptData(gpt);
  gpt->current_kernel = KERNEL_A;
  EXPECT(GPT_SUCCESS == GptUpdateKernelEntry(gpt, GPT_UPDATE_ENTRY_BAD));
  EXPECT((GPT_MODIFIED_HEADER1 | GPT_MODIFIED_ENTRIES1 |
          GPT_MODIFIED_HEADER2 | GPT_MODIFIED_ENTRIES2) == gpt->modified);
  EXPECT(entries[KERNEL_A].attributes & CGPT_ATTRIBUTE_BAD_MASK);
  EXPECT(entries2[KERNEL_A].attributes & CGPT_ATTRIBUTE_BAD_MASK);
  EXPECT(0 == Memcmp(entries, entries2, TOTAL_ENTRIES_SIZE));

  return TEST_OK;
}

/* Given an invalid kernel type, and expect GptUpdateKernelEntry() returns
 * GPT_ERROR_INVALID_UPDATE_TYPE. */
int UpdateInvalidKernelTypeTest() {
  GptData *gpt;

  gpt = GetEmptyGptData();
  BuildTestGptData(gpt);
  gpt->current_kernel = 0;  /* anything, but not CGPT_KERNEL_ENTRY_NOT_FOUND */
  EXPECT(GPT_ERROR_INVALID_UPDATE_TYPE ==
         GptUpdateKernelEntry(gpt, 99));  /* any invalid update_type value */

  return TEST_OK;
}

/* A normal boot case:
 *   GptInit()
 *   GptNextKernelEntry()
 *   GptUpdateKernelEntry()
 */
int NormalBootCase() {
  GptData *gpt;
  GptEntry *entries;
  uint64_t start_sector, size;

  gpt = GetEmptyGptData();
  entries = (GptEntry*)gpt->primary_entries;
  BuildTestGptData(gpt);

  EXPECT(GPT_SUCCESS == GptInit(gpt));
  EXPECT(GPT_SUCCESS == GptNextKernelEntry(gpt, &start_sector, &size));
  EXPECT(start_sector == 34);  /* Kernel A, see top of this file. */
  EXPECT(size == 100);

  EXPECT(GPT_SUCCESS == GptUpdateKernelEntry(gpt, GPT_UPDATE_ENTRY_TRY));
  EXPECT(((entries[KERNEL_A].attributes & CGPT_ATTRIBUTE_TRIES_MASK) >>
           CGPT_ATTRIBUTE_TRIES_OFFSET) == 1);

  return TEST_OK;
}

/* Higher priority kernel should boot first.
 *   KERNEL_A is low priority
 *   KERNEL_B is high priority.
 * We expect KERNEL_B is selected in first run, and then KERNEL_A.
 * We also expect the GptNextKernelEntry() wraps back to KERNEL_B if it's called
 * after twice.
 */
int HigherPriorityTest() {
  GptData *gpt;
  GptEntry *entries;

  gpt = GetEmptyGptData();
  entries = (GptEntry*)gpt->primary_entries;
  BuildTestGptData(gpt);

  SetPriority(gpt, PRIMARY, KERNEL_A, 0);
  SetPriority(gpt, PRIMARY, KERNEL_B, 1);
  RefreshCrc32(gpt);

  EXPECT(GPT_SUCCESS == GptInit(gpt));
  EXPECT(GPT_SUCCESS == GptNextKernelEntry(gpt, NULL, NULL));
  EXPECT(KERNEL_B == gpt->current_kernel);

  EXPECT(GPT_SUCCESS == GptNextKernelEntry(gpt, NULL, NULL));
  EXPECT(KERNEL_A == gpt->current_kernel);

  EXPECT(GPT_SUCCESS == GptNextKernelEntry(gpt, NULL, NULL));
  EXPECT(KERNEL_B == gpt->current_kernel);

  return TEST_OK;
}

int main(int argc, char *argv[]) {
  int i;
  int error_count = 0;
  struct {
    char *name;
    test_func fp;
    int retval;
  } test_cases[] = {
    { TEST_CASE(TestBuildTestGptData), },
    { TEST_CASE(ParameterTests), },
    { TEST_CASE(SignatureTest), },
    { TEST_CASE(RevisionTest), },
    { TEST_CASE(SizeTest), },
    { TEST_CASE(ReservedFieldsTest), },
    { TEST_CASE(MyLbaTest), },
    { TEST_CASE(SizeOfPartitionEntryTest), },
    { TEST_CASE(NumberOfPartitionEntriesTest), },
    { TEST_CASE(PartitionEntryLbaTest), },
    { TEST_CASE(FirstUsableLbaAndLastUsableLbaTest), },
    { TEST_CASE(HeaderCrcTest), },
    { TEST_CASE(EntriesCrcTest), },
    { TEST_CASE(IdenticalEntriesTest), },
    { TEST_CASE(SynonymousHeaderTest), },
    { TEST_CASE(ValidEntryTest), },
    { TEST_CASE(OverlappedPartitionTest), },
    { TEST_CASE(CorruptCombinationTest), },
    { TEST_CASE(TestQuickSortFixed), },
    { TEST_CASE(TestQuickSortRandom), },
    { TEST_CASE(NoValidKernelEntryTest), },
    { TEST_CASE(CombinationalNextKernelEntryTest), },
    { TEST_CASE(IncreaseTriesTest), },
    { TEST_CASE(MarkBadKernelEntryTest), },
    { TEST_CASE(UpdateInvalidKernelTypeTest), },
    { TEST_CASE(NormalBootCase), },
    { TEST_CASE(HigherPriorityTest), },
    { TEST_CASE(TestCrc32TestVectors), },
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
    printf("\n--------------------------------------------------\n");
    printf(COL_RED "The following %d test cases are failed:\n" COL_STOP,
           error_count);
    for (i = 0; i < sizeof(test_cases)/sizeof(test_cases[0]); ++i) {
      if (test_cases[i].retval)
        printf("  %s()\n", test_cases[i].name);
    }
  }

  return (error_count) ? 1 : 0;
}
