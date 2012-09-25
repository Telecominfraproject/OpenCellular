// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cgpt.h"

#include <string.h>

#include "cgptlib_internal.h"
#include "cgpt_params.h"


static const char* DumpCgptAddParams(const CgptAddParams *params) {
  static char buf[256];
  char tmp[64];

  buf[0] = 0;
  snprintf(tmp, sizeof(tmp), "-i %d ", params->partition);
  strncat(buf, tmp, sizeof(buf));
  if (params->label) {
    snprintf(tmp, sizeof(tmp), "-l %s ", params->label);
    strncat(buf, tmp, sizeof(buf));
  }
  if (params->set_begin) {
    snprintf(tmp, sizeof(tmp), "-b %llu ", (unsigned long long)params->begin);
    strncat(buf, tmp, sizeof(buf));
  }
  if (params->set_size) {
    snprintf(tmp, sizeof(tmp), "-s %llu ", (unsigned long long)params->size);
    strncat(buf, tmp, sizeof(buf));
  }
  if (params->set_type) {
    GuidToStr(&params->type_guid, tmp, sizeof(tmp));
    strncat(buf, "-t ", sizeof(buf));
    strncat(buf, tmp, sizeof(buf));
    strncat(buf, " ", sizeof(buf));
  }
  if (params->set_unique) {
    GuidToStr(&params->unique_guid, tmp, sizeof(tmp));
    strncat(buf, "-u ", sizeof(buf));
    strncat(buf, tmp, sizeof(buf));
    strncat(buf, " ", sizeof(buf));
  }
  if (params->set_successful) {
    snprintf(tmp, sizeof(tmp), "-S %d ", params->successful);
    strncat(buf, tmp, sizeof(buf));
  }
  if (params->set_tries) {
    snprintf(tmp, sizeof(tmp), "-T %d ", params->tries);
    strncat(buf, tmp, sizeof(buf));
  }
  if (params->set_priority) {
    snprintf(tmp, sizeof(tmp), "-P %d ", params->priority);
    strncat(buf, tmp, sizeof(buf));
  }
  if (params->set_raw) {
    snprintf(tmp, sizeof(tmp), "-A 0x%x ", params->raw_value);
    strncat(buf, tmp, sizeof(buf));
  }

  strncat(buf, "\n", sizeof(buf));
  return buf;
}

// This is an internal helper function which assumes no NULL args are passed.
// It sets the given attribute values for a single entry at the given index.
static void set_entry_attributes(struct drive drive,
                                 GptEntry *entry,
                                 uint32_t index,
                                 CgptAddParams *params) {
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
}

// Set the attributes such as is_successful, num_tries_left, priority, etc.
// from the given values in params.
int cgpt_set_attributes(CgptAddParams *params) {
  struct drive drive;

  int gpt_retval;
  GptEntry *entry;
  uint32_t index;

  if (params == NULL)
    return CGPT_FAILED;

  if (CGPT_OK != DriveOpen(params->drive_name, &drive, O_RDWR))
    return CGPT_FAILED;

  if (GPT_SUCCESS != (gpt_retval = GptSanityCheck(&drive.gpt))) {
    Error("GptSanityCheck() returned %d: %s\n",
          gpt_retval, GptError(gpt_retval));
    goto bad;
  }

  if (((drive.gpt.valid_headers & MASK_BOTH) != MASK_BOTH) ||
      ((drive.gpt.valid_entries & MASK_BOTH) != MASK_BOTH)) {
    Error("one of the GPT header/entries is invalid.\n"
          "please run 'cgpt repair' before adding anything.\n");
    goto bad;
  }

  if (params->partition == 0) {
    Error("invalid partition number: %d\n", params->partition);
    goto bad;
  }

  uint32_t max_part = GetNumberOfEntries(&drive.gpt);
  if (params->partition > max_part) {
    Error("invalid partition number: %d\n", params->partition);
    goto bad;
  }

  index = params->partition - 1;
  entry = GetEntry(&drive.gpt, PRIMARY, index);

  set_entry_attributes(drive, entry, index, params);

  RepairEntries(&drive.gpt, MASK_PRIMARY);
  RepairHeader(&drive.gpt, MASK_PRIMARY);

  drive.gpt.modified |= (GPT_MODIFIED_HEADER1 | GPT_MODIFIED_ENTRIES1 |
                         GPT_MODIFIED_HEADER2 | GPT_MODIFIED_ENTRIES2);
  UpdateCrc(&drive.gpt);

  // Write it all out.
  return DriveClose(&drive, 1);

bad:
  DriveClose(&drive, 0);
  return CGPT_FAILED;
}

