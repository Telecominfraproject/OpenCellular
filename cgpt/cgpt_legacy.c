// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string.h>

#include "cgpt.h"
#include "cgptlib_internal.h"
#include "vboot_host.h"

int CgptLegacy(CgptLegacyParams *params) {
  struct drive drive;
  int gpt_retval;
  GptHeader *h1, *h2;

  if (params == NULL)
    return CGPT_FAILED;

  if (CGPT_OK != DriveOpen(params->drive_name, &drive, O_RDWR,
                           params->drive_size))
    return CGPT_FAILED;

  if (GPT_SUCCESS != (gpt_retval = GptSanityCheck(&drive.gpt))) {
    Error("GptSanityCheck() returned %d: %s\n",
          gpt_retval, GptError(gpt_retval));
    return CGPT_FAILED;
  }

  h1 = (GptHeader *)drive.gpt.primary_header;
  h2 = (GptHeader *)drive.gpt.secondary_header;
  if (params->mode == CGPT_LEGACY_MODE_EFIPART) {
    drive.gpt.ignored = MASK_NONE;
    memcpy(h1->signature, GPT_HEADER_SIGNATURE, GPT_HEADER_SIGNATURE_SIZE);
    memcpy(h2->signature, GPT_HEADER_SIGNATURE, GPT_HEADER_SIGNATURE_SIZE);
    RepairEntries(&drive.gpt, MASK_SECONDARY);
    drive.gpt.modified |= (GPT_MODIFIED_HEADER1 | GPT_MODIFIED_ENTRIES1 |
                           GPT_MODIFIED_HEADER2);
  } else if (params->mode == CGPT_LEGACY_MODE_IGNORE_PRIMARY) {
    if (!(drive.gpt.valid_headers & MASK_SECONDARY) ||
        !(drive.gpt.valid_entries & MASK_SECONDARY) ||
        drive.gpt.ignored & MASK_SECONDARY) {
      Error("Refusing to mark primary GPT ignored unless secondary is valid.");
      return CGPT_FAILED;
    }
    memset(h1, 0, sizeof(*h1));
    memcpy(h1->signature, GPT_HEADER_SIGNATURE_IGNORED,
           GPT_HEADER_SIGNATURE_SIZE);
    drive.gpt.modified |= GPT_MODIFIED_HEADER1;
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
