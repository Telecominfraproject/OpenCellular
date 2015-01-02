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
    if (CGPT_OK != GenerateGuid(&h->disk_uuid)) {
      Error("Unable to generate new GUID.\n");
      return -1;
    }

    /* Calculate number of entries */
    h->size_of_entry = sizeof(GptEntry);
    h->number_of_entries = MAX_NUMBER_OF_ENTRIES;
    if (drive->gpt.flags & GPT_FLAG_EXTERNAL) {
      // We might have smaller space for the GPT table. Scale accordingly.
      //
      // +------+------------+---------------+-----+--------------+-----------+
      // | PMBR | Prim. Head | Prim. Entries | ... | Sec. Entries | Sec. Head |
      // +------+------------+---------------+-----+--------------+-----------+
      //
      // Half the size of gpt_drive_sectors must be big enough to hold PMBR +
      // GPT Header + Entries Table, though the secondary structures do not
      // contain PMBR.
      size_t required_headers_size =
          (GPT_PMBR_SECTORS + GPT_HEADER_SECTORS) * drive->gpt.sector_bytes;
      size_t min_entries_size = MIN_NUMBER_OF_ENTRIES * h->size_of_entry;
      size_t required_min_size = required_headers_size + min_entries_size;
      size_t half_size =
          (drive->gpt.gpt_drive_sectors / 2) * drive->gpt.sector_bytes;
      if (half_size < required_min_size) {
        Error("Not enough space to store GPT structures. Required %d bytes.\n",
              required_min_size * 2);
        return -1;
      }
      size_t max_entries =
          (half_size - required_headers_size) / h->size_of_entry;
      if (h->number_of_entries > max_entries) {
        h->number_of_entries = max_entries;
      }
    }

    /* Then use number of entries to calculate entries_lba. */
    h->entries_lba = h->my_lba + GPT_HEADER_SECTORS;
    if (!(drive->gpt.flags & GPT_FLAG_EXTERNAL)) {
      h->entries_lba += params->padding;
      h->first_usable_lba = h->entries_lba + CalculateEntriesSectors(h);
      h->last_usable_lba = (drive->gpt.streaming_drive_sectors - GPT_HEADER_SECTORS -
                            CalculateEntriesSectors(h) - 1);
    } else {
      h->first_usable_lba = params->padding;
      h->last_usable_lba = (drive->gpt.streaming_drive_sectors - 1);
    }

    size_t entries_size = h->number_of_entries * h->size_of_entry;
    AllocAndClear(&drive->gpt.primary_entries, entries_size);
    AllocAndClear(&drive->gpt.secondary_entries, entries_size);

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
