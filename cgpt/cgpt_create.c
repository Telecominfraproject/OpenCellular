// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


#include <string.h>

#include "cgpt.h"
#include "cgptlib_internal.h"
#include "vboot_host.h"

static void AllocAndClear(uint8_t **buf, uint64_t size) {
  if (*buf) {
    memset(*buf, 0, size);
  } else {
    *buf = calloc(1, size);
    if (!*buf) {
      Error("Cannot allocate %u bytes.\n", size);
      abort();
    }
  }
}

static int GptCreate(struct drive *drive, CgptCreateParams *params) {
  // Allocate and/or erase the data.
  // We cannot assume the GPT headers or entry arrays have been allocated
  // by GptLoad() because those fields might have failed validation checks.
  AllocAndClear(&drive->gpt.primary_header,
                drive->gpt.sector_bytes * GPT_HEADER_SECTORS);
  AllocAndClear(&drive->gpt.secondary_header,
                drive->gpt.sector_bytes * GPT_HEADER_SECTORS);
  AllocAndClear(&drive->gpt.primary_entries,
                drive->gpt.sector_bytes * GPT_ENTRIES_SECTORS);
  AllocAndClear(&drive->gpt.secondary_entries,
                drive->gpt.sector_bytes * GPT_ENTRIES_SECTORS);

  drive->gpt.modified |= (GPT_MODIFIED_HEADER1 | GPT_MODIFIED_ENTRIES1 |
                         GPT_MODIFIED_HEADER2 | GPT_MODIFIED_ENTRIES2);

  // Initialize a blank set
  if (!params->zap) {
    GptHeader *h = (GptHeader *)drive->gpt.primary_header;
    memcpy(h->signature, GPT_HEADER_SIGNATURE, GPT_HEADER_SIGNATURE_SIZE);
    h->revision = GPT_HEADER_REVISION;
    h->size = sizeof(GptHeader);
    h->my_lba = GPT_PMBR_SECTORS;  /* The second sector on drive. */
    h->alternate_lba = drive->gpt.gpt_drive_sectors - GPT_HEADER_SECTORS;
    h->entries_lba = h->my_lba + GPT_HEADER_SECTORS;
    if (drive->gpt.stored_on_device == GPT_STORED_ON_DEVICE) {
      h->entries_lba += params->padding;
      h->first_usable_lba = h->entries_lba + GPT_ENTRIES_SECTORS;
      h->last_usable_lba = (drive->gpt.drive_sectors - GPT_HEADER_SECTORS -
                            GPT_ENTRIES_SECTORS - 1);
    } else {
      h->first_usable_lba = params->padding;
      h->last_usable_lba = (drive->gpt.drive_sectors - 1);
    }
    if (CGPT_OK != GenerateGuid(&h->disk_uuid)) {
      Error("Unable to generate new GUID.\n");
      return -1;
    }
    h->size_of_entry = sizeof(GptEntry);
    h->number_of_entries = TOTAL_ENTRIES_SIZE / h->size_of_entry;
    if (drive->gpt.stored_on_device != GPT_STORED_ON_DEVICE) {
      // We might have smaller space for the GPT table. Scale accordingly.
      size_t half_size_sectors = drive->gpt.gpt_drive_sectors / 2;
      if (half_size_sectors < GPT_HEADER_SECTORS) {
        Error("Not enough space for a GPT header.\n");
        return -1;
      }
      half_size_sectors -= GPT_HEADER_SECTORS;
      size_t half_size = half_size_sectors * drive->gpt.sector_bytes;
      if (half_size < (MIN_NUMBER_OF_ENTRIES * h->size_of_entry)) {
        Error("Not enough space for minimum number of entries.\n");
        return -1;
      }
      if (128 > (half_size / h->size_of_entry)) {
        h->number_of_entries = half_size / h->size_of_entry;
      }
    }

    // Copy to secondary
    RepairHeader(&drive->gpt, MASK_PRIMARY);

    UpdateCrc(&drive->gpt);
  }

  return 0;
}

int CgptCreate(CgptCreateParams *params) {
  struct drive drive;

  if (params == NULL)
    return CGPT_FAILED;

  if (CGPT_OK != DriveOpen(params->drive_name, &drive, O_RDWR,
                           params->drive_size))
    return CGPT_FAILED;

  if (GptCreate(&drive, params))
    goto bad;

  // Write it all out
  return DriveClose(&drive, 1);

bad:

  DriveClose(&drive, 0);
  return CGPT_FAILED;
}
