// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


#include <string.h>

#include "cgpt.h"
#include "cgptlib_internal.h"
#include "vboot_host.h"

int CgptRepair(CgptRepairParams *params) {
  struct drive drive;

  if (params == NULL)
    return CGPT_FAILED;

  if (CGPT_OK != DriveOpen(params->drive_name, &drive, O_RDWR,
                           params->drive_size))
    return CGPT_FAILED;

  int gpt_retval = GptSanityCheck(&drive.gpt);
  if (params->verbose)
    printf("GptSanityCheck() returned %d: %s\n",
           gpt_retval, GptError(gpt_retval));

  GptHeader *header;
  if (MASK_PRIMARY == drive.gpt.valid_headers ||
      MASK_BOTH == drive.gpt.valid_headers) {
    header = (GptHeader *)(drive.gpt.primary_header);
  } else {
    header = (GptHeader *)(drive.gpt.secondary_header);
  }

  if (MASK_PRIMARY == drive.gpt.valid_entries) {
    free(drive.gpt.secondary_entries);
    drive.gpt.secondary_entries =
        malloc(header->size_of_entry * header->number_of_entries);
  } else if (MASK_SECONDARY == drive.gpt.valid_entries) {
    free(drive.gpt.primary_entries);
    drive.gpt.primary_entries =
        malloc(header->size_of_entry * header->number_of_entries);
  }

  GptRepair(&drive.gpt);
  if (drive.gpt.modified & GPT_MODIFIED_HEADER1)
    printf("Primary Header is updated.\n");
  if (drive.gpt.modified & GPT_MODIFIED_ENTRIES1)
    printf("Primary Entries is updated.\n");
  if (drive.gpt.modified & GPT_MODIFIED_ENTRIES2)
    printf("Secondary Entries is updated.\n");
  if (drive.gpt.modified & GPT_MODIFIED_HEADER2)
    printf("Secondary Header is updated.\n");

  return DriveClose(&drive, 1);
}
