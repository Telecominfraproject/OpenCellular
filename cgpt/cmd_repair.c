// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cgpt.h"

#include <getopt.h>
#include <string.h>

#include "cgpt_params.h"

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

  CgptRepairParams params;
  memset(&params, 0, sizeof(params));

  int c;
  int errorcnt = 0;

  opterr = 0;                     // quiet, you
  while ((c=getopt(argc, argv, ":hv")) != -1)
  {
    switch (c)
    {
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

  return cgpt_repair(&params);
}
