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
  printf("\nUsage: %s add [OPTIONS] DRIVE\n\n"
         "Add, edit, or remove a partition entry.\n\n"
         "Options:\n"
         "  -i NUM       Specify partition (default is next available)\n"
         "  -b NUM       Beginning sector\n"
         "  -s NUM       Size in sectors\n"
         "  -t GUID      Partition Type GUID\n"
         "  -u GUID      Partition Unique ID\n"
         "  -l LABEL     Label\n"
         "  -S NUM       set Successful flag (0|1)\n"
         "  -T NUM       set Tries flag (0-15)\n"
         "  -P NUM       set Priority flag (0-15)\n"
         "  -A NUM       set raw 64-bit attribute value\n"
         "\n"
         "Use the -i option to modify an existing partition.\n"
         "The -b, -s, and -t options must be given for new partitions.\n"
         "\n", progname);
  PrintTypes();
}

int cmd_add(int argc, char *argv[]) {
  struct drive drive;
  uint32_t partition = 0;
  uint64_t begin = 0;
  uint64_t size = 0;
  Guid type_guid;
  Guid unique_guid;
  char *label = 0;
  int successful = 0;
  int tries = 0;
  int priority = 0;
  uint16_t raw_value = 0;
  int set_begin = 0;
  int set_size = 0;
  int set_type = 0;
  int set_unique = 0;
  int set_successful = 0;
  int set_tries = 0;
  int set_priority = 0;
  int set_raw = 0;

  int gpt_retval;
  GptEntry *entry;
  uint32_t index;

  int c;
  int errorcnt = 0;
  char *e = 0;

  opterr = 0;                     // quiet, you
  while ((c=getopt(argc, argv, ":hi:b:s:t:u:l:S:T:P:A:")) != -1)
  {
    switch (c)
    {
    case 'i':
      partition = (uint32_t)strtoul(optarg, &e, 0);
      if (!*optarg || (e && *e))
      {
        Error("invalid argument to -%c: \"%s\"\n", c, optarg);
        errorcnt++;
      }
      break;
    case 'b':
      set_begin = 1;
      begin = strtoull(optarg, &e, 0);
      if (!*optarg || (e && *e))
      {
        Error("invalid argument to -%c: \"%s\"\n", c, optarg);
        errorcnt++;
      }
      break;
    case 's':
      set_size = 1;
      size = strtoull(optarg, &e, 0);
      if (!*optarg || (e && *e))
      {
        Error("invalid argument to -%c: \"%s\"\n", c, optarg);
        errorcnt++;
      }
      break;
    case 't':
      set_type = 1;
      if (CGPT_OK != SupportedType(optarg, &type_guid) &&
          CGPT_OK != StrToGuid(optarg, &type_guid)) {
        Error("invalid argument to -%c: %s\n", c, optarg);
        errorcnt++;
      }
      break;
    case 'u':
      set_unique = 1;
      if (CGPT_OK != StrToGuid(optarg, &unique_guid)) {
        Error("invalid argument to -%c: %s\n", c, optarg);
        errorcnt++;
      }
      break;
    case 'l':
      label = optarg;
      break;
    case 'S':
      set_successful = 1;
      successful = (uint32_t)strtoul(optarg, &e, 0);
      if (!*optarg || (e && *e))
      {
        Error("invalid argument to -%c: \"%s\"\n", c, optarg);
        errorcnt++;
      }
      if (successful < 0 || successful > 1) {
        Error("value for -%c must be between 0 and 1", c);
        errorcnt++;
      }
      break;
    case 'T':
      set_tries = 1;
      tries = (uint32_t)strtoul(optarg, &e, 0);
      if (!*optarg || (e && *e))
      {
        fprintf(stderr, "%s: invalid argument to -%c: \"%s\"\n",
                progname, c, optarg);
        errorcnt++;
      }
      if (tries < 0 || tries > 15) {
        Error("value for -%c must be between 0 and 15", c);
        errorcnt++;
      }
      break;
    case 'P':
      set_priority = 1;
      priority = (uint32_t)strtoul(optarg, &e, 0);
      if (!*optarg || (e && *e))
      {
        Error("invalid argument to -%c: \"%s\"\n", c, optarg);
        errorcnt++;
      }
      if (priority < 0 || priority > 15) {
        Error("value for -%c must be between 0 and 15", c);
        errorcnt++;
      }
      break;
    case 'A':
      set_raw = 1;
      raw_value = strtoull(optarg, &e, 0);
      if (!*optarg || (e && *e))
      {
        Error("invalid argument to -%c: \"%s\"\n", c, optarg);
        errorcnt++;
      }
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
    Error("missing drive argument\n");
    return CGPT_FAILED;
  }

  if (CGPT_OK != DriveOpen(argv[optind], &drive))
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
  if (partition) {
    if (partition > max_part) {
      Error("invalid partition number: %d\n", partition);
      goto bad;
    }
    index = partition - 1;
    entry = GetEntry(&drive.gpt, PRIMARY, index);
  } else {
    // find next empty partition
    for (index = 0; index < max_part; index++) {
      entry = GetEntry(&drive.gpt, PRIMARY, index);
      if (IsZero(&entry->type)) {
        partition = index + 1;
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
    if (!set_begin || !set_size || !set_type) {
      Error("-t, -b, and -s options are required for new partitions\n");
      goto bad;
    }
    if (IsZero(&type_guid)) {
      Error("New partitions must have a type other than \"unused\"\n");
      goto bad;
    }
    if (!set_unique)
      uuid_generate((uint8_t *)&entry->unique);
  }

  if (set_begin)
    entry->starting_lba = begin;
  if (set_size)
    entry->ending_lba = begin + size - 1;
  if (set_type)
    memcpy(&entry->type, &type_guid, sizeof(Guid));
  if (set_unique)
    memcpy(&entry->unique, &unique_guid, sizeof(Guid));
  if (label) {
    if (CGPT_OK != UTF8ToUTF16((uint8_t *)label, entry->name,
                               sizeof(entry->name) / sizeof(entry->name[0]))) {
      Error("The label cannot be converted to UTF16.\n");
      goto bad;
    }
  }
  if (set_raw) {
    entry->attrs.fields.gpt_att = raw_value;
  } else {
    if (set_successful)
      SetSuccessful(&drive.gpt, PRIMARY, index, successful);
    if (set_tries)
      SetTries(&drive.gpt, PRIMARY, index, tries);
    if (set_priority)
      SetPriority(&drive.gpt, PRIMARY, index, priority);
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
