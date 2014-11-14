// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uuid/uuid.h>

#include "cgpt.h"
#include "vboot_host.h"

extern const char* progname;

static void Usage(void)
{
  printf("\nUsage: %s prioritize [OPTIONS] DRIVE\n\n"
         "Reorder the priority of all active ChromeOS Kernel partitions.\n\n"
         "Options:\n"
         "  -D NUM       Size (in bytes) of the disk where partitions reside\n"
         "                 default 0, meaning partitions and GPT structs are\n"
         "                 both on DRIVE\n"
         "  -P NUM       Highest priority to use in the new ordering. The\n"
         "                 other partitions will be ranked in decreasing\n"
         "                 priority while preserving their original order.\n"
         "                 If necessary the lowest ranks will be coalesced.\n"
         "                 No active kernels will be lowered to priority 0.\n"
         "  -i NUM       Specify the partition to make the highest in the new\n"
         "                 order.\n"
         "  -f           Friends of the given partition (those with the same\n"
         "                 starting priority) are also updated to the new\n"
         "                 highest priority.\n"
         "\n"
         "With no options this will set the lowest active kernel to\n"
         "priority 1 while maintaining the original order.\n"
         "\n", progname);
}

int cmd_prioritize(int argc, char *argv[]) {
  CgptPrioritizeParams params;
  memset(&params, 0, sizeof(params));

  int c;
  int errorcnt = 0;
  char *e = 0;

  opterr = 0;                     // quiet, you
  while ((c=getopt(argc, argv, ":hi:fP:D:")) != -1)
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
    case 'i':
      params.set_partition = (uint32_t)strtoul(optarg, &e, 0);
      if (!*optarg || (e && *e))
      {
        Error("invalid argument to -%c: \"%s\"\n", c, optarg);
        errorcnt++;
      }
      break;
    case 'f':
      params.set_friends = 1;
      break;
    case 'P':
      params.max_priority = (int)strtol(optarg, &e, 0);
      if (!*optarg || (e && *e))
      {
        Error("invalid argument to -%c: \"%s\"\n", c, optarg);
        errorcnt++;
      }
      if (params.max_priority < 1 || params.max_priority > 15) {
        Error("value for -%c must be between 1 and 15\n", c);
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

  if (params.set_friends && !params.set_partition) {
    Error("the -f option is only useful with the -i option\n");
    Usage();
    return CGPT_FAILED;
  }

  if (optind >= argc) {
    Error("missing drive argument\n");
    return CGPT_FAILED;
  }

  params.drive_name = argv[optind];

  return CgptPrioritize(&params);
}
