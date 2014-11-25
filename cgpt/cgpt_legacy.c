// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string.h>

#include "cgpt.h"
#include "cgptlib_internal.h"
#include "vboot_host.h"

int CgptLegacy(CgptLegacyParams *params) {
  struct drive drive;
  GptHeader *h1, *h2;

  if (params == NULL)
    return CGPT_FAILED;

  if (CGPT_OK != DriveOpen(params->drive_name, &drive, O_RDWR,
                           params->drive_size))
    return CGPT_FAILED;

  h1 = (GptHeader *)drive.gpt.primary_header;
  h2 = (GptHeader *)drive.gpt.secondary_header;
  if (params->efipart) {
    memcpy(h1->signature, GPT_HEADER_SIGNATURE, GPT_HEADER_SIGNATURE_SIZE);
    memcpy(h2->signature, GPT_HEADER_SIGNATURE, GPT_HEADER_SIGNATURE_SIZE);
    RepairEntries(&drive.gpt, MASK_SECONDARY);
    drive.gpt.modified |= (GPT_MODIFIED_HEADER1 | GPT_MODIFIED_ENTRIES1 |
                           GPT_MODIFIED_HEADER2);
  } else {
    memcpy(h1->signature, GPT_HEADER_SIGNATURE2, GPT_HEADER_SIGNATURE_SIZE);
    memcpy(h2->signature, GPT_HEADER_SIGNATURE2, GPT_HEADER_SIGNATURE_SIZE);
    memset(drive.gpt.primary_entries, 0, drive.gpt.sector_bytes);
    drive.gpt.modified |= (GPT_MODIFIED_HEADER1 | GPT_MODIFIED_ENTRIES1 |
                           GPT_MODIFIED_HEADER2);
  }

  UpdateCrc(&drive.gpt);

  // Write it all out
  return DriveClose(&drive, 1);
}
