/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Update GPT attribute bits.
 */
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include "cgpt.h"
#include "cgptlib_internal.h"
#include "cgpt_tofix.h"
#include "utility.h"

static struct number_range
    range_127_0 = {127, 0};

/* Integers to store parsed argument. */
static int help, partition, begin_lba, size_lba;
static char type[128], unique[128], name[128];

/* The structure for getopt_long(). When you add/delete any line, please refine
 * attribute_comments[] and third parameter of getopt_long() too.  */
static struct option adm_options[] = {
  {.name = "help", .has_arg = no_argument, .flag = 0, .val = 'h'},
  {.name = "partition", .has_arg = required_argument, .flag = 0, .val = 'i'},
#if 0//FIXME
  {.name = "bad", .has_arg = required_argument, .flag = 0, .val = 'b'},
  {.name = "successful", .has_arg = required_argument, .flag = 0, .val = 's'},
  {.name = "tries", .has_arg = required_argument, .flag = 0, .val = 't'},
  {.name = "priority", .has_arg = required_argument, .flag = 0, .val = 'p'},
#endif
  {.name = "type", .has_arg = required_argument, .flag = 0, .val = 't'},
  {.name = "unique", .has_arg = required_argument, .flag = 0, .val = 'u'},
  {.name = "begin", .has_arg = required_argument, .flag = 0, .val = 'b'},
  {.name = "size", .has_arg = required_argument, .flag = 0, .val = 's'},
  {.name = "name", .has_arg = required_argument, .flag = 0, .val = 'n'},
  { /* last element, which should be zero. */ }
};

/* Extra information than struct option, please update this structure if you
 * add/remove any line in attribute_options[]. */
static struct option_details adm_options_details[] = {
  /* help */
  { .comment = "print this help",
    .validator = AssignTrue,
    .valid_range = 0,
    .parsed = &help},
  /* partition */
  { .comment = "partition number (MUST HAVE)",
    .validator = InNumberRange,
    .valid_range = &range_127_0,
    .parsed = &partition},
#if 0//FIXME
  /* bad */
  { .comment = "mark partition bad",
    .validator = InNumberRange,
    .valid_range = &range_1_0,
    .parsed = &bad},
  /* successful */
  { .comment = "mark partition successful",
    .validator = InNumberRange,
    .valid_range = &range_1_0,
    .parsed = &successful},
  /* tries */
  { .comment = "tries",
    .validator = InNumberRange,
    .valid_range = &range_15_0,
    .parsed = &tries},
  /* priority */
  { .comment = "priority to boot",
    .validator = InNumberRange,
    .valid_range = &range_15_0,
    .parsed = &priority},
#endif
  /* type */
  { .comment = "Partition Type (xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx)",
    .validator = CopyString,
    .valid_range = (void*)sizeof(type),
    .parsed = &type},
  /* uuid */
  { .comment = "Partition UUID (xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx)",
    .validator = CopyString,
    .valid_range = (void*)sizeof(unique),
    .parsed = &unique},
  /* start */
  { .comment = "starting LBA",
    .validator = InNumberRange,
    .valid_range = 0,
    .parsed = &begin_lba},
  /* end */
  { .comment = "ending LBA",
    .validator = InNumberRange,
    .valid_range = 0,
    .parsed = &size_lba},
  /* name */
  { .comment = "Partition name",
    .validator = CopyString,
    .valid_range = (void*)sizeof(name),
    .parsed = &name},
  { /* last element, which should be zero. */ }
};

void AdmHelp() {
  printf("\nUsage: %s {add|delete|modify} [OPTIONS] device_name\n\n", progname);
  ShowOptions(adm_options, adm_options_details, ARRAY_COUNT(adm_options));
  PrintTypes();
  printf("\n");
}

enum {
  ADD,
  DELETE,
  MODIFY,
} command;

/* Parses all options (and validates them), then opens the drive and sets
 * corresponding bits in GPT entry. */
