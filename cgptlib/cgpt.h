/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef VBOOT_REFERENCE_CGPT_H_
#define VBOOT_REFERENCE_CGPT_H_

#include <stdint.h>

enum {
        GPT_ERROR_INVALID_HEADERS = 1,
        GPT_ERROR_INVALID_ENTRIES,
        GPT_ERROR_INVALID_SECTOR_SIZE,
        GPT_ERROR_INVALID_SECTOR_NUMBER,
};

#define GPT_MODIFIED_HEADER1 0x01
#define GPT_MODIFIED_HEADER2 0x02
#define GPT_MODIFIED_ENTRIES1 0x04
#define GPT_MODIFIED_ENTRIES2 0x08

#define GPT_UPDATE_ENTRY_TRY 1
  /* System will be trying to boot the currently selected kernel partition.
   * Update its try count if necessary. */
#define GPT_UPDATE_ENTRY_BAD 2
  /* The currently selected kernel partition failed validation.  Mark entry as
   * invalid. */

struct GPTData {
        /* Fill in the following fields before calling GPTInit() */
        uint8_t *header1;       /* GPT primary header, from sector 1 of disk
                                 *   (size: 512 bytes) */
	uint8_t *header2;       /* GPT secondary header, from last sector of
                                 *   disk (size: 512 bytes) */
	uint8_t *entries1;      /* primary GPT table, follows primary header
                                 *   (size: 16 KB) */
	uint8_t *entries2;      /* secondary GPT table, precedes secondary
                                 *   header (size: 16 KB) */
	uint32_t sector_bytes;  /* Size of a LBA sector, in bytes */
	uint64_t drive_sectors; /* Size of drive in LBA sectors, in sectors */

	/* Outputs */
	uint8_t modified;       /* Which inputs have been modified?
	                         * 0x01 = header1
	                         * 0x02 = header2
	                         * 0x04 = table1
	                         * 0x08 = table2  */

        /* Internal state */
        uint8_t current_kernel; // the current kernel index
};
typedef struct GPTData GPTData_t;

int GPTInit(GPTData_t *gpt);
/* Initializes the GPT data structure's internal state.  The header1, header2,
 * table1, table2, and drive_size fields should be filled in first.
 *
 * On return the modified field may be set, if the GPT data has been modified
 * and should be written to disk.
 *
 * Returns 0 if successful, non-zero if error:
 *   GPT_ERROR_INVALID_HEADERS, both partition table headers are invalid, enters
 *                              recovery mode,
 *   GPT_ERROR_INVALID_ENTRIES, both partition table entries are invalid, enters
 *                              recovery mode,
 *   GPT_ERROR_INVALID_SECTOR_SIZE, size of a sector is not supported,
 *   GPT_ERROR_INVALID_SECTOR_NUMBER, number of sectors in drive is invalid (too
 *                                    small) */

int GPTNextKernelEntry(GPTData_t *gpt, uint64_t *start_sector, uint64_t *size);
/* Provides the location of the next kernel partition, in order of decreasing
 * priority.  On return the start_sector parameter contains the LBA sector
 * for the start of the kernel partition, and the size parameter contains the
 * size of the kernel partition in LBA sectors.
 *
 * Returns 0 if successful, 1 if error or no more sectors.  */

int GPTUpdateKernelEntry(GPTData_t *gpt, uint32_t update_type);
/* Updates the kernel entry with the specified index, using the specified type
 * of update (GPT_UPDATE_ENTRY_*).
 *
 * On return the modified field may be set, if the GPT data has been modified
 * and should be written to disk.
 *
 * Returns 0 if successful, 1 if error. */

#endif  // VBOOT_REFERENCE_CGPT_H_
