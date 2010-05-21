/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Utility for ChromeOS-specific GPT partitions, Please see corresponding .c
 * files for more details.
 */
#include "cgpt.h"
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "utility.h"

/* For usage print */
const char* progname;

/* Lists all command here. */
struct {
  const char *name;
  int (*fp)(int argc, char *argv[]);
  const char *comment;
} cmds[] = {
  {"attribute", CgptAttribute, "Update GPT attribute bits "
                               "(for ChromeOS kernel entry only)"},
  {"repair", CgptRepair, "Repair primary and secondary headers and tables"},
  {"show", CgptShow, "Show partition details"},
};

/* Shows main menu. If 'message' is non-NULL, shows it as header. Then, this
 * traverses cmds[] and shows supported commands and their comments. */
void Usage(const char *message) {
  int i;

  if (message) printf("%s\n", message);
  printf("Usage: %s COMMAND [OPTIONS]\n\n"
         "Supported commands:\n\n",
         progname);
  for (i = 0; i < sizeof(cmds)/sizeof(cmds[0]); ++i) {
    printf("    %-10s  %s\n", cmds[i].name, cmds[i].comment);
  }
  printf("\nFor more detailed usage, use %s COMMAND --help.\n\n", progname);
}

/* Loads sectors from 'fd'.
 * *buf is pointed to an allocated memory when returned, and should be
 * freed by cgpt_close().
 *
 *   fd -- file descriptot.
 *   buf -- pointer to buffer pointer
 *   sector -- offset of starting sector (in sectors)
 *   sector_bytes -- bytes per sector
 *   sector_count -- number of sectors to load
 *
 * Returns CGPT_OK for successful. Aborts if any error occurs.
 */
int Load(const int fd, uint8_t **buf,
         const uint64_t sector,
         const uint64_t sector_bytes,
         const uint64_t sector_count) {
  int count;  /* byte count to read */
  int nread;

  assert(buf);
  count = sector_bytes * sector_count;
  *buf = Malloc(count);
  assert(*buf);

  if (-1 == lseek(fd, sector * sector_bytes, SEEK_SET))
    goto error_free;

  nread = read(fd, *buf, count);
  if (nread < count)
    goto error_free;

  return CGPT_OK;

error_free:
  Free(*buf);
  *buf = 0;
  abort();
}

/* Saves sectors to 'fd'.
 *
 *   fd -- file descriptot.
 *   buf -- pointer to buffer
 *   sector -- starting sector offset
 *   sector_bytes -- bytes per sector
 *   sector_count -- number of sector to save
 *
 * Returns CGPT_OK for successful, CGPT_FAILED for failed.
 */
int Save(const int fd, const uint8_t *buf,
         const uint64_t sector,
         const uint64_t sector_bytes,
         const uint64_t sector_count) {
  int count;  /* byte count to write */
  int nwrote;

  assert(buf);
  count = sector_bytes * sector_count;

  if (-1 == lseek(fd, sector * sector_bytes, SEEK_SET))
    return CGPT_FAILED;

  nwrote = write(fd, buf, count);
  if (nwrote < count)
    return CGPT_FAILED;

  return CGPT_OK;
}

/* Opens a block device (a regular file works well too).
 *
 * Returns CGPT_FAILED if any error happens.
 * Returns CGPT_OK if success and information are stored in 'drive'. */
