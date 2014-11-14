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
  printf("\nUsage: %s add [OPTIONS] DRIVE\n\n"
         "Add, edit, or remove a partition entry.\n\n"
         "Options:\n"
         "  -D NUM       Size (in bytes) of the disk where partitions reside\n"
         "                 default 0, meaning partitions and GPT structs are\n"
         "                 both on DRIVE\n"
         "  -i NUM       Specify partition (default is next available)\n"
         "  -b NUM       Beginning sector\n"
         "  -s NUM       Size in sectors\n"
         "  -t GUID      Partition Type GUID\n"
         "  -u GUID      Partition Unique ID\n"
         "  -l LABEL     Label\n"
         "  -S NUM       set Successful flag (0|1)\n"
         "  -T NUM       set Tries flag (0-15)\n"
         "  -P NUM       set Priority flag (0-15)\n"
         "  -A NUM       set raw 64-bit attribute value\n"
         "\n"
         "Use the -i option to modify an existing partition.\n"
         "The -b, -s, and -t options must be given for new partitions.\n"
         "\n", progname);
  PrintTypes();
}

int cmd_add(int argc, char *argv[]) {

  CgptAddParams params;
  memset(&params, 0, sizeof(params));

  int c;
  int errorcnt = 0;
  char *e = 0;

  opterr = 0;                     // quiet, you
  while ((c=getopt(argc, argv, ":hi:b:s:t:u:l:S:T:P:A:D:")) != -1)
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
      params.partition = (uint32_t)strtoul(optarg, &e, 0);
      if (!*optarg || (e && *e))
      {
        Error("invalid argument to -%c: \"%s\"\n", c, optarg);
        errorcnt++;
      }
      break;
    case 'b':
      params.set_begin = 1;
      params.begin = strtoull(optarg, &e, 0);
      if (!*optarg || (e && *e))
      {
        Error("invalid argument to -%c: \"%s\"\n", c, optarg);
        errorcnt++;
      }
      break;
    case 's':
      params.set_size = 1;
      params.size = strtoull(optarg, &e, 0);
      if (!*optarg || (e && *e))
      {
        Error("invalid argument to -%c: \"%s\"\n", c, optarg);
        errorcnt++;
      }
      break;
    case 't':
      params.set_type = 1;
      if (CGPT_OK != SupportedType(optarg, &params.type_guid) &&
          CGPT_OK != StrToGuid(optarg, &params.type_guid)) {
        Error("invalid argument to -%c: %s\n", c, optarg);
        errorcnt++;
      }
      break;
    case 'u':
      params.set_unique = 1;
      if (CGPT_OK != StrToGuid(optarg, &params.unique_guid)) {
        Error("invalid argument to -%c: %s\n", c, optarg);
        errorcnt++;
      }
      break;
    case 'l':
      params.label = optarg;
      break;
    case 'S':
      params.set_successful = 1;
      params.successful = (uint32_t)strtoul(optarg, &e, 0);
      if (!*optarg || (e && *e))
      {
        Error("invalid argument to -%c: \"%s\"\n", c, optarg);
        errorcnt++;
      }
      if (params.successful < 0 || params.successful > 1) {
        Error("value for -%c must be between 0 and 1", c);
        errorcnt++;
      }
      break;
    case 'T':
      params.set_tries = 1;
      params.tries = (uint32_t)strtoul(optarg, &e, 0);
      if (!*optarg || (e && *e))
      {
        fprintf(stderr, "%s: invalid argument to -%c: \"%s\"\n",
                progname, c, optarg);
        errorcnt++;
      }
      if (params.tries < 0 || params.tries > 15) {
        Error("value for -%c must be between 0 and 15", c);
        errorcnt++;
      }
      break;
    case 'P':
      params.set_priority = 1;
      params.priority = (uint32_t)strtoul(optarg, &e, 0);
      if (!*optarg || (e && *e))
      {
        Error("invalid argument to -%c: \"%s\"\n", c, optarg);
        errorcnt++;
      }
      if (params.priority < 0 || params.priority > 15) {
        Error("value for -%c must be between 0 and 15", c);
        errorcnt++;
      }
      break;
    case 'A':
      params.set_raw = 1;
      params.raw_value = strtoull(optarg, &e, 0);
      if (!*optarg || (e && *e))
      {
        Error("invalid argument to -%c: \"%s\"\n", c, optarg);
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

  if (optind >= argc)
  {
    Error("missing drive argument\n");
    return CGPT_FAILED;
  }

  params.drive_name = argv[optind];

  return CgptAdd(&params);
}
