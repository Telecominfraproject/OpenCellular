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
         "  -M FILE"
         "      Matching partition data must also contain FILE content\n"
         "  -O NUM"
         "       Byte offset into partition to match content (default 0)\n"
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
static uint8_t *matchbuf = NULL;
static uint64_t matchlen = 0;
static uint64_t matchoffset = 0;
static uint8_t *comparebuf = NULL;

static Guid unique_guid;
static Guid type_guid;
static char *label;
static int hits = 0;

#define BUFSIZE 1024
// FIXME: currently we only support 512-byte sectors.
#define LBA_SIZE 512


// remember one of the possibly many hits
static int match_partnum = 0;           // 0 for no match, 1-N for match
static char match_filename[BUFSIZE];    // matching filename


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

// fill comparebuf with the data to be examined, returning true on success.
static int FillBuffer(int fd, uint64_t pos, uint64_t count) {
  uint8_t *bufptr = comparebuf;

  if (-1 == lseek(fd, pos, SEEK_SET))
    return 0;

  // keep reading until done or error
  while (count) {
    ssize_t bytes_read = read(fd, bufptr, count);
    // negative means error, 0 means (unexpected) EOF
    if (bytes_read <= 0)
      return 0;
    count -= bytes_read;
    bufptr += bytes_read;
  }

  return 1;
}

// check partition data content. return true for match, 0 for no match or error
static int match_content(struct drive *drive, GptEntry *entry) {
  uint64_t part_size;

  if (!matchlen)
    return 1;

  // Ensure that the region we want to match against is inside the partition.
  part_size = LBA_SIZE * (entry->ending_lba - entry->starting_lba + 1);
  if (matchoffset + matchlen > part_size) {
    return 0;
  }

  // Read the partition data.
  if (!FillBuffer(drive->fd,
                  (LBA_SIZE * entry->starting_lba) + matchoffset,
                  matchlen)) {
    Error("unable to read partition data\n");
    return 0;
  }

  // Compare it
  if (0 == memcmp(matchbuf, comparebuf, matchlen)) {
    return 1;
  }

  // Nope.
  return 0;
}

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
  char partlabel[GPT_PARTNAME_LEN];

  if (CGPT_OK != DriveOpen(filename, &drive))
    return 0;

  if (GPT_SUCCESS != GptSanityCheck(&drive.gpt)) {
    (void) DriveClose(&drive, 0);
    return 0;
  }

  for (i = 0; i < GetNumberOfEntries(&drive.gpt); ++i) {
    entry = GetEntry(&drive.gpt, ANY_VALID, i);

    if (IsZero(&entry->type))
      continue;

    int found = 0;
    if ((set_unique && GuidEqual(&unique_guid, &entry->unique)) ||
        (set_type && GuidEqual(&type_guid, &entry->type))) {
      found = 1;
    } else if (set_label) {
      if (CGPT_OK != UTF16ToUTF8(entry->name,
                                 sizeof(entry->name) / sizeof(entry->name[0]),
                                 (uint8_t *)partlabel, sizeof(partlabel))) {
        Error("The label cannot be converted from UTF16, so abort.\n");
        return 0;
      }
      if (!strncmp(label, partlabel, sizeof(partlabel))) {
        found = 1;
      }
    }
    if (found && match_content(&drive, entry)) {
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

  // It should be a block device under /dev/,
  for (i = 0; devdirs[i]; i++) {
    sprintf(pathname, "%s/%s", devdirs[i], basename);

    if (0 != stat(pathname, &statbuf))
      continue;

    if (!S_ISBLK(statbuf.st_mode))
      continue;

    // It should have a symlink called /sys/block/*/device
    sprintf(tmpname, "%s/%s/device", SYS_BLOCK_DIR, basename);

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
  char partname[128];                   // max size for /proc/partition lines?
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

    if (sscanf(line, " %d %d %llu %127[^\n ]", &ma, &mi, &sz, partname) != 4)
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
  char *e = 0;
  int c;

  opterr = 0;                     // quiet, you
  while ((c=getopt(argc, argv, ":hv1nt:u:l:M:O:")) != -1)
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
    case 'M':
      matchbuf = ReadFile(optarg, &matchlen);
      if (!matchbuf || !matchlen) {
        Error("Unable to read from %s\n", optarg);
        errorcnt++;
      }
      // Go ahead and allocate space for the comparison too
      comparebuf = (uint8_t *)malloc(matchlen);
      if (!comparebuf) {
        Error("Unable to allocate %" PRIu64 "bytes for comparison buffer\n",
              matchlen);
        errorcnt++;
      }
      break;
    case 'O':
      matchoffset = strtoull(optarg, &e, 0);
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
  if (!set_unique && !set_type && !set_label) {
    Error("You must specify at least one of -t, -u, or -l\n");
    errorcnt++;
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
