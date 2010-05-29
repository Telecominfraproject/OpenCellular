/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Developper mode.
 */
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include "cgpt.h"
#include "cgptlib_internal.h"
#include "utility.h"

/* Integers to store parsed argument. */
static int help, primary_header, primary_entries,
           secondary_header, secondary_entries;

/* The structure for getopt_long(). When you add/delete any line, please refine
 * attribute_comments[] and third parameter of getopt_long() too.  */
static struct option dev_options[] = {
  {.name = "help", .has_arg = no_argument, .flag = 0, .val = 'h'},
  {.name = "primary-header", .has_arg = no_argument, .flag = 0, .val = 'a'},
  {.name = "primary-entries", .has_arg = no_argument, .flag = 0, .val = 'b'},
  {.name = "secondary-entries", .has_arg = no_argument, .flag = 0, .val = 'c'},
  {.name = "secondary-header", .has_arg = no_argument, .flag = 0, .val = 'd'},
  { /* last element, which should be zero. */ }
};

/* Extra information than struct option, please update this structure if you
 * add/remove any line in attribute_options[]. */
static struct option_details dev_options_details[] = {
  /* help */
  { .comment = "print this help",
    .validator = AssignTrue,
    .valid_range = 0,
    .parsed = &help},
  /* primary-header */
  { .comment = "damage primary header",
    .validator = AssignTrue,
    .valid_range = 0,
    .parsed = &primary_header},
  /* primary-entries */
  { .comment = "damage primary entries",
    .validator = AssignTrue,
    .valid_range = 0,
    .parsed = &primary_entries},
  /* secondary-entries */
  { .comment = "damage secondary entries",
    .validator = AssignTrue,
    .valid_range = 0,
    .parsed = &secondary_entries},
  /* secondary-header */
  { .comment = "damage secondary header",
    .validator = AssignTrue,
    .valid_range = 0,
    .parsed = &secondary_header},
  { /* last element, which should be zero. */ }
};

void DevHelp() {
  printf("\nDeveloper mode.\n\n");
  printf("\nUsage: %s dev [OPTIONS] device_name\n\n", progname);
  ShowOptions(dev_options, dev_options_details, ARRAY_COUNT(dev_options));
  printf("\n");
}

/* Very simple function, you may choose damage one or more of the following
 * sections:
 *
 *   Primary GPT header
 *   Primary GPT table entries
 *   Secondary GPT table entries
 *   Secondary GPT header
 */
int CgptDev(int argc, char *argv[]) {
  struct drive drive;

  /* I know this is NOT the perfect place to put code to make options[] and
   * details[] are synced. But this is the best place we have right now since C
   * preprocessor doesn't know sizeof() for #if directive. */
  assert(ARRAY_COUNT(dev_options) ==
         ARRAY_COUNT(dev_options_details));

  help = primary_header = primary_entries =
         secondary_header = secondary_entries = NOT_INITED;

  if (CGPT_OK != HandleOptions(argc, argv,
                     "h",
                     ARRAY_COUNT(dev_options),
                     dev_options,
                     dev_options_details))
    return CGPT_FAILED;
  if (help != NOT_INITED) {
    DevHelp();
    return CGPT_FAILED;
  }

  if (CGPT_OK != OpenDriveInLastArgument(argc, argv, &drive))
    return CGPT_FAILED;

  #define ANY_PRIME 7
  if (primary_header != NOT_INITED) {
    printf("* damage Pri Header\n");
    drive.gpt.primary_header[0] += ANY_PRIME;
    drive.gpt.modified |= GPT_MODIFIED_HEADER1;
  }
  if (primary_entries != NOT_INITED) {
    printf("* damage Pri Table\n");
    drive.gpt.primary_entries[0] += ANY_PRIME;
    drive.gpt.modified |= GPT_MODIFIED_ENTRIES1;
  }
  if (secondary_entries != NOT_INITED) {
    printf("* damage Sec Table\n");
    drive.gpt.secondary_entries[0] += ANY_PRIME;
    drive.gpt.modified |= GPT_MODIFIED_ENTRIES2;
  }
  if (secondary_header != NOT_INITED) {
    printf("* damage Sec Header\n");
    drive.gpt.secondary_header[0] += ANY_PRIME;
    drive.gpt.modified |= GPT_MODIFIED_HEADER2;
  }

  DriveClose(&drive);

  return CGPT_OK;
}
