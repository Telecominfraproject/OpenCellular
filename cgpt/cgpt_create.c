// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cgpt.h"

#include <string.h>

#include "cgptlib_internal.h"
#include "cgpt_params.h"

int cgpt_create(CgptCreateParams *params) {
  struct drive drive;

  if (params == NULL)
    return CGPT_FAILED;

  if (CGPT_OK != DriveOpen(params->drive_name, &drive, O_RDWR))
    return CGPT_FAILED;

  // Erase the data
  memset(drive.gpt.primary_header, 0,
         drive.gpt.sector_bytes * GPT_HEADER_SECTOR);
  memset(drive.gpt.secondary_header, 0,
         drive.gpt.sector_bytes * GPT_HEADER_SECTOR);
  memset(drive.gpt.primary_entries, 0,
         drive.gpt.sector_bytes * GPT_ENTRIES_SECTORS);
  memset(drive.gpt.secondary_entries, 0,
         drive.gpt.sector_bytes * GPT_ENTRIES_SECTORS);

  drive.gpt.modified |= (GPT_MODIFIED_HEADER1 | GPT_MODIFIED_ENTRIES1 |
                         GPT_MODIFIED_HEADER2 | GPT_MODIFIED_ENTRIES2);

  // Initialize a blank set
  if (!params->zap)
  {
    GptHeader *h = (GptHeader *)drive.gpt.primary_header;
    memcpy(h->signature, GPT_HEADER_SIGNATURE, GPT_HEADER_SIGNATURE_SIZE);
    h->revision = GPT_HEADER_REVISION;
    h->size = sizeof(GptHeader);
    h->my_lba = 1;
    h->alternate_lba = drive.gpt.drive_sectors - 1;
    h->first_usable_lba = 1 + 1 + GPT_ENTRIES_SECTORS;
    h->last_usable_lba = drive.gpt.drive_sectors - 1 - GPT_ENTRIES_SECTORS - 1;
    if (!uuid_generator) {
      Error("Unable to generate new GUID. uuid_generator not set.\n");
      goto bad;
    }
    (*uuid_generator)((uint8_t *)&h->disk_uuid);
    h->entries_lba = 2;
    h->number_of_entries = 128;
    h->size_of_entry = sizeof(GptEntry);

    // Copy to secondary
    RepairHeader(&drive.gpt, MASK_PRIMARY);

    UpdateCrc(&drive.gpt);
  }

  // Write it all out
  return DriveClose(&drive, 1);

bad:

  DriveClose(&drive, 0);
  return CGPT_FAILED;
}
