// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <getopt.h>
#include <string.h>

#include "cgpt.h"
#include "vboot_host.h"

extern const char* progname;

static void Usage(void)
{
  printf("\nUsage: %s repair [OPTIONS] DRIVE\n\n"
         "Repair damaged GPT headers and tables.\n\n"
         "Options:\n"
         "  -D NUM       Size (in bytes) of the disk where partitions reside\n"
         "                 default 0, meaning partitions and GPT structs are\n"
         "                 both on DRIVE\n"
         "  -v           Verbose\n"
         "\n", progname);
}

int cmd_repair(int argc, char *argv[]) {
  CgptRepairParams params;
  memset(&params, 0, sizeof(params));

  int c;
  char* e = 0;
  int errorcnt = 0;

  opterr = 0;                     // quiet, you
  while ((c=getopt(argc, argv, ":hvD:")) != -1)
  {
    switch (c)
    {
    case 'D':
      params.drive_size = strtoull(optarg, &e, 0);
      if (!*optarg || (e && *e))
      {
        Error("invalid argument to -%c: \"%s\"\n", c, optarg);
        errorcnt++;
      }
      break;
    case 'v':
      params.verbose++;
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

  params.drive_name = argv[optind];

  return CgptRepair(&params);
}
