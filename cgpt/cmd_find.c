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
  printf("\nUsage: %s find [OPTIONS] [DRIVE]\n\n"
         "Find a partition by its UUID or label. With no specified DRIVE\n"
         "it scans all physical drives.\n\n"
         "Options:\n"
         "  -D NUM       Size (in bytes) of the disk where partitions reside\n"
         "                 default 0, meaning partitions and GPT structs are\n"
         "                 both on DRIVE\n"
         "  -t GUID      Search for Partition Type GUID\n"
         "  -u GUID      Search for Partition Unique ID\n"
         "  -l LABEL     Search for Label\n"
         "  -v           Be verbose in displaying matches (repeatable)\n"
         "  -n           Numeric output only\n"
         "  -1           Fail if more than one match is found\n"
         "  -M FILE"
         "      Matching partition data must also contain FILE content\n"
         "  -O NUM"
         "       Byte offset into partition to match content (default 0)\n"
         "\n", progname);
  PrintTypes();
}

// read a file into a buffer, return buffer and update size
static uint8_t *ReadFile(const char *filename, uint64_t *size) {
  FILE *f;
  uint8_t *buf;

  f = fopen(filename, "rb");
  if (!f) {
    return NULL;
  }

  fseek(f, 0, SEEK_END);
  *size = ftell(f);
  rewind(f);

  buf = malloc(*size);
  if (!buf) {
    fclose(f);
    return NULL;
  }

  if(1 != fread(buf, *size, 1, f)) {
    fclose(f);
    free(buf);
    return NULL;
  }

  fclose(f);
  return buf;
}

int cmd_find(int argc, char *argv[]) {

  CgptFindParams params;
  memset(&params, 0, sizeof(params));

  int i;
  int errorcnt = 0;
  char *e = 0;
  int c;

  opterr = 0;                     // quiet, you
  while ((c=getopt(argc, argv, ":hv1nt:u:l:M:O:D:")) != -1)
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
    case 'n':
      params.numeric = 1;
      break;
    case '1':
      params.oneonly = 1;
      break;
    case 'l':
      params.set_label = 1;
      params.label = optarg;
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
    case 'M':
      params.matchbuf = ReadFile(optarg, &params.matchlen);
      if (!params.matchbuf || !params.matchlen) {
        Error("Unable to read from %s\n", optarg);
        errorcnt++;
      }
      // Go ahead and allocate space for the comparison too
      params.comparebuf = (uint8_t *)malloc(params.matchlen);
      if (!params.comparebuf) {
        Error("Unable to allocate %" PRIu64 "bytes for comparison buffer\n",
              params.matchlen);
        errorcnt++;
      }
      break;
    case 'O':
      params.matchoffset = strtoull(optarg, &e, 0);
      if (!*optarg || (e && *e)) {
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
  if (!params.set_unique && !params.set_type && !params.set_label) {
    Error("You must specify at least one of -t, -u, or -l\n");
    errorcnt++;
  }
  if (errorcnt)
  {
    Usage();
    return CGPT_FAILED;
  }

  if (optind < argc) {
    for (i=optind; i<argc; i++) {
      params.drive_name = argv[i];
      CgptFind(&params);
      }
  } else {
      CgptFind(&params);
  }

  if (params.oneonly && params.hits != 1) {
    return CGPT_FAILED;
  }

  if (params.match_partnum) {
    return CGPT_OK;
  }

  return CGPT_FAILED;
}
