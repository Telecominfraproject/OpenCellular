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

static struct number_range range_1_0 = {1, 0};
static struct number_range range_15_0 = {15, 0};
static struct number_range range_16_1 = {16, 1};

/* Integers to store parsed argument. */
static int help, partition, successful, tries, priority;

/* The structure for getopt_long(). When you add/delete any line, please refine
 * attribute_comments[] and third parameter of getopt_long() too.  */
static struct option attribute_options[] = {
  {.name = "help", .has_arg = no_argument, .flag = 0, .val = 'h'},
  {.name = "partition", .has_arg = required_argument, .flag = 0, .val = 'i'},
  {.name = "successful", .has_arg = required_argument, .flag = 0, .val = 's'},
  {.name = "tries", .has_arg = required_argument, .flag = 0, .val = 't'},
  {.name = "priority", .has_arg = required_argument, .flag = 0, .val = 'p'},
  { /* last element, which should be zero. */ }
};

/* Extra information than struct option, please update this structure if you
 * add/remove any line in attribute_options[]. */
static struct option_details attribute_options_details[] = {
  /* help */
  { .comment = "print this help",
    .validator = AssignTrue,
    .valid_range = 0,
    .parsed = &help},
  /* partition */
  { .comment = "partition number "
              "(default: first ChromeOS kernel)",
    .validator = InNumberRange,
    .valid_range = &range_16_1,
    .parsed = &partition},
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
  { /* last element, which should be zero. */ }
};

void AttributeHelp() {
  printf("\nUsage: %s attribute [OPTIONS] device_name\n\n", progname);
  ShowOptions(attribute_options, attribute_options_details,
              ARRAY_COUNT(attribute_options));
}

/* Parses all options (and validates them), then opens the drive and sets
 * corresponding bits in GPT entry. */
int CgptAttribute(int argc, char *argv[]) {
  struct drive drive;
  GptEntry *entry;

  /* I know this is NOT the perfect place to put code to make options[] and
   * details[] are synced. But this is the best place we have right now since C
   * preprocessor doesn't know sizeof() for #if directive. */
  assert(ARRAY_COUNT(attribute_options) ==
         ARRAY_COUNT(attribute_options_details));

  help = partition = successful = tries = priority = NOT_INITED;

  if (CGPT_OK != HandleOptions(argc, argv,
                     "hi:b:s:t:p:",
                     ARRAY_COUNT(attribute_options),
                     attribute_options,
                     attribute_options_details))
    return CGPT_FAILED;
  if (help != NOT_INITED) {
    AttributeHelp();
    return CGPT_FAILED;
  }

  if (CGPT_OK != OpenDriveInLastArgument(argc, argv, &drive))
    return CGPT_FAILED;

  if (CheckValid(&drive) != CGPT_OK) return CGPT_FAILED;

  /* partition is not specified, search for the first Chromeos kernel. */
  if (partition == NOT_INITED) {
    int i;
    for (i = 0; i < GetNumberOfEntries(&drive.gpt); ++i) {
      entry = GetEntry(&drive.gpt, PRIMARY, i);
      if (!Memcmp(&guid_chromeos_kernel, &entry->type, sizeof(Guid))) {
        partition = i+1;
        break;
      }
    }
    if (partition == NOT_INITED) {
      printf("[ERROR] No ChromeOS kernel partition found. "
             "Please use --partition to specify.\n");
      return CGPT_FAILED;
    } else {
      debug("No --partition is specified. "
            "Found the first ChromeOS kernel in partition [%d].\n",
            partition);
    }
  }
  int index = partition - 1;

  if (successful != NOT_INITED)
    SetSuccessful(&drive.gpt, PRIMARY, index, successful);
  if (tries != NOT_INITED)
    SetTries(&drive.gpt, PRIMARY, index, tries);
  if (priority != NOT_INITED)
    SetPriority(&drive.gpt, PRIMARY, index, priority);

  /* Display state */
  entry = GetEntry(&drive.gpt, PRIMARY, index);
  EntryDetails(entry, index, NOT_INITED);

  /* Claims primary is good, then secondary will be overwritten. */
  /* TODO: rspangler broke this during cgptlib refactoring; need to
   * update this to match new internal APIs. */
  RepairEntries(&drive.gpt, MASK_PRIMARY);
  RepairHeader(&drive.gpt, MASK_PRIMARY);

  /* Forces headers and entries are modified so that CRC32 will be re-calculated
   * and headers and entries will be updated to drive. */
  drive.gpt.modified |= (GPT_MODIFIED_HEADER1 | GPT_MODIFIED_ENTRIES1 |
                         GPT_MODIFIED_HEADER2 | GPT_MODIFIED_ENTRIES2);
  UpdateCrc(&drive.gpt);
  DriveClose(&drive);

  return CGPT_OK;
}
