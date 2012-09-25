/* Copyright (c) 2010-2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef VBOOT_REFERENCE_CGPTLIB_INTERNAL_H_
#define VBOOT_REFERENCE_CGPTLIB_INTERNAL_H_

#include "sysincludes.h"
#include "cgptlib.h"
#include "gpt.h"

/* If gpt->current_kernel is this value, means either:
 *   1. an initial value before scanning GPT entries,
 *   2. after scanning, no any valid kernel is found.
 */
#define CGPT_KERNEL_ENTRY_NOT_FOUND (-1)

/* Bit definitions and masks for GPT attributes.
 *
 *  63-61  -- (reserved)
 *     60  -- read-only
 *  59-57  -- (reserved)
 *     56  -- success
 *  55-52  -- tries
 *  51-48  -- priority
 *   47-2  -- UEFI: reserved for future use
 *      1  -- UEFI: partition is not mapped
 *      0  -- UEFI: partition is required
 */
#define CGPT_ATTRIBUTE_SUCCESSFUL_OFFSET (56 - 48)
#define CGPT_ATTRIBUTE_MAX_SUCCESSFUL (1ULL)
#define CGPT_ATTRIBUTE_SUCCESSFUL_MASK (CGPT_ATTRIBUTE_MAX_SUCCESSFUL << \
                                     CGPT_ATTRIBUTE_SUCCESSFUL_OFFSET)

#define CGPT_ATTRIBUTE_TRIES_OFFSET (52 - 48)
#define CGPT_ATTRIBUTE_MAX_TRIES (15ULL)
#define CGPT_ATTRIBUTE_TRIES_MASK (CGPT_ATTRIBUTE_MAX_TRIES << \
                                   CGPT_ATTRIBUTE_TRIES_OFFSET)

#define CGPT_ATTRIBUTE_PRIORITY_OFFSET (48 - 48)
#define CGPT_ATTRIBUTE_MAX_PRIORITY (15ULL)
#define CGPT_ATTRIBUTE_PRIORITY_MASK (CGPT_ATTRIBUTE_MAX_PRIORITY << \
                                      CGPT_ATTRIBUTE_PRIORITY_OFFSET)

/* Defines ChromeOS-specific limitation on GPT */
#define MIN_SIZE_OF_HEADER 92
#define MAX_SIZE_OF_HEADER 512
#define MIN_SIZE_OF_ENTRY 128
#define MAX_SIZE_OF_ENTRY 512
#define SIZE_OF_ENTRY_MULTIPLE 8
#define MIN_NUMBER_OF_ENTRIES 32
#define MAX_NUMBER_OF_ENTRIES 512

/* Defines GPT sizes */
#define GPT_PMBR_SECTOR 1  /* size (in sectors) of PMBR */
#define GPT_HEADER_SECTOR 1
#define GPT_ENTRIES_SECTORS 32  /* assume sector size if 512 bytes, then
                                 *  (TOTAL_ENTRIES_SIZE / 512) = 32 */

/* alias name of index in internal array for primary and secondary header and
 * entries. */
enum {
  /* constants for index */
  PRIMARY = 0,
  SECONDARY = 1,
  ANY_VALID = 9999,  /* accept any between primary and secondary */

  /* constants for bit mask */
  MASK_NONE = 0,
  MASK_PRIMARY = 1,
  MASK_SECONDARY = 2,
  MASK_BOTH = 3,
};

/* Verify GptData parameters are sane. */
int CheckParameters(GptData* gpt);

/* Check header fields.
 *
 * Returns 0 if header is valid, 1 if invalid. */
int CheckHeader(GptHeader* h, int is_secondary, uint64_t drive_sectors);

/* Calculate and return the header CRC. */
uint32_t HeaderCrc(GptHeader* h);

/* Check entries.
 *
 * Returns 0 if entries are valid, 1 if invalid. */
int CheckEntries(GptEntry* entries, GptHeader* h);

/* Check GptData, headers, entries.
 *
 * If successful, sets gpt->valid_headers and gpt->valid_entries and returns
 * GPT_SUCCESS.
 *
 * On error, returns a GPT_ERROR_* return code. */
int GptSanityCheck(GptData* gpt);

/* Repairs GPT data by copying from one set of valid headers/entries to the
 * other.  Assumes GptSanityCheck() has been run to determine which headers
 * and/or entries are already valid. */
void GptRepair(GptData* gpt);

/* Getters and setters for partition attribute fields. */
int GetEntrySuccessful(const GptEntry* e);
int GetEntryPriority(const GptEntry* e);
int GetEntryTries(const GptEntry* e);
void SetEntrySuccessful(GptEntry* e, int successful);
void SetEntryPriority(GptEntry* e, int priority);
void SetEntryTries(GptEntry* e, int tries);

/* Return 1 if the entry is unused, 0 if it is used. */
int IsUnusedEntry(const GptEntry* e);

/* Returns 1 if the entry is a Chrome OS kernel partition, else 0. */
int IsKernelEntry(const GptEntry* e);

/* Copies the current kernel partition's UniquePartitionGuid to the dest */
void GetCurrentKernelUniqueGuid(GptData *gpt, void *dest);

/* Returns a pointer to text describing the passed in error */
const char* GptErrorText(int error_code);

#endif /* VBOOT_REFERENCE_CGPTLIB_INTERNAL_H_ */
