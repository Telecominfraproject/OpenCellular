/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef VBOOT_REFERENCE_CGPTLIB_H_
#define VBOOT_REFERENCE_CGPTLIB_H_

#include "gpt.h"
#include <stdint.h>

enum {
  GPT_SUCCESS = 0,
  GPT_ERROR_NO_VALID_KERNEL,
  GPT_ERROR_INVALID_HEADERS,
  GPT_ERROR_INVALID_ENTRIES,
  GPT_ERROR_INVALID_SECTOR_SIZE,
  GPT_ERROR_INVALID_SECTOR_NUMBER,
  GPT_ERROR_INVALID_UPDATE_TYPE,
};

const char *GptError(int errno);

/* Bit masks for GptData.modified field. */
#define GPT_MODIFIED_HEADER1 0x01
#define GPT_MODIFIED_HEADER2 0x02
#define GPT_MODIFIED_ENTRIES1 0x04
#define GPT_MODIFIED_ENTRIES2 0x08

/* The 'update_type' of GptUpdateKernelEntry()
 * We expose TRY and BAD only because those are what verified boot needs.
 * For more precise control on GPT attribute bits, please refer to
 * gpt_internal.h */
enum {
  GPT_UPDATE_ENTRY_TRY = 1,
  /* System will be trying to boot the currently selected kernel partition.
   * Update its try count if necessary. */
  GPT_UPDATE_ENTRY_BAD = 2,
  /* The currently selected kernel partition failed validation.  Mark entry as
   * invalid. */
};

/* Defines ChromeOS-specific limitation on GPT */
#define MIN_SIZE_OF_HEADER 92
#define MAX_SIZE_OF_HEADER 512
#define MIN_SIZE_OF_ENTRY 128
#define MAX_SIZE_OF_ENTRY 512
#define SIZE_OF_ENTRY_MULTIPLE 8
#define MIN_NUMBER_OF_ENTRIES 32
#define MAX_NUMBER_OF_ENTRIES 512
#define TOTAL_ENTRIES_SIZE 16384  /* usual case is 128 bytes * 128 entries */

/* Defines GPT sizes */
#define GPT_PMBR_SECTOR 1  /* size (in sectors) of PMBR */
#define GPT_HEADER_SECTOR 1
#define GPT_ENTRIES_SECTORS 32  /* assume sector size if 512 bytes, then
                                 *  (TOTAL_ENTRIES_SIZE / 512) = 32 */

/* alias name of index in internal array for primary and secondary header and
 * entries. */
enum {
  PRIMARY = 0,
  SECONDARY = 1,
  MASK_NONE = 0,
  MASK_PRIMARY = 1,
  MASK_SECONDARY = 2,
  MASK_BOTH = 3,
};

typedef struct {
  /* Fill in the following fields before calling GptInit() */
  uint8_t *primary_header;       /* GPT primary header, from sector 1 of disk
                                  *   (size: 512 bytes) */
  uint8_t *secondary_header;     /* GPT secondary header, from last sector of
                                  *   disk (size: 512 bytes) */
  uint8_t *primary_entries;      /* primary GPT table, follows primary header
                                  *   (size: 16 KB) */
  uint8_t *secondary_entries;    /* secondary GPT table, precedes secondary
                                  *   header (size: 16 KB) */
  uint32_t sector_bytes;         /* Size of a LBA sector, in bytes */
  uint64_t drive_sectors;        /* Size of drive in LBA sectors, in sectors */

  /* Outputs */
  uint8_t modified;              /* Which inputs have been modified?
                                  * 0x01 = header1
                                  * 0x02 = header2
                                  * 0x04 = table1
                                  * 0x08 = table2  */

  /* Internal state */
  int current_kernel; /* the current chromeos kernel index in partition table.
                       * -1 means not found on drive. */
} GptData;

int GptInit(GptData *gpt);
/* Initializes the GPT data structure's internal state.  The following fields
 * must be filled before calling this function:
 *
 *   primary_header
 *   secondary_header
 *   primary_entries
 *   secondary_entries
 *   sector_bytes
 *   drive_sectors
 *
 * On return the modified field may be set, if the GPT data has been modified
 * and should be written to disk.
 *
 * Returns GPT_SUCCESS if successful, non-zero if error:
 *   GPT_ERROR_INVALID_HEADERS, both partition table headers are invalid, enters
 *                              recovery mode,
 *   GPT_ERROR_INVALID_ENTRIES, both partition table entries are invalid, enters
 *                              recovery mode,
 *   GPT_ERROR_INVALID_SECTOR_SIZE, size of a sector is not supported,
 *   GPT_ERROR_INVALID_SECTOR_NUMBER, number of sectors in drive is invalid (too
 *                                    small) */

int GptNextKernelEntry(GptData *gpt, uint64_t *start_sector, uint64_t *size);
/* Provides the location of the next kernel partition, in order of decreasing
 * priority.  On return the start_sector parameter contains the LBA sector
 * for the start of the kernel partition, and the size parameter contains the
 * size of the kernel partition in LBA sectors.
 *
 * Returns GPT_SUCCESS if successful, else
 *   GPT_ERROR_NO_VALID_KERNEL, no avaliable kernel, enters recovery mode */

int GptUpdateKernelEntry(GptData *gpt, uint32_t update_type);
/* Updates the kernel entry with the specified index, using the specified type
 * of update (GPT_UPDATE_ENTRY_*).
 *
 * On return the modified field may be set, if the GPT data has been modified
 * and should be written to disk.
 *
 * Returns GPT_SUCCESS if successful, else
 *   GPT_ERROR_INVALID_UPDATE_TYPE, invalid 'update_type' is given.
 */

#endif  /* VBOOT_REFERENCE_CGPTLIB_H_ */
