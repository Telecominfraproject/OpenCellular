// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cgpt.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uuid/uuid.h>

#include "cgptlib_internal.h"

static void Usage(void)
{
  printf("\nUsage: %s create [OPTIONS] DRIVE\n\n"
         "Create or reset an empty GPT.\n\n"
         "Options:\n"
         "  -z           Zero the sectors of the GPT table and entries\n"
         "\n", progname);
}

int cmd_create(int argc, char *argv[]) {
  struct drive drive;
  int zap = 0;

  int c;
  int errorcnt = 0;

  opterr = 0;                     // quiet, you
  while ((c=getopt(argc, argv, ":hz")) != -1)
  {
    switch (c)
    {
    case 'z':
      zap = 1;
      break;

    case 'h':
      Usage();
      return CGPT_OK;
    case '?':
      Error("unrecognized option: -%c\n", optopt);
      errorcnt++;
      break;
    case ':':
      Error("missing argument to -%c\n", optopt);
      errorcnt++;
      break;
    default:
      errorcnt++;
      break;
    }
  }
  if (errorcnt)
  {
    Usage();
    return CGPT_FAILED;
  }

  if (optind >= argc) {
    Usage();
    return CGPT_FAILED;
  }

  if (CGPT_OK != DriveOpen(argv[optind], &drive))
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
  if (!zap)
  {
    GptHeader *h = (GptHeader *)drive.gpt.primary_header;
    memcpy(h->signature, GPT_HEADER_SIGNATURE, GPT_HEADER_SIGNATURE_SIZE);
    h->revision = GPT_HEADER_REVISION;
    h->size = sizeof(GptHeader);
    h->my_lba = 1;
    h->alternate_lba = drive.gpt.drive_sectors - 1;
    h->first_usable_lba = 1 + 1 + GPT_ENTRIES_SECTORS;
    h->last_usable_lba = drive.gpt.drive_sectors - 1 - GPT_ENTRIES_SECTORS - 1;
    uuid_generate((uint8_t *)&h->disk_uuid);
    h->entries_lba = 2;
    h->number_of_entries = 128;
    h->size_of_entry = sizeof(GptEntry);

    // Copy to secondary
    RepairHeader(&drive.gpt, MASK_PRIMARY);

    UpdateCrc(&drive.gpt);
  }

  // Write it all out
  return DriveClose(&drive, 1);
}