int CgptAdm(int argc, char *argv[]) {
  struct drive drive;
  char *cmd;
  GptEntry *entry;
  Guid type_guid, unique_guid;
  int dirty = 0;

  /* I know this is NOT the perfect place to put code to make options[] and
   * details[] are synced. But this is the best place we have right now since C
   * preprocessor doesn't know sizeof() for #if directive. */
  assert(ARRAY_COUNT(adm_options) ==
         ARRAY_COUNT(adm_options_details));

  cmd = argv[optind - 1];
  if (!strcmp("add", cmd)) command = ADD;
  else if (!strcmp("delete", cmd)) command = DELETE;
  else if (!strcmp("modify", cmd)) command = MODIFY;

#if 0//FIXME
  help = partition = bad = successful = tries = priority =
#endif
  help = partition = begin_lba = size_lba = NOT_INITED;
  type[0] = '\0';
  unique[0] = '\0';
  name[0] = '\0';

  if (CGPT_OK != HandleOptions(argc, argv,
                     "hi:t:u:b:s:n:",
                     ARRAY_COUNT(adm_options),
                     adm_options,
                     adm_options_details))
    return CGPT_FAILED;
  if (help != NOT_INITED) {
    AdmHelp();
    return CGPT_FAILED;
  }

  if (CGPT_OK != OpenDriveInLastArgument(argc, argv, &drive))
    return CGPT_FAILED;

  if (CheckValid(&drive) != CGPT_OK) goto error_close;

  if (partition == NOT_INITED) {
    printf("[ERROR] Please provide partition number with --partition or -i.\n");
    goto error_close;
  }

  entry = GetEntry(&drive.gpt, PRIMARY, partition);
  /* check before really doing something. */
  switch (command) {
    case ADD:
      if (NonZeroGuid(&entry->type)) {
        printf("[ERROR] partition %d is not free, use '%s modify' instead.\n",
               partition, progname);
        goto error_close;
      }
      if (type[0] == '\0') {
        printf("* You must give a type with '--type' or '-t'.\n");
        PrintTypes();
        goto error_close;
      }
      if (begin_lba == NOT_INITED) {
        printf("* You didn't give the begin LBA, use '--begin' to specify.\n");
        goto error_close;
      }
      if (size_lba == NOT_INITED) {
        printf("* You didn't give size, use '--size' to specify.\n");
        goto error_close;
      }
      break;
    case DELETE:
      if (!NonZeroGuid(&entry->type)) {
        printf("[ERROR] partition %d is free already.\n", partition);
        goto error_close;
      }
      break;
    case MODIFY:
      if (!NonZeroGuid(&entry->type)) {
        printf("[ERROR] partition %d is free, use '%s add' first.\n",
               partition, progname);
        goto error_close;
      }
      break;
  }

#if 0 //FIXME
  if (bad != NOT_INITED)
    SetBad(&drive.gpt, PRIMARY, partition, bad);
  if (successful != NOT_INITED)
    SetSuccessful(&drive.gpt, PRIMARY, partition, successful);
  if (tries != NOT_INITED)
    SetTries(&drive.gpt, PRIMARY, partition, tries);
  if (priority != NOT_INITED)
    SetPriority(&drive.gpt, PRIMARY, partition, priority);
#endif
  if (type[0]) {
    if (CGPT_OK != SupportedType(type, &type_guid) &&
        CGPT_OK != StrToGuid(type, &type_guid)) {
      printf("[ERROR] You didn't give a valid type [%s]\n", type);
      goto error_close;
    }
    Memcpy(&entry->type, &type_guid, sizeof(Guid));
    ++dirty;
  }
  if (unique[0]) {
    if (CGPT_OK != StrToGuid(unique, &unique_guid)) {
      printf("[ERROR] You didn't give a valid UUID [%s]\n", unique);
      goto error_close;
    }
    Memcpy(&entry->unique, &unique_guid, sizeof(Guid));
    ++dirty;
  }
  if (begin_lba != NOT_INITED) {
    entry->starting_lba = begin_lba;
    ++dirty;
  }
  if (size_lba != NOT_INITED) {
    entry->ending_lba = entry->starting_lba + size_lba - 1;
    ++dirty;
  }
  if (name[0]) {
    UTF8ToUTF16((uint8_t*)name, entry->name);
    ++dirty;
  }

  if (command == DELETE) {
    Guid unused = GPT_ENT_TYPE_UNUSED;
    Memcpy(&entry->type, &unused, sizeof(Guid));
  }

  if (dirty) {
    uint32_t valid_entries;

    valid_entries = drive.gpt.valid_entries;
    if ((valid_entries != CheckValidEntries(&drive.gpt)) ||
        (valid_entries != CheckOverlappedPartition(&drive.gpt))) {
      printf("\n[ERROR] Your change makes GPT invalid (or worse). "
             "Please check your arguments.\n\n");
      drive.gpt.modified = 0;  /* DriveClose() won't update hard drive. */
      goto error_close;
    }

    /* Claims primary is good, then secondary will be overwritten. */
    RepairEntries(&drive.gpt, MASK_PRIMARY);
    RepairHeader(&drive.gpt, MASK_PRIMARY);

    /* Forces headers and entries are modified so that CRC32 will be
     * re-calculated and headers and entries will be updated to drive. */
    drive.gpt.modified |= (GPT_MODIFIED_HEADER1 | GPT_MODIFIED_ENTRIES1 |
                           GPT_MODIFIED_HEADER2 | GPT_MODIFIED_ENTRIES2);
    UpdateCrc(&drive.gpt);
  }
  DriveClose(&drive);
  return CGPT_OK;

error_close:
  DriveClose(&drive);
  return CGPT_FAILED;
}
