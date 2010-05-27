/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Repair headers and tables.
 *
 * If primary header or table is invalid, it copies from secondary (vice versa).
 */
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include "cgpt.h"
#include "cgptlib_internal.h"
#include "utility.h"

/* Integers to store parsed argument. */
static int help;

/* The structure for getopt_long(). When you add/delete any line, please refine
 * attribute_comments[] and third parameter of getopt_long() too.  */
static struct option repair_options[] = {
  {.name = "help", .has_arg = no_argument, .flag = 0, .val = 'h'},
};

/* Extra information than struct option, please update this structure if you
 * add/remove any line in attribute_options[]. */
static struct option_details repair_options_details[] = {
  /* help */
  { .comment = "print this help",
    .validator = AssignTrue,
    .valid_range = 0,
    .parsed = &help},
};

void RepairHelp() {
  printf("\nUsage: %s repair [OPTIONS] device_name\n\n", progname);
  ShowOptions(repair_options, repair_options_details,
              ARRAY_COUNT(repair_options));
  printf("\n");
}

/* Parses all options (and validates them), then opens the drive and sets
 * corresponding bits in GPT entry. */
int CgptRepair(int argc, char *argv[]) {
  struct drive drive;

  /* I know this is NOT the perfect place to put code to make options[] and
   * details[] are synced. But this is the best place we have right now since C
   * preprocessor doesn't know sizeof() for #if directive. */
  assert(ARRAY_COUNT(repair_options) ==
         ARRAY_COUNT(repair_options_details));

  help = NOT_INITED;

  if (CGPT_OK != HandleOptions(argc, argv,
                     "hr",
                     ARRAY_COUNT(repair_options),
                     repair_options,
                     repair_options_details))
    return CGPT_FAILED;
  if (help != NOT_INITED) {
    RepairHelp();
    return CGPT_FAILED;
  }

  if (CGPT_OK != OpenDriveInLastArgument(argc, argv, &drive))
    return CGPT_FAILED;

  GptRepair(&drive.gpt);
  if (drive.gpt.modified & GPT_MODIFIED_HEADER1)
    printf("* Pri Header is updated.\n");
  if (drive.gpt.modified & GPT_MODIFIED_ENTRIES1)
    printf("* Pri Table Entries is updated.\n");
  if (drive.gpt.modified & GPT_MODIFIED_ENTRIES2)
    printf("* Sec Table Entries is updated.\n");
  if (drive.gpt.modified & GPT_MODIFIED_HEADER2)
    printf("* Sec Header is updated.\n");

  DriveClose(&drive);

  return CGPT_OK;
}
