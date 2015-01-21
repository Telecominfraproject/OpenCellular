// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "cgpt.h"
#include "cgpt_nor.h"
#include "cgptlib_internal.h"
#include "vboot_host.h"

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

  if (params->numeric) {
    printf("%d\n", partnum);
  } else {
    if (params->show_fn) {
      params->show_fn(params, filename, partnum, entry);
    } else {
      printf(format, filename, partnum);
    }
  }
  if (params->verbose > 0)
    EntryDetails(entry, partnum - 1, params->numeric);
}

// This handles the MTD devices. ChromeOS uses /dev/mtdX for kernel partitions,
// /dev/ubiblockX_0 for root partitions, and /dev/ubiX for stateful partition.
static void chromeos_mtd_show(CgptFindParams *params, char *filename,
                              int partnum, GptEntry *entry) {
  if (GuidEqual(&guid_chromeos_kernel, &entry->type)) {
    printf("/dev/mtd%d\n", partnum);
  } else if (GuidEqual(&guid_chromeos_rootfs, &entry->type)) {
    printf("/dev/ubiblock%d_0\n", partnum);
  } else {
    printf("/dev/ubi%d_0\n", partnum);
  }
}

// This returns true if a GPT partition matches the search criteria. If a match
// isn't found (or if the file doesn't contain a GPT), it returns false. The
// filename and partition number that matched is left in a global, since we
// could have multiple hits.
static int gpt_search(CgptFindParams *params, struct drive *drive,
                      char *filename) {
  int i;
  GptEntry *entry;
  int retval = 0;
  char partlabel[GPT_PARTNAME_LEN];

  if (GPT_SUCCESS != GptSanityCheck(&drive->gpt)) {
    return 0;
  }

  for (i = 0; i < GetNumberOfEntries(drive); ++i) {
    entry = GetEntry(&drive->gpt, ANY_VALID, i);

    if (GuidIsZero(&entry->type))
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
    if (found && match_content(params, drive, entry)) {
      params->hits++;
      retval++;
      showmatch(params, filename, i+1, entry);
      if (!params->match_partnum)
        params->match_partnum = i+1;
    }
  }

  return retval;
}

static int do_search(CgptFindParams *params, char *fileName) {
  int retval;
  struct drive drive;

  if (CGPT_OK != DriveOpen(fileName, &drive, O_RDONLY, params->drive_size))
    return 0;

  retval = gpt_search(params, &drive, fileName);

  (void) DriveClose(&drive, 0);

  return retval;
}


#define PROC_MTD "/proc/mtd"
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
  char partname[128];                   // max size for /proc/partition lines?
  FILE *fp;
  char *pathname;

  fp = fopen(PROC_PARTITIONS, "re");
  if (!fp) {
    perror("can't read " PROC_PARTITIONS);
    return found;
  }

  size_t line_length = 0;
  char *line = NULL;
  while (getline(&line, &line_length, fp) != -1) {
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

  fp = fopen(PROC_MTD, "re");
  if (!fp) {
    free(line);
    return found;
  }

  while (getline(&line, &line_length, fp) != -1) {
    uint64_t sz;
    uint32_t erasesz;
    char name[128];
    // dev:  size  erasesize  name
    if (sscanf(line, "%64[^:]: %" PRIx64 " %x \"%127[^\"]\"",
               partname, &sz, &erasesz, name) != 4)
      continue;
    if (strcmp(partname, "mtd0") == 0) {
      char temp_dir[] = "/tmp/cgpt_find.XXXXXX";
      if (params->drive_size == 0) {
        if (GetMtdSize("/dev/mtd0", &params->drive_size) != 0) {
          perror("GetMtdSize");
          goto cleanup;
        }
      }
      if (ReadNorFlash(temp_dir) != 0) {
        perror("ReadNorFlash");
        goto cleanup;
      }
      char nor_file[64];
      if (snprintf(nor_file, sizeof(nor_file), "%s/rw_gpt", temp_dir) > 0) {
        params->show_fn = chromeos_mtd_show;
        if (do_search(params, nor_file)) {
          found++;
        }
        params->show_fn = NULL;
      }
      RemoveDir(temp_dir);
      break;
    }
  }
cleanup:
  fclose(fp);
  free(line);
  return found;
}


void CgptFind(CgptFindParams *params) {
  if (params == NULL)
    return;

  if (params->drive_name != NULL)
    do_search(params, params->drive_name);
  else
    scan_real_devs(params);
}
