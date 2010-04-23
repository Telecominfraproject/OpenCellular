/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "cgpt_test.h"
#include <string.h>
#include "cgpt.h"
#include "gpt.h"
#include "utility.h"

/* Testing partition layout (sector_bytes=512)
 *
 *     LBA   Size  Usage
 *       0      1  PMBR
 *       1      1  primary partition header
 *       2     32  primary partition entries (128B * 128)
 *      34    100  kernel A
 *     134    100  kernel B
 *     234    100  root A
 *     334    100  root B
 *     434     32  secondary partition entries
 *     466      1  secondary partition header
 *     467
 */
#define DEFAULT_SECTOR_SIZE 512
#define MAX_SECTOR_SIZE 4096
#define DEFAULT_DRIVE_SECTORS 467
#define PARTITION_ENTRIES_SIZE (16*1024)

#define TEST_CASE(func) #func, func
typedef int (*test_func)(void);

/* NOT A REAL CRC32, it is fake before I call real one . FIXME */
uint32_t CalculateCrc32(const uint8_t *start, size_t len) {
  uint32_t buf = 0;
  int i;
  for (i = 0; i < len; i += 4, len -= 4) {
    buf ^= *(uint32_t*)&start[i];
  }
  if (len >= 3) buf ^= start[i-2] << 16;
  if (len >= 2) buf ^= start[i-3] << 8;
  if (len >= 1) buf ^= start[i-4];
  return buf;
}

/* Given a GptData pointer, first re-calculate entries CRC32 value,
 * then reset header CRC32 value to 0, and calculate header CRC32 value.
 * Both primary and secondary are updated. */
void RefreshCrc32(struct GptData *gpt) {
  GptHeader *header, *header2;
  GptEntry *entries, *entries2;

  header = (GptHeader*)gpt->primary_header;
  entries = (GptEntry*)gpt->primary_entries;
  header2 = (GptHeader*)gpt->secondary_header;
  entries2 = (GptEntry*)gpt->secondary_entries;

  header->entries_crc32 = CalculateCrc32((uint8_t*)entries,
                                         sizeof(GptEntry));
  header->header_crc32 = 0;
  header->header_crc32 = CalculateCrc32((uint8_t*)header,
                                        header->size);
  header2->entries_crc32 = CalculateCrc32((uint8_t*)entries2,
                                          sizeof(GptEntry));
  header2->header_crc32 = 0;
  header2->header_crc32 = CalculateCrc32((uint8_t*)header2,
                                         header2->size);
}

/* Returns a pointer to a static GptData instance (no free is required).
 * All fields are zero except 4 pointers linking to header and entries.
 * All content of headers and entries are zero. */
struct GptData* GetAClearGptData() {
  static GptData_t gpt;
  static uint8_t primary_header[MAX_SECTOR_SIZE];
  static uint8_t primary_entries[PARTITION_ENTRIES_SIZE];
  static uint8_t secondary_header[MAX_SECTOR_SIZE];
  static uint8_t secondary_entries[PARTITION_ENTRIES_SIZE];

  Memset(&gpt, 0, sizeof(gpt));
  Memset(&primary_header, 0, sizeof(primary_header));
  Memset(&primary_entries, 0, sizeof(primary_entries));
  Memset(&secondary_header, 0, sizeof(secondary_header));
  Memset(&secondary_entries, 0, sizeof(secondary_entries));

  gpt.primary_header = primary_header;
  gpt.primary_entries = primary_entries;
  gpt.secondary_header = secondary_header;
  gpt.secondary_entries = secondary_entries;

  return &gpt;
}

/* Fills in most of fields and creates the layout described in the top of this
 * file. */