// This method gets the partition details such as the attributes, the
// guids of the partitions, etc. Input is the partition number or the
// unique id of the partition. Output is populated in the respective
// fields of params.
int cgpt_get_partition_details(CgptAddParams *params) {
  struct drive drive;

  int gpt_retval;
  GptEntry *entry;
  uint32_t index;
  int result = CGPT_FAILED;

  if (params == NULL)
    return result;

  if (CGPT_OK != DriveOpen(params->drive_name, &drive, O_RDWR)) {
    Error("Unable to open drive: %s\n", params->drive_name);
    return result;
  }

  if (GPT_SUCCESS != (gpt_retval = GptSanityCheck(&drive.gpt))) {
    Error("GptSanityCheck() returned %d: %s\n",
          gpt_retval, GptError(gpt_retval));
    goto bad;
  }

  if (((drive.gpt.valid_headers & MASK_BOTH) != MASK_BOTH) ||
      ((drive.gpt.valid_entries & MASK_BOTH) != MASK_BOTH)) {
    Error("one of the GPT header/entries is invalid.\n"
          "please run 'cgpt repair' before adding anything.\n");
    goto bad;
  }

  uint32_t max_part = GetNumberOfEntries(&drive.gpt);

  if (params->partition) {
    if (params->partition > max_part) {
      Error("invalid partition number: %d\n", params->partition);
      goto bad;
    }

    // A valid partition number has been specified, so get the entry directly.
    index = params->partition - 1;
    entry = GetEntry(&drive.gpt, PRIMARY, index);
  } else {
    // Partition number is not specified, try looking up by the unique id.
    if (!params->set_unique) {
      Error("either partition or unique_id must be specified\n");
      goto bad;
    }

    // A unique id is specified. find the entry that matches it.
    for (index = 0; index < max_part; index++) {
      entry = GetEntry(&drive.gpt, PRIMARY, index);
      if (GuidEqual(&entry->unique, &params->unique_guid)) {
        params->partition = index + 1;
        break;
      }
    }

    if (index >= max_part) {
      Error("no partitions with the given unique id available\n");
      goto bad;
    }
  }

  // At this point, irrespective of whether a partition number is specified
  // or a unique id is specified, we have valid non-null values for all these:
  // index, entry, params->partition.

  params->begin = entry->starting_lba;
  params->size =  entry->ending_lba - entry->starting_lba + 1;
  memcpy(&params->type_guid, &entry->type, sizeof(Guid));
  memcpy(&params->unique_guid, &entry->unique, sizeof(Guid));

  params->raw_value = entry->attrs.fields.gpt_att;
  params->successful = GetSuccessful(&drive.gpt, PRIMARY, index);
  params->tries = GetTries(&drive.gpt, PRIMARY, index);
  params->priority = GetPriority(&drive.gpt, PRIMARY, index);
  result = CGPT_OK;

bad:
  DriveClose(&drive, 0);
  return result;
}


int cgpt_add(CgptAddParams *params) {
  struct drive drive;

  int gpt_retval;
  GptEntry *entry, backup;
  uint32_t index;
  int rv;

  if (params == NULL)
    return CGPT_FAILED;

  if (CGPT_OK != DriveOpen(params->drive_name, &drive, O_RDWR))
    return CGPT_FAILED;

  if (GPT_SUCCESS != (gpt_retval = GptSanityCheck(&drive.gpt))) {
    Error("GptSanityCheck() returned %d: %s\n",
          gpt_retval, GptError(gpt_retval));
    goto bad;
  }

  if (((drive.gpt.valid_headers & MASK_BOTH) != MASK_BOTH) ||
      ((drive.gpt.valid_entries & MASK_BOTH) != MASK_BOTH)) {
    Error("one of the GPT header/entries is invalid.\n"
          "please run 'cgpt repair' before adding anything.\n");
    goto bad;
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
    // Find next empty partition.
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
  memcpy(&backup, entry, sizeof(backup));

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
      if (!uuid_generator) {
        Error("Unable to generate new GUID. uuid_generator not set.\n");
        goto bad;
      }
      (*uuid_generator)((uint8_t *)&entry->unique);
  }

  if (params->set_begin)
    entry->starting_lba = params->begin;
  if (params->set_size)
    entry->ending_lba = entry->starting_lba + params->size - 1;
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

  set_entry_attributes(drive, entry, index, params);

  RepairEntries(&drive.gpt, MASK_PRIMARY);
  RepairHeader(&drive.gpt, MASK_PRIMARY);

  drive.gpt.modified |= (GPT_MODIFIED_HEADER1 | GPT_MODIFIED_ENTRIES1 |
                         GPT_MODIFIED_HEADER2 | GPT_MODIFIED_ENTRIES2);
  UpdateCrc(&drive.gpt);

  rv = CheckEntries((GptEntry*)drive.gpt.primary_entries,
                    (GptHeader*)drive.gpt.primary_header);

  if (0 != rv) {
    // If the modified entry is illegal, recover it and return error.
    memcpy(entry, &backup, sizeof(*entry));
    Error("%s\n", GptErrorText(rv));
    Error(DumpCgptAddParams(params));
    goto bad;
  }

  // Write it all out.
  return DriveClose(&drive, 1);

bad:
  DriveClose(&drive, 0);
  return CGPT_FAILED;
}
