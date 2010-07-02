// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cgpt.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <uuid/uuid.h>

#include "cgptlib_internal.h"


static void Usage(void)
{
  printf("\nUsage: %s find [OPTIONS] [DRIVE]\n\n"
         "Find a partition by its UUID or label. With no specified DRIVE\n"
         "it scans all physical drives.\n\n"
         "Options:\n"
         "  -t GUID      Search for Partition Type GUID\n"
         "  -u GUID      Search for Partition Unique ID\n"
         "  -l LABEL     Search for Label\n"
         "  -v           Be verbose in displaying matches (repeatable)\n"
         "  -n           Numeric output only\n"
         "  -1           Fail if more than one match is found\n"
         "\n", progname);
  PrintTypes();
}


// globals
static int verbose = 0;
static int set_unique = 0;
static int set_type = 0;
static int set_label = 0;
static int oneonly = 0;
static int numeric = 0;

static Guid unique_guid;
static Guid type_guid;
static char *label;
static int hits = 0;

#define BUFSIZE 1024


// remember one of the possibly many hits
static int match_partnum = 0;           // 0 for no match, 1-N for match
static char match_filename[BUFSIZE];    // matching filename


// FIXME: This needs to handle /dev/mmcblk0 -> /dev/mmcblk0p3
static void showmatch(char *filename, int partnum, GptEntry *entry) {
  printf("%s%d\n", filename, partnum);
  if (verbose > 0)
    EntryDetails(entry, partnum - 1, numeric);
}

// This returns true if a GPT partition matches the search criteria. If a match
// isn't found (or if the file doesn't contain a GPT), it returns false. The
// filename and partition number that matched is left in a global, since we
// could have multiple hits.
static int do_search(char *filename) {
  int retval = 0;
  int i;
  struct drive drive;
  GptEntry *entry;
  char partlabel[sizeof(entry->name) * 3 / 2];

  if (CGPT_OK != DriveOpen(filename, &drive))
    return 0;

  if (GPT_SUCCESS != GptSanityCheck(&drive.gpt)) {
    (void) DriveClose(&drive, 0);
    return 0;
  }

  for (i = 0; i < GetNumberOfEntries(&drive.gpt); ++i) {
    entry = GetEntry(&drive.gpt, PRIMARY, i);

    if (IsZero(&entry->type))
      continue;

    int found = 0;
    if ((set_unique && !memcmp(&unique_guid, &entry->unique, sizeof(Guid))) ||
        (set_type && !memcmp(&type_guid, &entry->type, sizeof(Guid)))) {
      found = 1;
    } else if (set_label) {
      UTF16ToUTF8(entry->name, (uint8_t *)partlabel);
      if (!strncmp(label, partlabel, sizeof(partlabel))) {
        found = 1;
      }
    }
    if (found) {
      hits++;
      retval++;
      showmatch(filename, i+1, entry);
      if (!match_partnum) {
        match_partnum = i+1;
        strcpy(match_filename, filename);
      }
    }
  }

  (void) DriveClose(&drive, 0);

  return retval;
}
  

#define PROC_PARTITIONS "/proc/partitions"
#define DEV_DIR "/dev"
#define SYS_BLOCK_DIR "/sys/block"

static const char *devdirs[] = { "/dev", "/devices", "/devfs", 0 };

// Given basename "foo", see if we can find a whole, real device by that name.
// This is copied from the logic in the linux utility 'findfs', although that
// does more exhaustive searching.
static char *is_wholedev(const char *basename) {
  int i;
  struct stat statbuf;
  static char pathname[BUFSIZE];        // we'll return this.
  char tmpname[BUFSIZE];

//   printf("basename is %s\n", basename);

  // It should be a block device under /dev/, 
  for (i = 0; devdirs[i]; i++) {
    sprintf(pathname, "%s/%s", devdirs[i], basename);
//     printf(" look at %s\n", pathname);

    if (0 != stat(pathname, &statbuf))
      continue;

    if (!S_ISBLK(statbuf.st_mode))
      continue;

    // It should have a symlink called /sys/block/*/device
    sprintf(tmpname, "%s/%s/device", SYS_BLOCK_DIR, basename);
//     printf(" look at %s\n", tmpname);

    if (0 != lstat(tmpname, &statbuf))
      continue;

    if (!S_ISLNK(statbuf.st_mode))
      continue;

    // found it
    return pathname;
  }

  return 0;
}


// This scans all the physical devices it can find, looking for a match. It
// returns true if any matches were found, false otherwise.
static int scan_real_devs(void) {
  int found = 0;
  char line[BUFSIZE];
  char partname[128];
  FILE *fp;
  char *pathname;

  fp = fopen(PROC_PARTITIONS, "r");
  if (!fp) {
    perror("can't read " PROC_PARTITIONS);
    return found;
  }

  while (fgets(line, sizeof(line), fp)) {
    int ma, mi;
    long long unsigned int sz;
    
    if (sscanf(line, " %d %d %llu %128[^\n ]", &ma, &mi, &sz, partname) != 4)
      continue;

    if ((pathname = is_wholedev(partname))) {
      if (do_search(pathname)) {
        found++;
      }
    }
  }
  
  fclose(fp);
  return found;
}
  

int cmd_find(int argc, char *argv[]) {
  int i;
  
  int errorcnt = 0;
  int c;
  
  opterr = 0;                     // quiet, you
  while ((c=getopt(argc, argv, ":hv1nt:u:l:")) != -1)
  {
    switch (c)
    {
    case 'v':
      verbose++;
      break;
    case 'n':
      numeric = 1;
      break;
    case '1':
      oneonly = 1;
      break;
    case 'l':
      set_label = 1;
      label = optarg;
      break;
    case 't':
      set_type = 1;
      if (CGPT_OK != SupportedType(optarg, &type_guid) &&
          CGPT_OK != StrToGuid(optarg, &type_guid)) {
        Error("invalid argument to -%c: %s\n", c, optarg);
        errorcnt++;
      }
      break;
    case 'u':
      set_unique = 1;
      if (CGPT_OK != StrToGuid(optarg, &unique_guid)) {
        Error("invalid argument to -%c: %s\n", c, optarg);
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


  if (optind < argc) {
    for (i=optind; i<argc; i++)
      do_search(argv[i]);
  } else {
    scan_real_devs();
  }

  if (oneonly && hits != 1) {
    return CGPT_FAILED;
  }

  if (match_partnum) {
    return CGPT_OK;
  }

  return CGPT_FAILED;
}