struct GptData*
BuildTestGptData(uint32_t sector_bytes) {
  GptData_t *gpt;
  GptHeader *header, *header2;
  GptEntry *entries, *entries2;
  Guid chromeos_kernel = GPT_ENT_TYPE_CHROMEOS_KERNEL;

  gpt = GetAClearGptData();
  gpt->sector_bytes = sector_bytes;
  gpt->drive_sectors = DEFAULT_DRIVE_SECTORS;

  /* build primary */
  header = (GptHeader*)gpt->primary_header;
  entries = (GptEntry*)gpt->primary_entries;
  Memcpy(header->signature, GPT_HEADER_SIGNATURE, sizeof(GPT_HEADER_SIGNATURE));
  header->revision = GPT_HEADER_REVISION;
  header->size = sizeof(GptHeader) - sizeof(header->padding);
  header->my_lba = 1;
  header->first_usable_lba = 34;
  header->last_usable_lba = DEFAULT_DRIVE_SECTORS - 1 - 32 - 1;  /* 433 */
  header->entries_lba = 2;
  header->number_of_entries = 128;  /* 512B / 128B * 32sectors = 128 entries */
  header->size_of_entry = 128;  /* bytes */
  Memcpy(&entries[0].type, &chromeos_kernel, sizeof(chromeos_kernel));
  entries[0].starting_lba = 34;
  entries[0].ending_lba = 133;
  Memcpy(&entries[1].type, &chromeos_kernel, sizeof(chromeos_kernel));
  entries[1].starting_lba = 134;
  entries[1].ending_lba = 233;
  Memcpy(&entries[2].type, &chromeos_kernel, sizeof(chromeos_kernel));
  entries[2].starting_lba = 234;
  entries[2].ending_lba = 333;
  Memcpy(&entries[3].type, &chromeos_kernel, sizeof(chromeos_kernel));
  entries[3].starting_lba = 334;
  entries[3].ending_lba = 433;

  /* build secondary */
  header2 = (GptHeader*)gpt->secondary_header;
  entries2 = (GptEntry*)gpt->secondary_entries;
  Memcpy(header2, header, sizeof(header));
  Memcpy(entries2, entries, sizeof(entries));
  header2->my_lba = DEFAULT_DRIVE_SECTORS - 1;  /* 466 */
  header2->entries_lba = DEFAULT_DRIVE_SECTORS - 1 - 32;  /* 434 */

  RefreshCrc32(gpt);
  return gpt;
}

/* Dumps memory starting from [vp] with [len] bytes.
 * Prints [memo] if not NULL. Example output:
 *
 *  00 01 02 03 04 05 06 07 - 08 09 0a 0b 0c 0d 0e 0f
 *  10 11 12 13 14 15 16 17 - 18 19 1a 1b 1c 1d 1e 1f
 *  ...
 */
