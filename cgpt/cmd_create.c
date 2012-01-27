// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cgpt.h"

#include <getopt.h>
#include <string.h>

#include "cgpt_params.h"

static void Usage(void)
{
  printf("\nUsage: %s create [OPTIONS] DRIVE\n\n"
         "Create or reset an empty GPT.\n\n"
         "Options:\n"
         "  -z           Zero the sectors of the GPT table and entries\n"
         "\n", progname);
}

int cmd_create(int argc, char *argv[]) {
  struct drive drive;

  CgptCreateParams params;
  memset(&params, 0, sizeof(params));

  int c;
  int errorcnt = 0;

  opterr = 0;                     // quiet, you
  while ((c=getopt(argc, argv, ":hz")) != -1)
  {
    switch (c)
    {
    case 'z':
      params.zap = 1;
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

  params.driveName = argv[optind];

  return cgpt_create(&params);
}
