// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cgpt.h"

#include <getopt.h>
#include <string.h>

#include "cgpt_params.h"

static void Usage(void)
{
  printf("\nUsage: %s boot [OPTIONS] DRIVE\n\n"
         "Edit the PMBR sector for legacy BIOSes\n\n"
         "Options:\n"
         "  -i NUM       Set bootable partition\n"
         "  -b FILE      Install bootloader code in the PMBR\n"
         "  -p           Create legacy PMBR partition table\n"
         "\n"
         "With no options, it will just print the PMBR boot guid\n"
         "\n", progname);
}


int cmd_boot(int argc, char *argv[]) {
  CgptBootParams params;
  memset(&params, 0, sizeof(params));


  int c;
  int errorcnt = 0;
  char *e = 0;

  opterr = 0;                     // quiet, you
  while ((c=getopt(argc, argv, ":hi:b:p")) != -1)
  {
    switch (c)
    {
    case 'i':
      params.partition = (uint32_t)strtoul(optarg, &e, 0);
      if (!*optarg || (e && *e))
      {
        Error("invalid argument to -%c: \"%s\"\n", c, optarg);
        errorcnt++;
      }
      break;
    case 'b':
      params.bootfile = optarg;
      break;
    case 'p':
      params.create_pmbr = 1;
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

  params.drive_name = argv[optind];

  return cgpt_boot(&params);
}
