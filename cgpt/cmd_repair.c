// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cgpt.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cgptlib_internal.h"

static void Usage(void)
{
  printf("\nUsage: %s repair [OPTIONS] DRIVE\n\n"
         "Repair damaged GPT headers and tables.\n\n"
         "Options:\n"
         "  -v           Verbose\n"
         "\n", progname);
}

int cmd_repair(int argc, char *argv[]) {
  struct drive drive;
  int verbose = 0;
  
  int c;
  int errorcnt = 0;

  opterr = 0;                     // quiet, you
  while ((c=getopt(argc, argv, ":hv")) != -1)
  {
    switch (c)
    {
    case 'v':
      verbose++;
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

  int gpt_retval = GptSanityCheck(&drive.gpt);
  if (verbose)
    printf("GptSanityCheck() returned %d: %s\n",
           gpt_retval, GptError(gpt_retval));

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