static void dump(void *vp, int len, char* memo) {
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
void DumpGptData(struct GptData *gpt) {
  printf("DumpGptData(%p)...\n", gpt);
  dump(gpt, sizeof(gpt), NULL);
  dump(gpt->primary_header, sizeof(GptHeader), "Primary header");
  dump(gpt->primary_entries, sizeof(GptEntry) * 8, "Primary entries");
  dump(gpt->secondary_header, sizeof(GptHeader), "Secondary header");
  dump(gpt->secondary_entries, sizeof(GptEntry) * 8,
       "Secondary entries");
}

/* Tests if signature ("EFI PART") is checked. */
int SignatureTest() {
  int i;
  GptData_t *gpt;
  GptHeader *primary_header, *secondary_header;

  gpt = BuildTestGptData(DEFAULT_SECTOR_SIZE);
  primary_header = (GptHeader*)gpt->primary_header;
  secondary_header = (GptHeader*)gpt->secondary_header;

  EXPECT(GPT_SUCCESS == GptInit(gpt));

  /* change every char in signature of primary. Secondary is still valid. */
  for (i = 0; i < 8; ++i) {
    gpt->primary_header[i] ^= 0xff;
    RefreshCrc32(gpt);
    EXPECT(GPT_SUCCESS == GptInit(gpt));
    gpt->primary_header[i] ^= 0xff;
    RefreshCrc32(gpt);
  }

  /* change every char in signature of secondary. Primary is still valid. */
  for (i = 0; i < 8; ++i) {
    gpt->secondary_header[i] ^= 0xff;
    RefreshCrc32(gpt);
    EXPECT(GPT_SUCCESS == GptInit(gpt));
    gpt->secondary_header[i] ^= 0xff;
    RefreshCrc32(gpt);
  }

  /* change every char in signature of primary and secondary. Expect fail. */
  for (i = 0; i < 8; ++i) {
    gpt->primary_header[i] ^= 0xff;
    gpt->secondary_header[i] ^= 0xff;
    RefreshCrc32(gpt);
    EXPECT(GPT_ERROR_INVALID_HEADERS == GptInit(gpt));
    gpt->primary_header[i] ^= 0xff;
    gpt->secondary_header[i] ^= 0xff;
    RefreshCrc32(gpt);
  }

  return TEST_OK;
}

/* Tests if header CRC in two copies are calculated. */
int HeaderCrcTest() {
  return TEST_FAIL;
}

/* Tests if myLBA field is checked (1 for primary, last for secondary). */
int MyLbaTest() {
  return TEST_FAIL;
}

/* Tests if SizeOfPartitionEntry is checked. SizeOfPartitionEntry must be
 * between 128 and 512, and a multiple of 8. */
int SizeOfPartitionEntryTest() {
  return TEST_FAIL;
}

/* Tests if NumberOfPartitionEntries is checes. NumberOfPartitionEntries must
 * be between 32 and 512, and SizeOfPartitionEntry * NumberOfPartitionEntries
 * must be 16384. */
int NumberOfPartitionEntriesTest() {
  return TEST_FAIL;
}

/* Tests if PartitionEntryLBA in primary/secondary headers is checked. */
int PartitionEntryLbaTest() {
  return TEST_FAIL;
}

/* Tests if FirstUsableLBA and LastUsableLBA are checked.
 * FirstUsableLBA must be after the end of the primary GPT table array.
 * LastUsableLBA must be before the start of the secondary GPT table array.
 * FirstUsableLBA <= LastUsableLBA. */
int FirstUsableLbaAndLastUsableLbaTest() {
  return TEST_FAIL;
}

/* Tests if GptInit() handles non-identical partition entries well.
 * Two copies of partition table entries must be identical. If not, we trust the
 * primary table entries, and mark secondary as modified (see Caller's write-
 * back order below). */
int IdenticalEntriesTest() {
  return TEST_FAIL;
}

/* Tests if GptInit() handles non-identical headers well.
 * Two partition headers must be identical. If not, we trust the primary
 * partition header, and mark secondary as modified (see Caller's write-back
 * order below). */
int IdenticalHeaderTest() {
  return TEST_FAIL;
}

/* Tests if PartitionEntryArrayCRC32 is checked.
 * PartitionEntryArrayCRC32 must be calculated over SizeOfPartitionEntry *
 * NumberOfPartitionEntries bytes.
 */
int EntriesCrcTest() {
  return TEST_FAIL;
}

/* Tests if partition geometry is checked.
 * All active (non-zero PartitionTypeGUID) partition entries should have:
 *   entry.StartingLBA >= header.FirstUsableLBA
 *   entry.EndingLBA <= header.LastUsableLBA
 *   entry.StartingLBA <= entry.EndingLBA
 */
int ValidEntryTest() {
  return TEST_FAIL;
}

/* Tests if overlapped partition tables can be detected. */
int NoOverlappedPartitionTest() {
  return TEST_FAIL;
}

/* Tests if GptNextKernelEntry() can survive in different corrupt header/entries
 * combinations, like:
 *   primary GPT header         - valid
 *   primary partition table    - invalid
 *   secondary partition table  - valid
 *   secondary GPT header       - invalid
 */
int CorruptCombinationTest() {
  return TEST_FAIL;
}

int main(int argc, char *argv[]) {
  int i;
  struct {
    char *name;
    test_func fp;
    int retval;
  } test_cases[] = {
    { TEST_CASE(SignatureTest), },
#if 0
    { TEST_CASE(HeaderCrcTest), },
    { TEST_CASE(MyLbaTest), },
    { TEST_CASE(SizeOfPartitionEntryTest), },
    { TEST_CASE(NumberOfPartitionEntriesTest), },
    { TEST_CASE(PartitionEntryLbaTest), },
    { TEST_CASE(FirstUsableLbaAndLastUsableLbaTest), },
    { TEST_CASE(IdenticalEntriesTest), },
    { TEST_CASE(IdenticalHeaderTest), },
    { TEST_CASE(EntriesCrcTest), },
    { TEST_CASE(ValidEntryTest), },
    { TEST_CASE(NoOverlappedPartitionTest), },
    { TEST_CASE(CorruptCombinationTest), },
#endif
  };

  for (i = 0; i < sizeof(test_cases)/sizeof(test_cases[0]); ++i) {
    printf("Running %s() ...\n", test_cases[i].name);
    test_cases[i].retval = test_cases[i].fp();
    if (test_cases[i].retval)
      printf(COL_RED "[ERROR]" COL_STOP " %s()\n\n", test_cases[i].name);
    else
      printf(COL_GREEN "[PASS]" COL_STOP " %s()\n\n", test_cases[i].name);
  }

  printf("\n--------------------------------------------------\n");
  printf("The following test cases are failed:\n");
  for (i = 0; i < sizeof(test_cases)/sizeof(test_cases[0]); ++i) {
    if (test_cases[i].retval)
      printf("  %s()\n", test_cases[i].name);
  }

  return 0;
}