int DriveOpen(const char *drive_path, struct drive *drive) {
  struct stat stat;
  int gpt_retval;

  assert(drive_path);
  assert(drive);

  Memset(drive, 0, sizeof(struct drive));
  drive->fd = open(drive_path, O_RDWR);
  if (drive->fd == -1) {
    printf("[ERROR] Cannot open drive file [%s]: %s\n",
           drive_path, strerror(errno));
    return CGPT_FAILED;
  }

  if (fstat(drive->fd, &stat) == -1) {
    goto error_close;
  }
  if ((stat.st_mode & S_IFMT) != S_IFREG) {
    if (ioctl(drive->fd, BLKGETSIZE64, &drive->size) < 0) {
      printf("[ERROR] Cannot get sector size from drive file [%s]: %s\n",
             drive_path, strerror(errno));
      goto error_close;
    }
    if (ioctl(drive->fd, BLKSSZGET, &drive->gpt.sector_bytes) < 0) {
      printf("[ERROR] Cannot get drive size from drive file [%s]: %s\n",
             drive_path, strerror(errno));
      goto error_close;
    }
  } else {
    drive->gpt.sector_bytes = 512;  /* bytes */
    drive->size = stat.st_size;
  }
  if (drive->size % drive->gpt.sector_bytes) {
    printf("[ERROR] Media size (%llu) is not the multiple of sector size(%d)\n",
           (long long unsigned int)drive->size, drive->gpt.sector_bytes);
    goto error_close;
  }
  drive->gpt.drive_sectors = drive->size / drive->gpt.sector_bytes;
  debug("drive: size:%llu sector_size:%d num_sector:%llu\n",
        (long long unsigned int)drive->size, drive->gpt.sector_bytes,
        (long long unsigned int)drive->gpt.drive_sectors);

  Load(drive->fd, &drive->gpt.primary_header, GPT_PMBR_SECTOR,
       drive->gpt.sector_bytes, GPT_HEADER_SECTOR);
  Load(drive->fd, &drive->gpt.secondary_header,
       drive->gpt.drive_sectors - GPT_PMBR_SECTOR,
       drive->gpt.sector_bytes, GPT_HEADER_SECTOR);
  Load(drive->fd, &drive->gpt.primary_entries,
       GPT_PMBR_SECTOR + GPT_HEADER_SECTOR,
       drive->gpt.sector_bytes, GPT_ENTRIES_SECTORS);
  Load(drive->fd, &drive->gpt.secondary_entries,
       drive->gpt.drive_sectors - GPT_HEADER_SECTOR - GPT_ENTRIES_SECTORS,
       drive->gpt.sector_bytes, GPT_ENTRIES_SECTORS);

  if (GPT_SUCCESS != (gpt_retval = GptInit(&drive->gpt))) {
    printf("[ERROR] GptInit(): %s\n", GptError(gpt_retval));
    goto error_close;
  }

  drive->inited = 1;

  return CGPT_OK;

error_close:
  close(drive->fd);
  return CGPT_FAILED;
}

int DriveClose(struct drive *drive) {
  if (drive->inited) {
    if (drive->gpt.modified & GPT_MODIFIED_HEADER1)
      assert(CGPT_OK ==
          Save(drive->fd, drive->gpt.primary_header, GPT_PMBR_SECTOR,
               drive->gpt.sector_bytes, GPT_HEADER_SECTOR));
    if (drive->gpt.modified & GPT_MODIFIED_HEADER2)
      assert(CGPT_OK ==
          Save(drive->fd, drive->gpt.secondary_header,
               drive->gpt.drive_sectors - GPT_PMBR_SECTOR,
               drive->gpt.sector_bytes, GPT_HEADER_SECTOR));
    if (drive->gpt.modified & GPT_MODIFIED_ENTRIES1)
      assert(CGPT_OK ==
          Save(drive->fd, drive->gpt.primary_entries,
               GPT_PMBR_SECTOR + GPT_HEADER_SECTOR,
               drive->gpt.sector_bytes, GPT_ENTRIES_SECTORS));
    if (drive->gpt.modified & GPT_MODIFIED_ENTRIES2)
      assert(CGPT_OK ==
          Save(drive->fd, drive->gpt.secondary_entries,
               drive->gpt.drive_sectors - GPT_HEADER_SECTOR -
                                          GPT_ENTRIES_SECTORS,
               drive->gpt.sector_bytes, GPT_ENTRIES_SECTORS));

    close(drive->fd);
  }

  Free(drive->gpt.primary_header);
  drive->gpt.primary_header = 0;
  Free(drive->gpt.primary_entries);
  drive->gpt.primary_entries = 0;
  Free(drive->gpt.secondary_header);
  drive->gpt.secondary_header = 0;
  Free(drive->gpt.secondary_entries);
  drive->gpt.secondary_entries = 0;

  drive->inited = 0;
  return CGPT_OK;
}

int main(int argc, char *argv[]) {
  char *cmd;
  int i;

  progname = argv[0];
  cmd = argv[optind++];
  for (i = 0; i < sizeof(cmds)/sizeof(cmds[0]); ++i) {
    if (cmd && !strcmp(cmds[i].name, cmd))
      return cmds[i].fp(argc, argv);
  }

  Usage(0);
  return CGPT_FAILED;
}
