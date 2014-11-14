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
  printf("\nUsage: %s legacy [OPTIONS] DRIVE\n\n"
         "Switch GPT header signature to \"CHROMEOS\".\n\n"
         "Options:\n"
         "  -D NUM       Size (in bytes) of the disk where partitions reside\n"
         "                 default 0, meaning partitions and GPT structs are\n"
         "                 both on DRIVE\n"
         "  -e           Switch GPT header signature back to \"EFI PART\"\n"
         "\n", progname);
}

int cmd_legacy(int argc, char *argv[]) {
  CgptLegacyParams params;
  memset(&params, 0, sizeof(params));

  int c;
  char* e = 0;
  int errorcnt = 0;

  opterr = 0;                     // quiet, you
  while ((c=getopt(argc, argv, ":heD:")) != -1)
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
    case 'e':
      params.efipart = 1;
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
    Usage();
    return CGPT_FAILED;
  }

  params.drive_name = argv[optind];

  return CgptLegacy(&params);
}
