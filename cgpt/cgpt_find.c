// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cgpt.h"

#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "cgptlib_internal.h"
#include "cgpt_params.h"

#define BUFSIZE 1024
// FIXME: currently we only support 512-byte sectors.
#define LBA_SIZE 512


// fill comparebuf with the data to be examined, returning true on success.
static int FillBuffer(CgptFindParams *params, int fd, uint64_t pos, 
                       uint64_t count) {
  uint8_t *bufptr = params->comparebuf;

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
static int match_content(CgptFindParams *params, struct drive *drive, 
                             GptEntry *entry) {
  uint64_t part_size;

  if (!params->matchlen)
    return 1;

  // Ensure that the region we want to match against is inside the partition.
  part_size = LBA_SIZE * (entry->ending_lba - entry->starting_lba + 1);
  if (params->matchoffset + params->matchlen > part_size) {
    return 0;
  }

  // Read the partition data.
  if (!FillBuffer(params,
                  drive->fd,
                  (LBA_SIZE * entry->starting_lba) + params->matchoffset,
                  params->matchlen)) {
    Error("unable to read partition data\n");
    return 0;
  }

  // Compare it
  if (0 == memcmp(params->matchbuf, params->comparebuf, params->matchlen)) {
    return 1;
  }

  // Nope.
  return 0;
}

// This needs to handle /dev/mmcblk0 -> /dev/mmcblk0p3, /dev/sda -> /dev/sda3
static void showmatch(CgptFindParams *params, char *filename, 
                           int partnum, GptEntry *entry) {
  char * format = "%s%d\n";
  if (strncmp("/dev/mmcblk", filename, 11) == 0)
    format = "%sp%d\n";
  if (params->numeric)
    printf("%d\n", partnum);
  else
    printf(format, filename, partnum);
  if (params->verbose > 0)
    EntryDetails(entry, partnum - 1, params->numeric);
}

// This returns true if a GPT partition matches the search criteria. If a match
// isn't found (or if the file doesn't contain a GPT), it returns false. The
// filename and partition number that matched is left in a global, since we
// could have multiple hits.
static int do_search(CgptFindParams *params, char *fileName) {
  int retval = 0;
  int i;
  struct drive drive;
  GptEntry *entry;
  char partlabel[GPT_PARTNAME_LEN];

  if (CGPT_OK != DriveOpen(fileName, &drive))
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
    if ((params->set_unique && GuidEqual(&params->unique_guid, &entry->unique))
        || (params->set_type && GuidEqual(&params->type_guid, &entry->type))) {
      found = 1;
    } else if (params->set_label) {
      if (CGPT_OK != UTF16ToUTF8(entry->name,
                                 sizeof(entry->name) / sizeof(entry->name[0]),
                                 (uint8_t *)partlabel, sizeof(partlabel))) {
        Error("The label cannot be converted from UTF16, so abort.\n");
        return 0;
      }
      if (!strncmp(params->label, partlabel, sizeof(partlabel)))
        found = 1;
    }
    if (found && match_content(params, &drive, entry)) {
      params->hits++;
      retval++;
      showmatch(params, fileName, i+1, entry);
      if (!params->match_partnum)
        params->match_partnum = i+1;
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
static int scan_real_devs(CgptFindParams *params) {
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
      if (do_search(params, pathname)) {
        found++;
      }
    }
  }

  fclose(fp);
  return found;
}


void cgpt_find(CgptFindParams *params) {
  if (params == NULL)
    return;

  if (params->driveName != NULL)
    do_search(params, params->driveName);
  else
    scan_real_devs(params);
}
