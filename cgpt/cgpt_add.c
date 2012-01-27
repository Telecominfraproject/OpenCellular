// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cgpt.h"

#include <string.h>

#include "cgptlib_internal.h"
#include "cgpt_params.h"

int cgpt_add(CgptAddParams *params) {

  struct drive drive;

  int gpt_retval;
  GptEntry *entry;
  uint32_t index;

  if (params == NULL)
    return CGPT_FAILED;

  if (CGPT_OK != DriveOpen(params->driveName, &drive))
    return CGPT_FAILED;

  if (GPT_SUCCESS != (gpt_retval = GptSanityCheck(&drive.gpt))) {
    Error("GptSanityCheck() returned %d: %s\n",
          gpt_retval, GptError(gpt_retval));
    return CGPT_FAILED;
  }

  if (((drive.gpt.valid_headers & MASK_BOTH) != MASK_BOTH) ||
      ((drive.gpt.valid_entries & MASK_BOTH) != MASK_BOTH)) {
    Error("one of the GPT header/entries is invalid.\n"
          "please run 'cgpt repair' before adding anything.\n");
    return CGPT_FAILED;
  }

  uint32_t max_part = GetNumberOfEntries(&drive.gpt);
  if (params->partition) {
    if (params->partition > max_part) {
      Error("invalid partition number: %d\n", params->partition);
      goto bad;
    }
    index = params->partition - 1;
    entry = GetEntry(&drive.gpt, PRIMARY, index);
  } else {
    // find next empty partition
    for (index = 0; index < max_part; index++) {
      entry = GetEntry(&drive.gpt, PRIMARY, index);
      if (IsZero(&entry->type)) {
        params->partition = index + 1;
        break;
      }
    }
    if (index >= max_part) {
      Error("no unused partitions available\n");
      goto bad;
    }
  }

  // New partitions must specify type, begin, and size.
  if (IsZero(&entry->type)) {
    if (!params->set_begin || !params->set_size || !params->set_type) {
      Error("-t, -b, and -s options are required for new partitions\n");
      goto bad;
    }
    if (IsZero(&params->type_guid)) {
      Error("New partitions must have a type other than \"unused\"\n");
      goto bad;
    }
    if (!params->set_unique)
      uuid_generate((uint8_t *)&entry->unique);
  }

  if (params->set_begin)
    entry->starting_lba = params->begin;
  if (params->set_size)
    entry->ending_lba = params->begin + params->size - 1;
  if (params->set_type)
    memcpy(&entry->type, &params->type_guid, sizeof(Guid));
  if (params->set_unique)
    memcpy(&entry->unique, &params->unique_guid, sizeof(Guid));
  if (params->label) {
    if (CGPT_OK != UTF8ToUTF16((uint8_t *)params->label, entry->name,
                               sizeof(entry->name) / sizeof(entry->name[0]))) {
      Error("The label cannot be converted to UTF16.\n");
      goto bad;
    }
  }
  if (params->set_raw) {
    entry->attrs.fields.gpt_att = params->raw_value;
  } else {
    if (params->set_successful)
      SetSuccessful(&drive.gpt, PRIMARY, index, params->successful);
    if (params->set_tries)
      SetTries(&drive.gpt, PRIMARY, index, params->tries);
    if (params->set_priority)
      SetPriority(&drive.gpt, PRIMARY, index, params->priority);
  }

  RepairEntries(&drive.gpt, MASK_PRIMARY);
  RepairHeader(&drive.gpt, MASK_PRIMARY);

  drive.gpt.modified |= (GPT_MODIFIED_HEADER1 | GPT_MODIFIED_ENTRIES1 |
                         GPT_MODIFIED_HEADER2 | GPT_MODIFIED_ENTRIES2);
  UpdateCrc(&drive.gpt);

  // Write it all out
  return DriveClose(&drive, 1);

bad:
  (void) DriveClose(&drive, 0);
  return CGPT_FAILED;
}
