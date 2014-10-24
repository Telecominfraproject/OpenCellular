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
      h->first_usable_lba = 0;
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
      size_t half_size = drive->flash_size / 2;
      size_t header_block_size = GPT_HEADER_SECTORS * drive->gpt.sector_bytes;
      if (half_size < header_block_size) {
        Error("Not enough space for a GPT header.\n");
        return -1;
      }
      half_size -= header_block_size;
      if (half_size < MIN_NUMBER_OF_ENTRIES * h->size_of_entry) {
        Error("Not enough space for minimum number of entries.\n");
        return -1;
      }
      if (128 > half_size / h->size_of_entry) {
        h->number_of_entries = half_size / h->size_of_entry;
      }
    }

    // Copy to secondary
    RepairHeader(&drive->gpt, MASK_PRIMARY);

    UpdateCrc(&drive->gpt);
  }

  return 0;
}

static int MtdCreate(struct drive *drive, CgptCreateParams *params) {
  MtdDiskLayout *h = &drive->mtd.primary;
  memset(h, 0, sizeof(*h));
  drive->mtd.modified = 1;

  if (!params->zap) {
    // Prep basic parameters
    memcpy(h->signature, MTD_DRIVE_SIGNATURE, sizeof(h->signature));
    h->size = sizeof(*h);
    h->first_offset = 0;
    h->last_offset = (drive->mtd.drive_sectors * drive->mtd.sector_bytes) - 1;
    h->crc32 = MtdHeaderCrc(h);
  }
  if (params->size) {
    h->last_offset = params->size - 1;
    drive->size = params->size;
    drive->mtd.drive_sectors = drive->size / drive->mtd.sector_bytes;
  } else if (!drive->mtd.drive_sectors) {
    Error("MTD create with params->size == 0 && drive->mtd.drive_sectors == 0");
    return -1;
  }

  return 0;
}

int CgptCreate(CgptCreateParams *params) {
  struct drive drive;

  if (params == NULL)
    return CGPT_FAILED;

  if (CGPT_OK != DriveOpen(params->drive_name, &drive, O_RDWR))
    return CGPT_FAILED;

  if (drive.is_mtd) {
    if (MtdCreate(&drive, params))
      goto bad;
  } else {
    if (GptCreate(&drive, params))
      goto bad;
  }

  // Write it all out
  return DriveClose(&drive, 1);

bad:

  DriveClose(&drive, 0);
  return CGPT_FAILED;
}
