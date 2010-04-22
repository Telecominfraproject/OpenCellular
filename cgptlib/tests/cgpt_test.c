/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "cgpt.h"
#include "cgpt_test.h"

#define TEST_CASE(func) #func, func
typedef int (*test_func)(void);

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

/* Tests if GPTInit() handles non-identical partition entries well.
 * Two copies of partition table entries must be identical. If not, we trust the
 * primary table entries, and mark secondary as modified (see Caller's write-
 * back order below). */
int IdenticalEntriesTest() {
  return TEST_FAIL;
}

/* Tests if GPTInit() handles non-identical headers well.
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

/* Tests if GPTNextKernelEntry() can survive in different corrupt header/entries
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
